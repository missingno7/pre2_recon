/* Capture-private seam; never used by a presenter. */
#ifndef PRE2_ENH_CAPTURE_H
#define PRE2_ENH_CAPTURE_H
#include "enh_render.h"
void enh_capture_reset(void);
void enh_capture_cels(void);
void enh_capture_backdrop(uint16_t segment);
void enh_capture_level(uint32_t epoch);
void enh_capture_view(FrameGameView *view);
void enh_capture_publish(FrameGameView *view, uint16_t start);
void enh_capture_tile(uint16_t destination, uint8_t tile, uint8_t cls);
void enh_capture_restore(uint16_t source, uint16_t destination);
void enh_capture_panel(FrameGameView *view, unsigned panel);
void enh_capture_hud(unsigned kind, unsigned index, uint16_t destination);
void enh_capture_hud_copy(uint16_t source, uint16_t destination, unsigned width, unsigned rows, unsigned ss, unsigned ds);
void enh_capture_hud_clear(uint16_t destination, unsigned width, unsigned rows, unsigned stride);
void enh_capture_surface_ring(unsigned page);
void enh_capture_surface_clear(FrameGameView pages[2], uint16_t destination, unsigned width, unsigned rows, unsigned stride);
void enh_capture_surface_copy(FrameGameView pages[2], uint16_t source, uint16_t destination, unsigned width, unsigned rows);
void enh_capture_surface_blackout(FrameGameView pages[2], unsigned x, unsigned width, unsigned y, uint16_t page);
void enh_capture_surface_iris(FrameGameView pages[2], uint16_t page, int cx, int cy, unsigned radius);
void enh_capture_surface_sprites(unsigned page);
void enh_capture_surface_sprite(unsigned page, const FrameSprite *sprite);
void enh_capture_surface_publish(FrameGameView *view, uint16_t start);
void enh_capture_surface_hide(void);
#endif
