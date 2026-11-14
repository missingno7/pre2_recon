/* Native screen renderer. No faithful image or live machine is an input. */
#ifndef PRE2_ENH_SCREENS_H
#define PRE2_ENH_SCREENS_H
#include "enh_render.h"
#include "../c/scene.h"

/* Immutable indexed source images. Capture shares unchanged 512-pixel blocks.
 * A publication is borrowed until the next publication or compositor_init().
 * Retain every copy kept beyond that point, and release it when discarded.
 * Retained views survive session reset; final release frees their last assets.
 * Retain/release are thread-safe; capture/publication remain single-producer. */
#define SCREEN_PIXEL_BLOCK 512u
#define SCREEN_PIXEL_BLOCKS 1024u
typedef struct FrameScreenPixels {
    uint32_t size;
    const uint8_t *block[];
} FrameScreenPixels;

/* Capture integration: call observe AFTER the original handler for core/game
 * events; screen 400.. handlers capture their own operations. Call publish at
 * the end of compositor_publish, after the game view. Both live in draw_screens.c.
 * The compositor wires these two hooks in production. */
void draw_screens_observe(const SceneEvent *event);
void draw_screens_publish(Pre2Frame *frame, uint16_t latched_start);

void enh_screen_view_retain(const FrameScreenView *view);
void enh_screen_view_release(const FrameScreenView *view);

/* Heap allocations owned by screen capture, including retained consumers.
 * Counts include allocation headers, exclude the fixed native working buffers.
 * Resetting peaks does not reset ownership or live counters. */
typedef struct EnhScreenMemoryStats {
    size_t live_bytes, peak_bytes, live_blocks, peak_blocks, live_images, live_layers;
    uint64_t allocations;
} EnhScreenMemoryStats;
EnhScreenMemoryStats enh_screen_memory_stats(void);
void enh_screen_memory_reset_peaks(void);

/* Capture-thread diagnostics; read/reset only while that producer is stopped. */
typedef struct EnhScreenObserveStats { uint64_t events, skipped, handoffs; } EnhScreenObserveStats;
EnhScreenObserveStats enh_screen_observe_stats(void);

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
