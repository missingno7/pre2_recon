/* VGA presentation: the modelled VGA state (c/vga.h) -> RGBA, scanned out the way the VGA
 * CRT controller and attribute controller do it, driven only by register state:
 *
 *   - geometry: horizontal display end (CRTC 01h) in character clocks of 8 dots; the
 *     256-colour mode (attribute 10h bit 6) joins two dots per pixel. Vertical display end
 *     (CRTC 12h + overflow 07h bits 1/6) in scan lines, divided by the character height
 *     (CRTC 09h bits 0-4 + 1) and scan doubling (CRTC 09h bit 7). The canvas is 320 or 640
 *     wide; columns beyond the display end show the overscan colour (attribute 11h), so a
 *     program that narrows the display (the map screen sets CRTC 01h = 26h) keeps a stable
 *     picture with a border, as on a monitor.
 *   - addressing: start address CRTC 0Ch:0Dh, row pitch CRTC 13h * 2 address units; byte
 *     mode (CRTC 17h bit 6), word mode, doubleword mode (CRTC 14h bit 6). CRTC 17h bit 0
 *     clear replaces address bit 13 by row-scan-counter bit 0, bit 1 clear replaces bit 14
 *     by row-scan bit 1 (CGA/Hercules-compatible wrap: with one scan line per row this
 *     wraps the display at 8 KiB, which the map screen uses).
 *   - line compare (CRTC 18h + overflow): the address counter restarts at 0 from that line;
 *     attribute 10h bit 5 then also suppresses panning below it.
 *   - pixel panning: attribute 13h (0..7 pixels; half pixels in 256-colour mode).
 *   - colour: 16-colour planar: bit p from plane p -> attribute palette register (6 bits)
 *     -> DAC index with the colour select bits (attribute 14h; attribute 10h bit 7 selects
 *     which bits) -> PEL mask -> DAC. 256-colour (chain 4): the byte -> PEL mask -> DAC.
 *   - blanking: sequencer 01h bit 5 (screen off) or the attribute address PAS bit clear.
 * Chain-4 data are stored by the model at plane (address & 3), offset (address >> 2), so a
 * doubleword-mode address counter c reads offset c of each plane.
 * Text modes are not presented (the game uses text only when it exits).
 */
#include "../c/pre2.h"
#include "../c/vga.h"
#include "present.h"

static uint8_t expand6(uint8_t v) { return (uint8_t)((v << 2) | (v >> 4)); }

static uint32_t dac_rgb(const VgaState *v, uint8_t idx)
{
    idx &= v->pel_mask;
    return 0xFF000000u | (uint32_t)expand6(v->dac[idx][0]) << 16 | (uint32_t)expand6(v->dac[idx][1]) << 8 |
           expand6(v->dac[idx][2]);
}

static uint8_t attr_to_dac(const VgaState *v, int c)
{
    uint8_t a = v->attr[c & 15] & 0x3F;
    if (v->attr[0x10] & 0x80) return (uint8_t)((a & 0x0F) | ((v->attr[0x14] & 0x0F) << 4));
    return (uint8_t)(a | ((v->attr[0x14] & 0x0C) << 4));
}

void present_vga(uint32_t *out, int *width, int *height)
{
    present_vga_start(out, width, height, (uint16_t)(pre2_vga.crtc[0x0C] << 8 | pre2_vga.crtc[0x0D]));
}

/* `start`: the start address the CRTC latched for this frame (at the vertical retrace). */
void present_vga_start(uint32_t *out, int *width, int *height, uint16_t start)
{
    const VgaState *v = &pre2_vga;
    int text = !(v->gc[6] & 1);                       /* graphics controller misc: alphanumeric */
    int mode256 = v->attr[0x10] & 0x40;
    int hde = v->crtc[0x01] + 1;
    int dots = hde * 8;
    int vis_w = mode256 ? dots / 2 : dots;
    int vde = (v->crtc[0x12] | ((v->crtc[0x07] & 0x02) << 7) | ((v->crtc[0x07] & 0x40) << 3)) + 1;
    int char_h = (v->crtc[0x09] & 0x1F) + 1;
    int dbl = (v->crtc[0x09] & 0x80) ? 2 : 1;
    int group = dbl * (char_h <= 2 ? char_h : 1);      /* physical scan lines per output row */
    int rows = vde / group;
    int lc = v->crtc[0x18] | ((v->crtc[0x07] & 0x10) << 4) | ((v->crtc[0x09] & 0x40) << 3);
    int w = vis_w <= 320 ? 320 : 640, h = rows <= 200 ? 200 : (rows <= 350 ? 350 : 480);
    if (w > PRESENT_MAX_W) w = PRESENT_MAX_W;
    if (h > PRESENT_MAX_H) h = PRESENT_MAX_H;
    *width = w; *height = h;
    uint32_t overscan = dac_rgb(v, attr_to_dac(v, v->attr[0x11] & 15));
    int blank = (v->seq[1] & 0x20) || !(v->attr_index & 0x20) || text;
    if (blank) {
        uint32_t c = text ? 0xFF000000u : overscan;
        for (int i = 0; i < w * h; i++) out[i] = c;
        return;
    }
    uint32_t pal[16];
    for (int i = 0; i < 16; i++) pal[i] = dac_rgb(v, attr_to_dac(v, i));
    uint16_t pitch = (uint16_t)(v->crtc[0x13] * 2);
    int dword = v->crtc[0x14] & 0x40;
    int byte_mode = v->crtc[0x17] & 0x40;
    int pan = v->attr[0x13] & 7;
    int chain4 = v->seq[4] & 0x08;
    uint16_t row_base = start;
    int row_scan = 0;
    int y = 0;
    for (int scan = 0; scan < vde && y < h; scan++) {
        if (scan == lc) { row_base = 0; row_scan = 0; }
        int first_of_row = (scan % group) == 0;       /* output the first scan line of each group */
        if (first_of_row) {
            uint32_t *line = out + y * w;
            int p = (scan >= lc && (v->attr[0x10] & 0x20)) ? 0 : pan;
            for (int x = 0; x < w; x++) {
                if (x >= vis_w) { line[x] = overscan; continue; }
                int px = mode256 ? x * 2 + p : x + p;  /* in dots */
                uint16_t counter = (uint16_t)(row_base + px / 8);
                uint16_t addr = counter;
                if (dword) addr = (uint16_t)(counter << 2);
                else if (!byte_mode) addr = (uint16_t)((counter << 1) | ((counter >> ((v->crtc[0x17] & 0x20) ? 15 : 13)) & 1));
                if (!(v->crtc[0x17] & 0x01)) addr = (uint16_t)((addr & ~0x2000) | ((row_scan & 1) << 13));
                if (!(v->crtc[0x17] & 0x02)) addr = (uint16_t)((addr & ~0x4000) | ((row_scan & 2) << 13));
                if (mode256) {
                    uint16_t off = chain4 && dword ? (uint16_t)(addr >> 2) : addr;
                    line[x] = dac_rgb(v, v->plane[(px / 2) & 3][off]);
                } else {
                    int bit = 7 - (px & 7), c = 0;
                    for (int q = 0; q < 4; q++) c |= ((v->plane[q][addr] >> bit) & 1) << q;
                    line[x] = pal[c];
                }
            }
            y++;
        }
        /* the row scan counter advances once per scan line (twice as slowly when doubling) */
        if (dbl == 1 || (scan & 1)) {
            if (++row_scan >= char_h) { row_scan = 0; row_base = (uint16_t)(row_base + pitch); }
        }
    }
    for (; y < h; y++) for (int x = 0; x < w; x++) out[y * w + x] = overscan;
}
