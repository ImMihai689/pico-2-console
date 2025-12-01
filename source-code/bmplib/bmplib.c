#include "bmplib.h"
#include <stdio.h>
#include "pico/stdlib.h"

#define extract_color(col, size, off, new) (((col >> off) & ((1 << size) - 1)) >> (size - new))

// Read a bitmask into an offset (the number of 0s to the right) and a size (number of 1s in the middle)
void bitmask_size(DWORD bitmask, BYTE *offset, BYTE *size)
{
    *offset = 0;
    *size = 0;

    if((bitmask & 0xFFFF) == 0) { *offset += 16; bitmask >>= 16; }
    if((bitmask & 0xFF) == 0) { *offset += 8; bitmask >>= 8; }
    if((bitmask & 0xF) == 0) { *offset += 4; bitmask >>= 4; }
    if((bitmask & 0x3) == 0) { *offset += 2; bitmask >>= 2; }
    if((bitmask & 0x1) == 0) { *offset += 1; bitmask >>= 1; }

    bitmask = ~bitmask;

    if((bitmask & 0xFFFF) == 0) { *size += 16; bitmask >>= 16; }
    if((bitmask & 0xFF) == 0) { *size += 8; bitmask >>= 8; }
    if((bitmask & 0xF) == 0) { *size += 4; bitmask >>= 4; }
    if((bitmask & 0x3) == 0) { *size += 2; bitmask >>= 2; }
    if((bitmask & 0x1) == 0) { *size += 1; bitmask >>= 1; }
}

// Resize a color from one depth to another
DWORD resize_color(DWORD color, BYTE depth, BYTE new_depth)
{
    if(new_depth < depth)
        return color >> (depth - new_depth);

    return color << (new_depth - depth);
}

