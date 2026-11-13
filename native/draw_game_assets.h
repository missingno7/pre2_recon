/* Native-owned gameplay assets, invalidated by their original loader events. */
#ifndef DRAW_GAME_ASSETS_H
#define DRAW_GAME_ASSETS_H
#include <stdint.h>
typedef struct GameCel {
    uint8_t w, h, row;
    uint32_t size;
    uint8_t *blocks;                 /* coverage followed by four source colour blocks */
    uint8_t *pixels, *coverage;
} GameCel;
extern GameCel game_cels[1024];
extern uint8_t game_tiles[256][256];
extern uint8_t game_hud[65536], game_front[65536];
void game_assets_reset(void);
void game_assets_capture(void);
void game_tiles_capture(void);
uint8_t game_cel_byte(const GameCel *c, uint16_t off);
#endif
