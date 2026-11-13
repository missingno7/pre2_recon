/* Screens drawing family of the native compositor (see compositor.h, c/scene_screens.h). */
#include "compositor.h"
#include "../c/scene_screens.h"
#include "../c/pre2.h"
#include <string.h>

/* Captures are owned copies: the callback background is overwritten by shifted fonts,
 * PRESENT reuses its palette, and the intro loads its second pair over the first. */
static uint8_t picture[4 * 65536], fonts[65536], world[65536], glyph[48];
static uint32_t font_epoch, world_epoch;
static FrameTransition transition;

static void plane_byte(uint16_t dst, unsigned plane, uint8_t bits)
{
    uint8_t mask = (uint8_t)(1u << plane);
    for (int d = 0; d < 8; d++) {
        uint8_t *p = comp_px(dst, d);
        *p = (uint8_t)((*p & ~mask) | ((bits & (0x80u >> d)) ? mask : 0));
    }
}

static void copy_byte(uint16_t src, uint16_t dst)
{
    /* One whole indexed byte is the semantic latch-copy unit. */
    uint8_t dots[8];
    memcpy(dots, comp_px(src, 0), 8);
    memcpy(comp_px(dst, 0), dots, 8);
}

static void picture_event(const SceneEvent *e)
{
    uint16_t seg = (uint16_t)e->a[0], src = (uint16_t)e->a[1], dst = (uint16_t)e->a[2];
    unsigned n = (unsigned)e->a[3], mask = (unsigned)e->a[4], k = 0;
    if (e->a[5]) {
        for (unsigned i = 0; i < n; i++) picture[i] = rd8(seg, (uint16_t)(src + i));
        for (unsigned i = 0; i < n; i++) {
            uint16_t a = (uint16_t)(dst + i);
            comp_chain4[a & 3][a >> 2] = picture[i];
        }
    } else {
        for (unsigned p = 0; p < 4; p++) if (mask & (1u << p)) {
            for (unsigned i = 0; i < n; i++) picture[k + i] = rd8(seg, (uint16_t)(src + k + i));
            for (unsigned i = 0; i < n; i++) plane_byte((uint16_t)(dst + i), p, picture[k + i]);
            k += n;
        }
    }
}

static void clear_event(const SceneEvent *e)
{
    for (unsigned i = 0; i < (unsigned)e->a[1]; i++)
        memset(comp_px((uint16_t)(e->a[0] + i), 0), 0, 8);
}

static void copy_event(const SceneEvent *e)
{
    for (unsigned i = 0; i < (unsigned)e->a[2]; i++)
        copy_byte((uint16_t)(e->a[0] + i), (uint16_t)(e->a[1] + i));
}

static void char_event(const SceneEvent *e)
{
    uint8_t c = (uint8_t)e->a[1], g = (uint8_t)(c - 0x30);
    if (g > 9) g = (uint8_t)(g - 2);
    for (unsigned i = 0; i < sizeof glyph; i++)
        glyph[i] = c == 0x20 ? 0 : rd8((uint16_t)e->a[0], (uint16_t)(g * 48u + i));
    for (unsigned p = 0; p < 4; p++) for (unsigned r = 0; r < 12; r++)
        plane_byte((uint16_t)(e->a[2] + r * 40), p, glyph[p * 12 + r]);
}

static void fonts_event(const SceneEvent *e)
{
    if (!++font_epoch) font_epoch = 1;
    for (unsigned i = 0; i < sizeof fonts; i++) fonts[i] = rd8((uint16_t)e->a[0], (uint16_t)i);
}

static void string_event(const SceneEvent *e)
{
    if (!font_epoch) return;
    uint16_t bx = (uint16_t)e->a[0], mask = (uint16_t)e->a[1], cursor = (uint16_t)e->a[2];
    for (;;) {
        uint8_t c = rd8(DATA_SEG, bx++), g;
        if (c == 0x20) g = 0x2B;
        else {
            if (c < 0x30) return;
            g = (uint8_t)(c - 0x30);
            if (g > 9) g = (uint8_t)(g - 7);
        }
        uint16_t src = (uint16_t)(g * 0x60u + e->a[4] + 6);
        cursor = (uint16_t)(cursor + e->a[3]);
        uint16_t old = (uint16_t)(cursor + 0x50 + e->a[6]);
        uint16_t draw = (uint16_t)(cursor + 0x50 + e->a[5]);
        for (unsigned r = 0; r < 12; r++) {
            old &= mask;
            for (unsigned b = 0; b < 3; b++) for (unsigned p = 2; p < 4; p++)
                plane_byte((uint16_t)(old + b), p, 0);
            old = (uint16_t)(old + 40);
        }
        for (unsigned p = 2; p < 4; p++) {
            for (unsigned r = 0; r < 12; r++) {
                draw &= mask;
                for (unsigned b = 0; b < 3; b++) plane_byte((uint16_t)(draw + b), p, fonts[src++]);
                draw = (uint16_t)(draw + 40);
            }
            src = (uint16_t)(src + 12);
            draw = (uint16_t)(draw - 480);
        }
    }
}

