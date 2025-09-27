#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "pico/types.h"

#include "ff.h"

// For memory efficency purpuses, sprites can be loaded with more or less bits per color.
// This defines:
// If a sprite is color or monochrome;
// How many bits is one pixel;
// If color, how the bits are distributed to red, green and blue (RGB, in order);
// How the base color is used, if it is used.
enum color_modes {
    CM_BW_1 = 0, // Monochrome, 1 bit per pixel
    CM_BW_2, // Monochrome, 2 bits per pixel
    CM_BW_4, // Monochrome, 4 bits per pixel
    CM_BW_1_T, // Monochrome, 1 bit per pixel, base color is transparent
    CM_BW_2_T, // Monochrome, 2 bits per pixel, base color is transparent
    CM_BW_4_T, // Monochrome, 4 bits per pixel, base color is transparent
    CM_CL_6, // Color, 6 bits per pixel, 222
    CM_CL_8_R, // Color, 8 bits per pixel, 233
    CM_CL_8_G, // Color, 8 bits per pixel, 323
    CM_CL_8_B, // Color, 8 bits per pixel, 332
    CM_CL_12, // Color, 12 bits per pixel, 444
    CM_CL_16, // Color, 16 bits per pixel, 565
    CM_CL_4_RG, // Color, 4 bits per pixel, 22x, x is the base color
    CM_CL_4_RB, // Color, 4 bits per pixel, 2x2, x is the base color
    CM_CL_4_GB, // Color, 4 bits per pixel, x22, x is the base color
    CM_CL_8_RG, // Color, 8 bits per pixel, 44x, x is the base color
    CM_CL_8_RB, // Color, 8 bits per pixel, 4x4, x is the base color
    CM_CL_8_GB, // Color, 8 bits per pixel, x44, x is the base color
    CM_CL_8_R_T, // Color, 8 bits per pixel, 233, base color is transparent
    CM_CL_8_G_T, // Color, 8 bits per pixel, 323, base color is transparent
    CM_CL_8_B_T, // Color, 8 bits per pixel, 332, base color is transparent
    CM_CL_12_T, // Color, 12 bits per pixel, 444, base color in 222 format is transparent
    CM_CL_16_T, // Color, 16 bits per pixel, 565, base color in 131 format is transparent
};

/// @brief Data structure for a 2D sprite
typedef struct {
    uint8_t size_x; // The horizontal size in pixels. Do not edit manually
    uint8_t size_y; // The vertical size in pixels. Do not edit manually
    uint8_t color_mode; // The color mode of the sprite. See color_modes for more info. Do not edit manually
    uint8_t base_color; // Used by some color modes, see color_modes for more info. Do not edit manually
    uint8_t *data; // The sprite data. Do not edit manually
} sprite_t;

#endif