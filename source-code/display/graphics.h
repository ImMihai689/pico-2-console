#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "pico/types.h"

typedef bool rgb_mode;

// A 565-bit RGB mode bitmap
typedef struct
{
    int16_t x0; // Rect coordinate
    int16_t y0; // Rect coordinate
    int16_t x1; // Rect coordinate
    int16_t y1; // Rect coordinate
    uint32_t pixels; // The number of pixels in the bitmap
    uint16_t *data; // The pixel array
} bitmap_565_t;



#endif