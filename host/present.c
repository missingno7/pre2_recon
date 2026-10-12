/* VGA presentation: the modelled VGA state (c/vga.h) -> 320x200 RGBA, as a 16-colour
 * planar display (BIOS mode 0Dh, the game's mode) scans it out.
 *
 * Scan-out: start address = CRTC 0Ch:0Dh, line pitch = CRTC 13h * 2 bytes (40 for mode
 * 0Dh), horizontal pixel panning = attribute 13h (0..7). Each pixel's colour index takes
 * bit p from plane p; the attribute palette (attribute 0..15) maps it to a DAC entry
 * (with colour select bits 4..7 from attribute 14h when attribute 10h bit 7 is set,
 * otherwise bits 4..5 from attribute 14h bits 0..1 as on the VGA), and the 6-bit DAC
 * triple is expanded to 8 bits. Mode 0Dh doubles scan lines in hardware; the 200 visible
 * lines are produced here directly. Video output disabled (attribute index bit 5 clear)
 * shows black.
 */
#include "../c/pre2.h"
#include "../c/vga.h"
#include "present.h"

static uint8_t expand6(uint8_t v) { return (uint8_t)((v << 2) | (v >> 4)); }

void present_vga(uint32_t *out, int width, int height)
{
    const VgaState *v = &pre2_vga;
    uint16_t start = (uint16_t)(v->crtc[0x0C] << 8 | v->crtc[0x0D]);
    uint16_t pitch = (uint16_t)(v->crtc[0x13] * 2);
    if (!pitch) pitch = 40;
    int pan = v->attr[0x13] & 7;
    uint32_t pal[16];
    for (int i = 0; i < 16; i++) {
        uint8_t a = v->attr[i] & 0x3F;
        uint8_t dac = (v->attr[0x10] & 0x80) ? (uint8_t)((a & 0x0F) | ((v->attr[0x14] & 0x0F) << 4))
                                             : (uint8_t)(a | ((v->attr[0x14] & 0x0C) << 4));
        dac &= v->pel_mask;
        pal[i] = 0xFF000000u | (uint32_t)expand6(v->dac[dac][0]) << 16 |
                 (uint32_t)expand6(v->dac[dac][1]) << 8 | expand6(v->dac[dac][2]);
    }
    int enabled = v->attr_index & 0x20;
    for (int y = 0; y < height; y++) {
        uint16_t line = (uint16_t)(start + y * pitch);
        for (int x = 0; x < width; x++) {
            if (!enabled) { out[y * width + x] = 0xFF000000u; continue; }
            int px = x + pan;
            uint16_t off = (uint16_t)(line + (px >> 3));
            int bit = 7 - (px & 7);
            int c = 0;
            for (int p = 0; p < 4; p++) c |= ((v->plane[p][off] >> bit) & 1) << p;
            out[y * width + x] = pal[c];
        }
    }
}