FRESULT bmp_read(BITMAP *bmpf, FIL *fil)
{
    if(fil->fptr != 0)
        return 20;
    
    DWORD read_data = 0;
    UINT read_bytes = 0;
    FRESULT fr;
    UINT header_size = 0;
    DWORD pixel_array_offset = 0;

    BYTE red_mask_offset = 0, red_mask_size = 0;
    BYTE green_mask_offset = 0, green_mask_size = 0;
    BYTE blue_mask_offset = 0, blue_mask_size = 0;
    BYTE alpha_mask_offset = 0, alpha_mask_size = 0;

    BYTE bits_per_pixel;     // The number of bits that represent one pixel
    WORD color_tabe_size;    // The number of colors in the color table

    // Parse the header

    fr = f_read(fil, &read_data, 2, &read_bytes);   // Read file signature
    if(fr != FR_OK)
        return fr;
    if((read_data & 0xFFFF) != 0x4D42)
        return FR_INVALID_OBJECT;
    
    
    fil->fptr = 0x0A;                                // Skip over file size and unused bytes

    fr = f_read(fil, &read_data, 4, &read_bytes);    // Read pixel array offset
    if(fr != FR_OK)
        return fr;
    pixel_array_offset = read_data;
    
    fr = f_read(fil, &read_data, 4, &read_bytes);    // Read DIB header size
    if(fr != FR_OK)
        return fr;
    header_size = read_data;

    if(header_size != 108 && header_size != 124)
        return FR_INVALID_OBJECT;
    
    fr = f_read(fil, &read_data, 4, &read_bytes);    // Read image width
    if(fr != FR_OK)
        return fr;
    bmpf->image_width = read_data;

    fr = f_read(fil, &read_data, 4, &read_bytes);    // Read image height
    if(fr != FR_OK)
        return fr;
    bmpf->image_height = read_data;

    fil->fptr += 2;                                  // Skip number of planes

    fr = f_read(fil, &read_data, 2, &read_bytes);    // Read bits per pixel
    if(fr != FR_OK)
        return fr;
    bits_per_pixel = read_data;
    
    fr = f_read(fil, &read_data, 4, &read_bytes);    // Read compression
    if(fr != FR_OK)
        return fr;
    if(read_data != 3)
        return FR_INVALID_OBJECT;
    
    fil->fptr += 4;                                  // Skip bitmap data size (including padding)

    fil->fptr += 4;
    fil->fptr += 4;                                  // Skip pixels/metre (both X and Y)

    fr = f_read(fil, &read_data, 4, &read_bytes);    // Read color table size
    if(fr != FR_OK)
        return fr;
    color_tabe_size = read_data;

    fil->fptr += 4;                                  // Skip important color count

    fr = f_read(fil, &read_data, 4, &read_bytes);    // Read red channel bitmask
    if(fr != FR_OK)
        return fr;
    bitmask_size(read_data, &(red_mask_offset), &(red_mask_size));

    fr = f_read(fil, &read_data, 4, &read_bytes);    // Read green channel bitmask
    if(fr != FR_OK)
        return fr;
    bitmask_size(read_data, &(green_mask_offset), &(green_mask_size));

    fr = f_read(fil, &read_data, 4, &read_bytes);    // Read blue channel bitmask
    if(fr != FR_OK)
        return fr;
    bitmask_size(read_data, &(blue_mask_offset), &(blue_mask_size));

    fr = f_read(fil, &read_data, 4, &read_bytes);    // Read alpha channel bitmask
    if(fr != FR_OK)
        return fr;
    bitmask_size(read_data, &(alpha_mask_offset), &(alpha_mask_size));

    // Read the actual image data

    fil->fptr = pixel_array_offset;

    DWORD bytes_per_pixel = bits_per_pixel >> 3;
    DWORD padding_bytes = 4 - (((bits_per_pixel >> 3) * bmpf->image_width) & 0b11);
    if(padding_bytes == 4)
        padding_bytes = 0;
    
    DWORD bit = 0;
    DWORD i = 0;

    bmpf->pixel_data[0] = 0;

    for(int y = bmpf->image_height - 1; y >= 0; y--)
    {
        for(int x = 0; x < bmpf->image_width; x++)
        {
            fr = f_read(fil, &read_data, bytes_per_pixel, &read_bytes);
            if(fr)
                return fr;
            
            DWORD color = 0;
            {
                BYTE inc = 0;
                color = // ? gl gng
                extract_color(read_data, red_mask_size, red_mask_offset, bmpf->bitmask.red_bits) |
                extract_color(read_data, green_mask_size, green_mask_offset, bmpf->bitmask.green_bits) << (inc += bmpf->bitmask.red_bits) |
                extract_color(read_data, blue_mask_size, blue_mask_offset, bmpf->bitmask.blue_bits) << (inc += bmpf->bitmask.green_bits) |
                extract_color(read_data, alpha_mask_size, alpha_mask_offset, bmpf->bitmask.alpha_bits) << (inc += bmpf->bitmask.blue_bits);
            }

            BYTE bitppixel = bmpf->bitmask.red_bits + bmpf->bitmask.green_bits + bmpf->bitmask.blue_bits + bmpf->bitmask.alpha_bits;

            bmpf->pixel_data[i] |= color << bit;
            bit += bitppixel;
            if(bit > 31)
            {
                bit -= 32;
                i++;
                bmpf->pixel_data[i] = color >> (bitppixel - bit);
            }
        }
        if(padding_bytes)
        {
            f_read(fil, &read_data, padding_bytes, &read_bytes);
            if(fr)
                return fr;
        }

    }

    return FR_OK;
}


DWORD bmp_get_pixel(BITMAP *bmp, WORD x, WORD y)
{
    y = bmp->image_height - y - 1;

    BYTE bits_per_pixel = 
    bmp->bitmask.red_bits +
    bmp->bitmask.green_bits +
    bmp->bitmask.blue_bits +
    bmp->bitmask.alpha_bits;

    DWORD i = ((y * bmp->image_width + x) * bits_per_pixel);
    BYTE bit = (i & 0x1F);
    i = i >> 5;

    QWORD color = 0;

    color = ((QWORD)bmp->pixel_data[i]) | (((QWORD)bmp->pixel_data[i + 1]) << 32);

    color >>= bit;
    color &= (1 << bits_per_pixel) - 1;

    return (DWORD)color;
}