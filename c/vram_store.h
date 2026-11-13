/* Video memory used as storage, as explicit native storage (docs/native.md, "Video memory as
 * storage"). The original keeps data that is not on screen in off-screen video memory and
 * reads some of it back into decisions or into game memory. Each such use is listed here; the
 * game's code writes the data here as well as to the VGA, and when video memory is not modelled
 * (vga_memory_off, production) reads it from here. Not game state: copies of what the game
 * wrote to video memory, as the VGA model's planes are.
 *
 * 1. The tile cache. The original keeps the level's 256 tile slots in off-screen video memory: slot t at
 * A000:5E80h + 32*t in each of the four planes (16x16 pixels, 2 bytes per row per plane).
 * LoadLevelTileSet fills the slots of level tiles, UploadSpriteTiles those taken from the
 * sprite sheet; ClassifyTileCache then reads the slots back through the VGA colour compare
 * to build TileClassTable and the mask table in DATA, which the game keeps using. So the
 * contents are game-relevant data, not only pixels. The uploads also write them here; with
 * video memory not modelled (vga_memory_off) ClassifyTileCache reads them from here, and the
 * native renderer takes the level's tile graphics from here.
 *
 * `generation` counts upload passes (a renderer's decoded tiles are stale when it changes).
 *
 * 2. The level stash. LoadLevel writes the level segment into A000:9DC0h with four map-mask
 * passes (one source pointer that wraps at 64 KiB: plane p holds bytes p*6240h.. of the
 * segment, modulo 10000h), loads the sprite sheet over the level segment, uploads its tiles,
 * then reads the four planes back into the level segment (the destination offset wraps too).
 * A workspace whose contents return to game memory.
 */
#ifndef VRAM_STORE_H
#define VRAM_STORE_H
#include <stdint.h>

#define TILE_CACHE_VRAM 0x5E80u
typedef struct TileCache {
    uint8_t slot[256][4][32];
    uint32_t generation;
} TileCache;
extern TileCache pre2_tile_cache;

/* A planar write of `v` at A000:off with map mask `planes` (bits 0-3), when off is a cache slot. */
static inline void tile_cache_write(uint16_t off, uint8_t planes, uint8_t v)
{
    uint16_t rel = (uint16_t)(off - TILE_CACHE_VRAM);
    if (rel >= 256u * 32u) return;
    for (int p = 0; p < 4; p++) if (planes & (1 << p)) pre2_tile_cache.slot[rel >> 5][p][rel & 31] = v;
}
/* The VGA read-mode-1 result for colour 0 with every plane compared: bit set where the pixel
 * is colour 0 in all four planes. */
static inline uint8_t tile_cache_colour0(uint16_t off)
{
    uint16_t rel = (uint16_t)(off - TILE_CACHE_VRAM);
    if (rel >= 256u * 32u) return 0xFF;
    const uint8_t (*s)[32] = pre2_tile_cache.slot[rel >> 5];
    return (uint8_t)~(s[0][rel & 31] | s[1][rel & 31] | s[2][rel & 31] | s[3][rel & 31]);
}

#define LEVEL_STASH_VRAM 0x9DC0u
#define LEVEL_STASH_BYTES 0x6240u
extern uint8_t pre2_level_stash[4][LEVEL_STASH_BYTES];
static inline uint8_t *level_stash_at(int plane, uint16_t off) { return &pre2_level_stash[plane & 3][(uint16_t)(off - LEVEL_STASH_VRAM)]; }
#endif
