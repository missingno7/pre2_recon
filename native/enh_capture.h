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
#endif
