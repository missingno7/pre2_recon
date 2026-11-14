/* Native screen renderer. No faithful image or live machine is an input. */
#ifndef PRE2_ENH_SCREENS_H
#define PRE2_ENH_SCREENS_H
#include "enh_render.h"
#include "../c/scene.h"

/* Immutable indexed source images. Capture shares unchanged 512-pixel blocks;
 * views and images remain valid until the next compositor_init(). */
#define SCREEN_PIXEL_BLOCK 512u
#define SCREEN_PIXEL_BLOCKS 1024u
typedef struct FrameScreenPixels {
    uint32_t size;
    const uint8_t *block[];
} FrameScreenPixels;

/* Capture integration: call observe AFTER the original handler for core/game
 * events; screen 400.. handlers capture their own operations. Call publish at
 * the end of compositor_publish, after the game view. Both live in draw_screens.c.
 * The supervisor wires these two hooks with the presenter integration. */
void draw_screens_observe(const SceneEvent *event);
void draw_screens_publish(Pre2Frame *frame, uint16_t latched_start);

/* Width 320..2048 at the authored height; high-resolution pictures retain their
 * own width/height (640 x 350/480). Palette components retain the six-bit format.
 * Integer pan equals the publication. Fractional indices use floor sampling;
 * a presenter can blend two adjacent renders in RGB for fractional filtering.
 * Returns 0, or -1 for invalid view/dimensions/buffer/coordinates. */
int enh_screens_render(const FrameScreenView *view,
                       const uint8_t palette[256][3], uint8_t pel_mask,
                       double pan_x, double pan_y, unsigned width, unsigned height,
                       EnhImage *out);
#endif
