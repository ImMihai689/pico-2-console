/*-----------------------------------------------------------------------\
|  Low level disk implementation for SDHC cards                          |
\-----------------------------------------------------------------------*/

#include "ff.h"
#include "diskio.h"
#include "sdconf.h"

#include "hardware/timer.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"

volatile DSTATUS sta = STA_NOINIT;

DSTATUS disk_status (BYTE pdrv)
{
    if(pdrv != 0)
        return STA_NOINIT | STA_NODISK;
    return sta;
}

/// @brief Send dummy clocks (MOSI held high)
/// @param sdbd_cfg 
/// @param count The amount of bytes to send (no. of clocks = count * 8)
void sd_spi_dummy_clocks(uint count)
{
    uint8_t ones = 0xff;
    for(int i = 0; i < count; i++)
        spi_write_blocking(SD_SPI, &ones, 1);
}


void sdbd_spi_write_command(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    uint8_t full_cmd[6];
    full_cmd[0] = cmd | 0x40;
    full_cmd[1] = (arg >> 24) & 0xFF;
    full_cmd[2] = (arg >> 16) & 0xFF;
    full_cmd[3] = (arg >> 8) & 0xFF;
    full_cmd[4] = (arg >> 0) & 0xFF;
    full_cmd[5] = crc | 0x01;

    spi_write_blocking(SD_SPI, full_cmd, 6);
}

/// @brief Wait for and read a response from an SD card
/// @param sdbd_cfg 
/// @param data The data buffer to write the response to
/// @param len The leght of the incoming data
/// @return true if recieved a response, false if the response timed out
bool sdbd_spi_recieve_response(uint8_t *data, uint len)
{
    uint wait_bytes;
    for(wait_bytes = 0; wait_bytes < 10; wait_bytes++)
    {
        spi_read_blocking(SD_SPI, 0xFF, data, 1);
        if(data[0] != 0xFF)
            break;
    }
    if(wait_bytes > 8)
        return false;
    if(len > 1)
        spi_read_blocking(SD_SPI, 0xFF, &(data[1]), len - 1);
    return true;
}

bool sdbd_spi_send_recieve_command(uint8_t cmd, uint32_t arg, uint8_t *response, bool pull_cs_up)
{
    gpio_put(SD_CS, true);

    uint8_t crc = 0;
    if(cmd == 0)
        crc = 0x95;
    else if(cmd == 8)
        crc = 0x87;

    sd_spi_dummy_clocks(1);

    gpio_put(SD_CS, false);

    sdbd_spi_write_command(cmd, arg, crc);\

    bool ret_val = true;

    if(cmd == 8 || cmd == 58)
        ret_val = sdbd_spi_recieve_response(response, 5);
    else
        ret_val = sdbd_spi_recieve_response(response, 1);
    
    if(pull_cs_up)
        gpio_put(SD_CS, true);

    return ret_val;
}

void sdbd_spi_recieve_data_packet(uint8_t *data_token, uint8_t *data_block, bool full_block)
{
    *data_token = 0xFF;
    do
    {
        spi_read_blocking(SD_SPI, 0xFF, data_token, 1);
    } while (*data_token == 0xFF);

    if(full_block)
    {
        spi_read_blocking(SD_SPI, 0xFF, data_block, 512);
        sd_spi_dummy_clocks(2);
    }
    else
    {
        spi_read_blocking(SD_SPI, 0xFF, data_block, 16);
        sd_spi_dummy_clocks(2);
    }
}

void sdbd_spi_send_data_packet(uint8_t data_token, const uint8_t *data_block)
{
    spi_write_blocking(SD_SPI, &data_token, 1);
    spi_write_blocking(SD_SPI, data_block, 512);
    data_token = 0;
    spi_write_blocking(SD_SPI, &data_token, 1);
    spi_write_blocking(SD_SPI, &data_token, 1);
}

