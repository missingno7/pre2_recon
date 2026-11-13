#include "draw_game_assets.h"
#include "../c/pre2.h"
#include "../c/vram_store.h"
#include <stdlib.h>
#include <string.h>

GameCel game_cels[1024];
uint8_t game_tiles[256][256];
uint8_t game_hud[65536], game_front[65536];

void game_assets_reset(void)
{
    for (int i = 0; i < 1024; i++) {
        free(game_cels[i].blocks); free(game_cels[i].pixels); free(game_cels[i].coverage);
    }
    memset(game_cels, 0, sizeof game_cels);
    memset(game_tiles, 0, sizeof game_tiles);
    memset(game_hud, 0, sizeof game_hud); memset(game_front, 0, sizeof game_front);
}

void game_assets_capture(void)
{
    game_assets_reset();
    for (unsigned o = 0; o < 65536; o++) {
        game_hud[o] = rd8(DW(DS_word_003D), (uint16_t)o);
        game_front[o] = rd8(DW(DS_word_003B), (uint16_t)o);
    }
    for (int i = 0; i < 1024; i++) {
        GameCel *c = &game_cels[i];
        uint16_t k = (uint16_t)(i * 2), wh = DW(DS_FrameWidths + k);
        if (!(uint8_t)wh) break;
        c->w = (uint8_t)wh; c->h = (uint8_t)(wh >> 8); c->row = (uint8_t)((c->w + 7) / 8);
        c->size = (uint32_t)c->row * c->h;
        /* Retain the complete source window: the original's unchecked source arithmetic
         * can read past a frame on clipped flash passes. Offsets wrap within the segment. */
        c->blocks = malloc(65536);
        c->pixels = malloc(c->size * 8); c->coverage = malloc(c->size * 8);
        if (!c->blocks || !c->pixels || !c->coverage) abort();
        uint16_t seg = DW(DS_FrameDataSegments + k), base = DW(DS_FrameDataOffsets + k);
        for (unsigned o = 0; o < 65536; o++) c->blocks[o] = rd8(seg, (uint16_t)(base + o));
        for (uint32_t o = 0; o < c->size; o++) for (int d = 0; d < 8; d++) {
            uint8_t bit = (uint8_t)(0x80 >> d), col = 0;
            for (int p = 0; p < 4; p++) if (c->blocks[o + (p + 1) * c->size] & bit) col |= (uint8_t)(1 << p);
            c->pixels[o * 8 + d] = col;
            c->coverage[o * 8 + d] = !!(c->blocks[o] & bit);
        }
    }
}
uint8_t game_cel_byte(const GameCel *c, uint16_t off)
{
    if (!c->blocks) return 0;
    if (off >= c->size * 5 || !c->size) return c->blocks[off];
    unsigned block = off / c->size, pixel = (off % c->size) * 8;
    uint8_t bits = 0;
    for (int x = 0; x < 8; x++) {
        int set = block ? !!(c->pixels[pixel + x] & (1 << (block - 1))) : c->coverage[pixel + x];
        if (set) bits |= (uint8_t)(0x80 >> x);
    }
    return bits;
}

void game_tiles_capture(void)
{
    for (int t = 0; t < 256; t++) for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) {
        uint8_t col = 0;
        for (int p = 0; p < 4; p++) col |= (uint8_t)(((pre2_tile_cache.slot[t][p][y * 2 + x / 8] >> (7 - x % 8)) & 1) << p);
        game_tiles[t][y * 16 + x] = col;
    }
}
