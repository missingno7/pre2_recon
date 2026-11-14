/* The native compositor core (see compositor.h): the video store, event dispatch, mode sets,
 * publication. Drawing families live in draw_game.c and draw_screens.c. */
#include "compositor.h"
#include "scanout.h"
#include "../c/pre2.h"
#include "../c/vga.h"
#include "../c/game.h"
#include "../c/scene_screens.h"
#include "enh_screens.h"
#include <string.h>

uint8_t comp_planar[0x10000 * 8];
uint8_t comp_chain4[4][0x10000];
void (*compositor_game_view)(FrameGameView *v, uint16_t start) = 0;
void (*compositor_transition)(FrameTransition *t, uint16_t start) = 0;

static SceneHandler handlers[1024];
static uint32_t unhandled;
static uint16_t unhandled_first;

void compositor_handle(uint16_t kind, SceneHandler h) { if (kind < 1024) handlers[kind] = h; }

static void sink(const SceneEvent *e)
{
    if (e->kind < 1024 && handlers[e->kind]) handlers[e->kind](e);
    else if (!unhandled++) unhandled_first = e->kind;
    /* After dispatch, so the gameplay family has captured its native assets: the screen
     * model keeps its own authoring surfaces for game/screen handoffs (HUD, cels, copies). */
    draw_screens_observe(e);
}

uint32_t compositor_unhandled(uint16_t *first_kind) { if (first_kind) *first_kind = unhandled_first; return unhandled; }

/* BIOS mode set: the memory clear (the text-mode fill is not presented and not mirrored). */
static void on_mode_set(const SceneEvent *e)
{
    if (e->a[0] & 0x80) return;
    memset(comp_planar, 0, sizeof comp_planar);
    memset(comp_chain4, 0, sizeof comp_chain4);
}

void compositor_init(void)
{
    memset(handlers, 0, sizeof handlers);
    memset(comp_planar, 0, sizeof comp_planar);
    memset(comp_chain4, 0, sizeof comp_chain4);
    unhandled = 0; unhandled_first = 0;
    compositor_game_view = 0; compositor_transition = 0;
    compositor_handle(SCENE_MODE_SET, on_mode_set);
    draw_game_register();
    draw_screens_register();
    pre2_scene_sink = sink;
}

static uint8_t store_planar(void *u, uint16_t addr, int dot) { (void)u; return comp_planar[(uint32_t)addr * 8u + (uint32_t)dot]; }
static uint8_t store_chain4(void *u, int q, uint16_t off) { (void)u; return comp_chain4[q & 3][off]; }
static const ScanSource scan_store = { store_planar, store_chain4, 0 };

void compositor_publish(uint64_t t_ns, uint16_t start, Pre2Frame *f)
{
    static uint64_t seq;
    ScanInfo si;
    scan_indices(&scan_store, start, f->index, &si);
    f->seq = ++seq; f->t_ns = t_ns;
    f->w = (uint16_t)si.w; f->h = (uint16_t)si.h; f->blank = (uint8_t)si.blank;
    /* The logical palette: the DAC as the game programmed it. The game reads it back (fades,
     * the PRESENT morph), so it is part of the logical register file the production machine
     * keeps; the frame carries a copy, not a reference. */
    memcpy(f->palette, pre2_vga.dac, sizeof f->palette);
    f->pel_mask = pre2_vga.pel_mask;
    if (si.blank_black) { memset(f->palette[0], 0, 3); memset(f->index, 0, (size_t)si.w * (size_t)si.h); f->pel_mask = 0xFF; }
    f->screen = pre2_phase;
    memset(&f->transition, 0, sizeof f->transition);
    draw_screens_transition(&f->transition);              /* screens' fades and map scrolls */
    if (compositor_transition) compositor_transition(&f->transition, start);   /* gameplay's, if any */
    f->game.valid = 0;
    if (compositor_game_view) compositor_game_view(&f->game, start);
    draw_screens_publish(f, start);   /* semantic screen view; invalid while a world view shows */
}

static uint8_t expand6(uint8_t v) { return (uint8_t)((v << 2) | (v >> 4)); }
void frame_to_xrgb(const Pre2Frame *f, uint32_t *out)
{
    uint32_t pal[256];
    for (int i = 0; i < 256; i++) {
        const uint8_t *c = f->palette[i & f->pel_mask];
        pal[i] = 0xFF000000u | (uint32_t)expand6(c[0]) << 16 | (uint32_t)expand6(c[1]) << 8 | expand6(c[2]);
    }
    for (int i = 0; i < f->w * f->h; i++) out[i] = pal[f->index[i]];
}

uint32_t compositor_diff_vga(uint32_t *first_addr)
{
    uint32_t n = 0;
    int first = 1;
    if (pre2_vga.seq[4] & 0x08) {                       /* chain 4 */
        for (int q = 0; q < 4; q++) for (uint32_t o = 0; o < 0x10000; o++)
            if (comp_chain4[q][o] != pre2_vga.plane[q][o]) { if (first) { *first_addr = o * 4u + (uint32_t)q; first = 0; } n++; }
        return n;
    }
    if (!(pre2_vga.gc[6] & 1)) return 0;                /* text mode: not presented */
    for (uint32_t a = 0; a < 0x10000; a++) {
        for (int d = 0; d < 8; d++) {
            int bit = 7 - d, c = 0;
            for (int q = 0; q < 4; q++) c |= ((pre2_vga.plane[q][a] >> bit) & 1) << q;
            if (comp_planar[a * 8 + (uint32_t)d] != c) { if (first) { *first_addr = a; first = 0; } n++; }
        }
    }
    return n;
}