/// @brief Read CSD or CID on the SD card
/// @param sdbd_cfg 
/// @param data 16 bytes for the response
/// @param cmd true to read CSD, false to read CID
void sdbd_spi_read_card_register(uint8_t data[16], bool cmd)
{
    if(cmd)
    {
        uint8_t cmd9_res, data_token;
        sdbd_spi_send_recieve_command(9, 0, &cmd9_res, false);
        sdbd_spi_recieve_data_packet(&data_token, data, false);
        gpio_put(SD_CS, true);
    }
    else
    {
        uint8_t cmd10_res, data_token;
        sdbd_spi_send_recieve_command(10, 0, &cmd10_res, false);
        sdbd_spi_recieve_data_packet(&data_token, data, false);
        gpio_put(SD_CS, true);
    }
}


DSTATUS disk_initialize(BYTE pdrv)
{
    sta = STA_NOINIT;
    
    spi_init(SD_SPI, SD_INIT_BR);

    gpio_set_function(SD_CS, GPIO_FUNC_SIO);
    gpio_set_dir(SD_CS, true);
    gpio_set_drive_strength(SD_CS, GPIO_DRIVE_STRENGTH_8MA);
    gpio_put(SD_CS, true);

    gpio_set_function(SD_MISO, GPIO_FUNC_SPI);
    gpio_set_function(SD_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(SD_SCLK, GPIO_FUNC_SPI);

    gpio_set_pulls(SD_MISO, true, false);
    gpio_set_pulls(SD_MOSI, true, false);

    uint8_t init_response[5];

    int card_type = 0;

    sd_spi_dummy_clocks(12);

    if(!sdbd_spi_send_recieve_command(0, 0, init_response, true))
        return STA_NOINIT;

    if(init_response[0] != 0x01)
        return STA_NOINIT;
    

    if(sdbd_spi_send_recieve_command(8, 0x000001AA, init_response, true))
    {
        if(init_response[4] != 0xAA || init_response[3] != 0x01)
            return STA_NOINIT;
        
        
        uint timeout_time = time_us_32() + 1200000;

        do
        {
            sdbd_spi_send_recieve_command(55, 0, init_response, true);
            sdbd_spi_send_recieve_command(41, 0x40000000, init_response, true);
        } while(init_response[0] == 0x01 && time_us_32() < timeout_time);
        
        if(time_us_32() >= timeout_time)
            return STA_NOINIT;
        

        sdbd_spi_send_recieve_command(58, 0, init_response, true);
        if(init_response[1] & 0x40)
        {
            card_type = 1;
        }
        else
        {
            sdbd_spi_send_recieve_command(16, 0x00000200, init_response, true);
            card_type = 2;
        }
    }
    else
    {

        uint timeout_time = time_us_32() + 1200000;

        do
        {
            sdbd_spi_send_recieve_command(55, 0, &init_response[0], true);
            sdbd_spi_send_recieve_command(41, 0x40000000, &init_response[0], true);
        } while(init_response[0] == 0x01 && time_us_32() < timeout_time);
        
        if(time_us_32() >= timeout_time)
        {
            return STA_NOINIT;
        }

        sdbd_spi_send_recieve_command(16, 0x00000200, &init_response[0], true);
        card_type = 3;
        
    }


    // todo add something like this for SDXC and SDSC cards
    if(1 == card_type)
    {
        uint8_t csd_register[16];
        csd_register[7] = 0;
        csd_register[8] = 0;
        csd_register[9] = 0;
        sdbd_spi_read_card_register(csd_register, true);

        uint32_t c_size = (csd_register[7] << 16) | (csd_register[8] << 8) | csd_register[9];
        uint32_t block_count = c_size << 10;

    }
    else
    {
    }


    spi_set_baudrate(SD_SPI, SD_RUNNING_BR);

    sta = 0;

    return 0;
}

void sd_deinit()
{
    uint8_t res;
    sdbd_spi_send_recieve_command(0, 0, &res, true);

    spi_deinit(SD_SPI);
    gpio_set_function(SD_MISO, GPIO_FUNC_NULL);
    gpio_set_function(SD_MOSI, GPIO_FUNC_NULL);
    gpio_set_function(SD_SCLK, GPIO_FUNC_NULL);
    gpio_set_function(SD_CS, GPIO_FUNC_NULL);

    sta = STA_NOINIT;
}

DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count)
{

    if(pdrv != 0)
        return RES_PARERR;
    
    if(sta != 0)
        return RES_NOTRDY;

    if(count > 1)
    {
        uint8_t cmd_res = 0, data_token = 0;
        sdbd_spi_send_recieve_command(18, sector, &cmd_res, false);
        if(cmd_res != 0)
        {
            gpio_put(SD_CS, true);
            return RES_ERROR;
        }
        for(int i = 0; i < count; i++)
        {
            sdbd_spi_recieve_data_packet(&data_token, buff + i * FF_MIN_SS, true);
        }
        sdbd_spi_write_command(12, 0, 0);
        sd_spi_dummy_clocks(1);
        sdbd_spi_recieve_response(&cmd_res, 1);
        if(cmd_res != 0)
        {
            uint8_t miso_read = 0;
            do
            {
                spi_read_blocking(SD_SPI, 0xFF, &miso_read, 1);
            } while (miso_read < 0x0F);
            gpio_put(SD_CS, true);
            return RES_ERROR;
        }

        uint8_t miso_read = 0;
        do
        {
            spi_read_blocking(SD_SPI, 0xFF, &miso_read, 1);
        } while (miso_read < 0x0F);

        gpio_put(SD_CS, true);
        
    }
    else
    {
        uint8_t cmd_res, data_token;
        sdbd_spi_send_recieve_command(17, sector, &cmd_res, false);
        if(cmd_res != 0)
        {
            gpio_put(SD_CS, true);
            return RES_ERROR;
        }
        sdbd_spi_recieve_data_packet(&data_token, buff, true);
        if(cmd_res != 0)
        {
            gpio_put(SD_CS, true);
            return RES_ERROR;
        }
        gpio_put(SD_CS, true);
    }

    return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count)
{

    if(pdrv != 0)
        return RES_PARERR;
    
    if(sta != 0)
        return RES_NOTRDY;

    if(count > 1)
    {
        uint8_t cmd_res, data_res;
        sdbd_spi_send_recieve_command(25, sector, &cmd_res, false);
        if(cmd_res != 0)
        {
            gpio_put(SD_CS, true);
            return RES_ERROR;
        }
        for(int i = 0; i < count; i++)
        {
            uint8_t data_res;
            sdbd_spi_send_data_packet(0xFC, (BYTE *)(buff + i * FF_MIN_SS));
            spi_read_blocking(SD_SPI, 0xFF, &data_res, 1);
            uint8_t miso_read = 0;
            do
            {
                spi_read_blocking(SD_SPI, 0xFF, &miso_read, 1);
            } while (miso_read != 0xFF);
        }
        const uint8_t stop_trans = 0xFD;
        spi_write_blocking(SD_SPI, &stop_trans, 1);
        uint8_t miso_read;
        spi_read_blocking(SD_SPI, 0xFF, &miso_read, 1);
        miso_read = 0;
        do
        {
            spi_read_blocking(SD_SPI, 0xFF, &miso_read, 1);
        } while (miso_read != 0xFF);
    }
    else
    {
        uint8_t cmd_res = 0xFF, data_res = 0xFF;
        sdbd_spi_send_recieve_command(24, sector, &cmd_res, false);

        if(cmd_res != 0)
        {
            gpio_put(SD_CS, true);
            return RES_ERROR;
        }
        sdbd_spi_send_data_packet(0xFE, (BYTE *)buff);
        spi_read_blocking(SD_SPI, 0xFF, &data_res, 1);
        
        uint8_t miso_read = 0;
        do
        {
            spi_read_blocking(SD_SPI, 0xFF, &miso_read, 1);
        } while (miso_read != 0xFF);
        
        if((data_res & 0x1F) != 0x05)
        {
            gpio_put(SD_CS, true);
            return RES_ERROR;
        }
    }

    gpio_put(SD_CS, true);

    return RES_OK;
}



DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void* buff)
{
    if(pdrv != 0)
        return RES_PARERR;
    
    // sync is handled in disk_write
    if(CTRL_SYNC == cmd)
        return RES_OK;

    
    if(CTRL_EJECT == cmd)
    {
        sd_deinit();
        return RES_OK;
    }

    return RES_PARERR;
}