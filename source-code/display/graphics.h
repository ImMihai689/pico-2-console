#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "pico/types.h"

#include "ff.h"

/// @brief A 16x16 12-bit color tile.
/// Used by tilesets and sprites
typedef struct {
    uint8_t pixel_data[384];
} tile_t;

/// @brief A set of tiles that can be shown on the screen.
/// The empty space in a tileset is dinamically changed to process sprites.
/// @note tilesets are used by sprites to render backgrounds where the sprite would be invisible
typedef struct {
    /// @brief How many tiles the set has.
    uint8_t tile_count;
    tile_t *tiles;
} tileset_t;

/// @brief Describes how to draw tiles on a screen.
/// @note A tilemap is always the size of the screen.
typedef struct {
    /// @brief Reference to the tileset where the tile comes from.
    tileset_t *tileset_origin[300];
    /// @brief The index in the tileset of this tile.
    uint8_t tile_index[300];
} tilemap_t;

/// @brief Sprites can be drawn at any position on the screen on top of the tilemap,
/// using bright pink (15, 0, 15) as the transparent color.
typedef struct {
    // The X and Y size of the tile, in tiles
    // [7:4]    the X size of the tile
    // [3:0]    the Y size of the tile
    uint8_t size;
    // The tiles of the sprite, ordered top to bottom, then left to right
    tile_t *tiles;
} bitmap_t;

#endif