static void world_event(const SceneEvent *e)
{
    if (!++world_epoch) world_epoch = 1;
    for (unsigned i = 0; i < sizeof world; i++) world[i] = rd8((uint16_t)e->a[0], (uint16_t)i);
}

static void world_column(const SceneEvent *e)
{
    if (!world_epoch) return;
    uint16_t col = (uint16_t)(((uint16_t)e->a[0] >> 3) - 1), src = col, dst = col;
    for (unsigned p = 0; p < 4; p++) {
        for (unsigned r = 0; r < 200; r++) {
            dst &= 0x1FFF;
            if ((unsigned)e->a[1] & (1u << p)) plane_byte(dst, p, world[src]);
            src = (uint16_t)(src + 80);
            dst = (uint16_t)(dst + 40);
        }
        dst = (uint16_t)(dst - 8000);
    }
}

static void map_edges(const SceneEvent *e)
{
    uint16_t start = (uint16_t)e->a[0], src, dst;
    if ((e->a[1] & 8) != (e->a[2] & 8)) {
        src = (uint16_t)(start + 39); dst = (uint16_t)(src - 40);
        for (unsigned r = 0; r < 200; r++) {
            src &= 0x1FFF; dst &= 0x1FFF;
            copy_byte(src, dst);
            src = (uint16_t)(src + 40); dst = (uint16_t)(dst + 40);
        }
    }
    uint16_t dy = (uint16_t)(e->a[4] - e->a[3]);
    if (!dy) return;
    if (!(dy & 0x8000)) {
        src = (uint16_t)(start - 1); dst = (uint16_t)(src + 0x1F18);
    } else {
        dy = (uint16_t)-dy;
        dst = (uint16_t)(start - 1 - 40 * (uint8_t)dy); src = (uint16_t)(dst + 0x1F18);
    }
    uint16_t n = (uint16_t)(41 * (uint8_t)dy);
    /* LOOP with zero count wraps and performs 65536 copies. */
    do {
        src &= 0x1FFF; dst &= 0x1FFF;
        copy_byte(src, dst); src++; dst++;
    } while (--n);
}

static void transition_event(const SceneEvent *e)
{
    memset(&transition, 0, sizeof transition);
    transition.kind = (uint8_t)e->a[0]; transition.phase = (uint8_t)e->a[1];
    transition.step = (uint16_t)e->a[2]; transition.steps = (uint16_t)e->a[3];
    transition.cx = (int16_t)e->a[4]; transition.cy = (int16_t)e->a[5];
}

/* The integrator can chain this to the gameplay hook; registration does not claim it. */
void draw_screens_transition(FrameTransition *t) { if (transition.kind) *t = transition; }

void draw_screens_register(void)
{
    memset(&transition, 0, sizeof transition);
    font_epoch = world_epoch = 0;
    memset(fonts, 0, sizeof fonts); memset(world, 0, sizeof world);
    compositor_handle(SCENE_SCREEN_PICTURE, picture_event);
    compositor_handle(SCENE_SCREEN_CLEAR, clear_event);
    compositor_handle(SCENE_SCREEN_COPY, copy_event);
    compositor_handle(SCENE_SCREEN_CHAR, char_event);
    compositor_handle(SCENE_SCREEN_FONT_BANKS, fonts_event);
    compositor_handle(SCENE_SCREEN_STRING, string_event);
    compositor_handle(SCENE_SCREEN_WORLD_MAP, world_event);
    compositor_handle(SCENE_SCREEN_WORLD_COLUMN, world_column);
    compositor_handle(SCENE_SCREEN_MAP_EDGES, map_edges);
    compositor_handle(SCENE_SCREEN_TRANSITION, transition_event);
}
