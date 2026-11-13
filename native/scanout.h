/* Display scan-out (native/scanout.c): the display controller's geometry and colour path,
 * from the logical register file (start address latched at the retrace, pitch, address
 * wrap, pel panning, display end, line compare, blanking, attribute palette) applied to a
 * pixel source. The result is an image of DAC indices: palette lookup is the presenter's.
 *
 * Sources: scan_vga (the VGA model's planes: the reference, used by tests) and the native
 * compositor's video store (native/compositor.h, production). Text modes are not presented
 * (the game uses text only when it exits): they scan as black. */
#ifndef SCANOUT_H
#define SCANOUT_H
#include <stdint.h>
#define SCAN_MAX_W 640
#define SCAN_MAX_H 480
/* Back-compatible names for the host's frame buffers. */
#define PRESENT_MAX_W SCAN_MAX_W
#define PRESENT_MAX_H SCAN_MAX_H

typedef struct ScanSource {
    uint8_t (*planar)(void *user, uint16_t addr, int dot);   /* 4-bit colour of dot 0..7 of byte addr */
    uint8_t (*chain4)(void *user, int plane, uint16_t off);  /* 256-colour byte */
    void *user;
} ScanSource;
extern const ScanSource scan_vga;

typedef struct ScanInfo { int w, h, blank, blank_black; } ScanInfo;
/* out: w*h DAC indices (at most SCAN_MAX_W*SCAN_MAX_H). */
void scan_indices(const ScanSource *src, uint16_t start, uint8_t *out, ScanInfo *info);

/* Reference XRGB of the VGA model (diagnostics). */
void present_vga(uint32_t *out, int *width, int *height);
void present_vga_start(uint32_t *out, int *width, int *height, uint16_t start);
#endif
