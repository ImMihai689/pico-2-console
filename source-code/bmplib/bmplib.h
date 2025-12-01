#ifndef BMPLIB_H
#define BMPLIB_H

#include "ff.h"

typedef struct {
    BYTE red_bits;
    BYTE blue_bits;
    BYTE green_bits;
    BYTE alpha_bits;
} COLOR_BITMASK;

typedef struct {
    WORD image_width;       // The image width in pixels
    WORD image_height;      // The image height in pixels
    COLOR_BITMASK bitmask;
    DWORD *pixel_data;
} BITMAP;

FRESULT bmp_read(BITMAP *bmpf, FIL *fil);

DWORD bmp_get_pixel(BITMAP *bmp, WORD x, WORD y);

#endif