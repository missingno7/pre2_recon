/* VGA BIOS mode set (INT 10h AH=00h) on the VGA model, as the IBM VGA BIOS performs it for
 * the modes this game uses (03h text at exit, 0Dh game screens, 12h intro, 13h pictures).
 *
 * A mode set loads the mode's full register set (miscellaneous output, sequencer, CRTC with
 * the protect bit cleared first, graphics controller, attribute controller), loads the
 * default DAC palette of the mode and clears the display memory unless AL bit 7 is set.
 * Programs depend on this: the game's map screen reprograms CRTC 01h (display end), 11h,
 * 17h (MA13 replaced by the row-scan bit: an 8 KiB display ring) and the pixel panning
 * register, and relies on the next mode set to restore them and to clear the screen.
 * Register values: the VGA BIOS parameter tables (IBM VGA Technical Reference).
 * The BIOS data area writes belong to the caller (c/platform.c, tests' INT 10h handlers).
 */
#include "pre2.h"
#include "vga.h"
#include <string.h>

typedef struct {
    uint8_t mode, misc, seq[5], crtc[25], gc[9], attr[21];
} VgaModeTable;

static const VgaModeTable modes[] = {
    { 0x03, 0x67, {0x03, 0x00, 0x03, 0x00, 0x02},
      {0x5F,0x4F,0x50,0x82,0x55,0x81,0xBF,0x1F,0x00,0x4F,0x0D,0x0E,0x00,0x00,0x00,0x00,
       0x9C,0x8E,0x8F,0x28,0x1F,0x96,0xB9,0xA3,0xFF},
      {0x00,0x00,0x00,0x00,0x00,0x10,0x0E,0x00,0xFF},
      {0x00,0x01,0x02,0x03,0x04,0x05,0x14,0x07,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
       0x0C,0x00,0x0F,0x08,0x00} },
    { 0x0D, 0x63, {0x03, 0x09, 0x0F, 0x00, 0x06},
      {0x2D,0x27,0x28,0x90,0x2B,0x80,0xBF,0x1F,0x00,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,
       0x9C,0x8E,0x8F,0x14,0x00,0x96,0xB9,0xE3,0xFF},
      {0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x0F,0xFF},
      {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
       0x01,0x00,0x0F,0x00,0x00} },
    { 0x12, 0xE3, {0x03, 0x01, 0x0F, 0x00, 0x06},
      {0x5F,0x4F,0x50,0x82,0x54,0x80,0x0B,0x3E,0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,
       0xEA,0x8C,0xDF,0x28,0x00,0xE7,0x04,0xE3,0xFF},
      {0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x0F,0xFF},
      {0x00,0x01,0x02,0x03,0x04,0x05,0x14,0x07,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
       0x01,0x00,0x0F,0x00,0x00} },
    { 0x13, 0x63, {0x03, 0x01, 0x0F, 0x00, 0x0E},
      {0x5F,0x4F,0x50,0x82,0x54,0x80,0xBF,0x1F,0x00,0x41,0x00,0x00,0x00,0x00,0x00,0x00,
       0x9C,0x8E,0x8F,0x28,0x40,0x96,0xB9,0xA3,0xFF},
      {0x00,0x00,0x00,0x00,0x00,0x40,0x05,0x0F,0xFF},
      {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
       0x41,0x00,0x0F,0x00,0x00} },
};

/* The 64-colour EGA palette (rgbRGB bits) loaded for the 350/480-line 16-colour modes and
 * text; the 200-line modes get the CGA-compatible set, where bit 4 is the intensity of all
 * three guns and colour 6 is brown. */
static void dac_ega(int i, uint8_t *rgb)
{
    rgb[0] = (uint8_t)(((i >> 2) & 1) * 0x2A + ((i >> 5) & 1) * 0x15);
    rgb[1] = (uint8_t)(((i >> 1) & 1) * 0x2A + ((i >> 4) & 1) * 0x15);
    rgb[2] = (uint8_t)((i & 1) * 0x2A + ((i >> 3) & 1) * 0x15);
}
static void dac_cga(int i, uint8_t *rgb)
{
    int hi = (i >> 4) & 1;
    rgb[0] = (uint8_t)(((i >> 2) & 1) * 0x2A + hi * 0x15);
    rgb[1] = (uint8_t)(((i >> 1) & 1) * 0x2A + hi * 0x15);
    rgb[2] = (uint8_t)((i & 1) * 0x2A + hi * 0x15);
    if ((i & 0x17) == 0x06) rgb[1] = 0x15;               /* brown */
}

