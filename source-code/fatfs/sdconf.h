
/*-----------------------------------------------------------------------\
|  Configuration for SDHC implementation                                 |
\-----------------------------------------------------------------------*/


#define SD_SPI spi0
// The SPI interface on the RP2040/RP2350 to use for communication

#define SD_CS 1U
#define SD_MISO 0U
#define SD_MOSI 3U
#define SD_SCLK 2U
// The pins to use to communicate to the SDHC card (must be compatible with the SPI function for the SD_SPI interface)

#define SD_CD 4U
// The chip detect pin for the SDHC card. Uncomment if you are using one, and replace 'x' with the pin it is on

#define NO_DMA
// Do not use DMA for the SPI transfers. Uncomment to use DMA for SPI transfers.
// In this implementation, using DMA has no practical effect other than taking up a DMA channel for transfers
// right now dma isn't even implemented lmao

#define SD_INIT_BR 200000
// The buadrate used when initialising the SDHC card (recommended 100kHz - 400kHz)

#define SD_RUNNING_BR 1000000
// The baudrate used when using the SDHC card normally (can be way higher than SD_INIT_BR, but I don't know how much, 1000000 is an example)