/* The VGA default 256-colour palette for mode 13h: the 16 CGA colours, 16 greys, then 216
 * entries of hue wheels (three intensities x three saturations x 24 hues), then black.
 * Entries 32..247 follow the documented structure, not a dump of a BIOS ROM; the game
 * loads its own palette before any mode 13h picture becomes visible. */
static void dac_vga256(int i, uint8_t *rgb)
{
    static const uint8_t grey[16] = {0x00,0x05,0x08,0x0B,0x0E,0x11,0x14,0x18,0x1C,0x20,0x24,0x28,0x2D,0x32,0x38,0x3F};
    if (i < 16) { dac_cga(((i & 8) << 1) | (i & 7), rgb); return; }
    if (i < 32) { rgb[0] = rgb[1] = rgb[2] = grey[i - 16]; return; }
    if (i >= 248) { rgb[0] = rgb[1] = rgb[2] = 0; return; }
    static const uint8_t top[3] = {0x3F, 0x1C, 0x10}, sat[3][3] = {{0x00, 0x1F, 0x2D}, {0x00, 0x0E, 0x14}, {0x00, 0x08, 0x0B}};
    int k = i - 32, group = k / 72, block = (k / 24) % 3, h = k % 24;
    uint8_t hi = top[group], lo = sat[group][block];
    int seg = h / 4, step = h % 4;
    uint8_t up = (uint8_t)(lo + (hi - lo) * step / 4), dn = (uint8_t)(hi - (hi - lo) * step / 4);
    uint8_t r, g, b;
    switch (seg) {                          /* blue -> magenta -> red -> yellow -> green -> cyan */
    case 0: r = up; g = lo; b = hi; break;
    case 1: r = hi; g = lo; b = dn; break;
    case 2: r = hi; g = up; b = lo; break;
    case 3: r = dn; g = hi; b = lo; break;
    case 4: r = lo; g = hi; b = up; break;
    default: r = lo; g = dn; b = hi; break;
    }
    rgb[0] = r; rgb[1] = g; rgb[2] = b;
}

EXPORT void vga_bios_mode_set(uint8_t al)
{
    VgaState *v = &pre2_vga;
    uint8_t mode = al & 0x7F;
    const VgaModeTable *t = 0;
    for (unsigned i = 0; i < sizeof modes / sizeof modes[0]; i++) if (modes[i].mode == mode) t = &modes[i];
    if (!t) return;                                  /* not a mode this game uses */
    v->misc_output = t->misc;
    memcpy(v->seq, t->seq, 5);
    memcpy(v->crtc, t->crtc, 25);
    memcpy(v->gc, t->gc, 9);
    memcpy(v->attr, t->attr, 21);
    v->attr_index = 0x20;                            /* palette address source: display on */
    v->attr_flipflop = 0;
    v->pel_mask = 0xFF;
    for (int i = 0; i < 256; i++) {
        uint8_t *rgb = v->dac[i];
        if (mode == 0x13) dac_vga256(i, rgb);
        else if (mode == 0x0D) { if (i < 64) dac_cga(i, rgb); else rgb[0] = rgb[1] = rgb[2] = 0; }
        else { if (i < 64) dac_ega(i, rgb); else rgb[0] = rgb[1] = rgb[2] = 0; }
    }
    v->dac_write_index = v->dac_read_index = 0;
    v->dac_component = v->dac_read_component = 0;
    if (!(al & 0x80)) {                              /* clear display memory */
        if (mode == 0x03) {                          /* text: blanks with attribute 07h */
            memset(v->plane, 0, sizeof v->plane);
            memset(v->plane[0], 0x20, 0x8000);
            memset(v->plane[1], 0x07, 0x8000);
        } else memset(v->plane, 0, sizeof v->plane);
    }
    memset(v->latch, 0, sizeof v->latch);
}
