/* Screens drawing family of the native compositor (see compositor.h, c/scene_screens.h). */
#include "compositor.h"
#include "../c/scene_screens.h"
#include "../c/pre2.h"
#include <string.h>
#include <stdlib.h>
#include "enh_screens.h"
#include "draw_game_assets.h"
#include "../c/vga.h"
#include "../c/game.h"
#include "../c/scene_game.h"
#include "../c/view.h"

/* Captures are owned copies: the callback background is overwritten by shifted fonts,
 * PRESENT reuses its palette, and the intro loads its second pair over the first. */
static uint8_t picture[4 * 65536], fonts[65536], world[65536], glyph[48];
static uint32_t font_epoch, world_epoch;
static FrameTransition transition;

/* Independent screen authoring targets. They are materialized only by native
 * picture, glyph, rectangle and sprite operations below, never copied from the
 * faithful compositor. Immutable block snapshots preserve old publications while
 * the next callback repairs the ring or overwrites an asset. */
typedef struct ScreenTarget {
    uint8_t pixel[65536u * 8u];
    uint8_t dirty[SCREEN_PIXEL_BLOCKS];
    uint32_t size;
    const FrameScreenPixels *snapshot;
} ScreenTarget;
static ScreenTarget authored, linear, master;
static uint8_t cleared[65536];
static uint16_t reveal_frontier[4];
static unsigned screen_kind;
static uint32_t screen_epoch;
static int model_failed;
static const FrameScreenLayer *screen_layers;
static const FrameScreenPixels *screen_fonts;
static const FrameScreenPixels *screen_cels[1024];
typedef struct ScreenAllocation { struct ScreenAllocation *next; } ScreenAllocation;
static ScreenAllocation *screen_allocations;

static void *screen_allocate(size_t size)
{
    ScreenAllocation *a = malloc(sizeof *a + size);
    if (!a) { model_failed = 1; return NULL; }
    a->next = screen_allocations; screen_allocations = a;
    return a + 1;
}
static void screen_reset(void)
{
    while (screen_allocations) {
        ScreenAllocation *a = screen_allocations; screen_allocations = a->next; free(a);
    }
    memset(&authored, 0, sizeof authored); memset(&linear, 0, sizeof linear);
    memset(&master, 0, sizeof master); memset(cleared, 1, sizeof cleared);
    authored.size = 65536u * 8u; linear.size = 65536u; master.size = 640u * 200u;
    memset(authored.dirty, 1, sizeof authored.dirty);
    memset(linear.dirty, 1, sizeof linear.dirty); memset(master.dirty, 1, sizeof master.dirty);
    memset(reveal_frontier, 0, sizeof reveal_frontier);
    screen_kind = FRAME_SCREEN_EMPTY; screen_epoch = 0; model_failed = 0;
    screen_layers = NULL; screen_fonts = NULL; memset(screen_cels, 0, sizeof screen_cels);
}
static void target_write(ScreenTarget *target, unsigned at, uint8_t value)
{
    if (target->pixel[at] != value) {
        target->pixel[at] = value; target->dirty[at / SCREEN_PIXEL_BLOCK] = 1;
    }
}
static const FrameScreenPixels *target_snapshot(ScreenTarget *target)
{
    unsigned blocks = (target->size + SCREEN_PIXEL_BLOCK - 1) / SCREEN_PIXEL_BLOCK;
    int changed = !target->snapshot;
    for (unsigned b = 0; b < blocks; b++) changed |= target->dirty[b];
    if (!changed) return target->snapshot;
    size_t bytes = sizeof(FrameScreenPixels) + blocks * sizeof(const uint8_t *);
    FrameScreenPixels *image = screen_allocate(bytes);
    if (!image) return NULL;
    memset(image, 0, bytes); image->size = target->size;
    for (unsigned b = 0; b < blocks; b++) {
        const uint8_t *old = target->snapshot ? target->snapshot->block[b] : NULL;
        if (old && (!target->dirty[b] || !memcmp(old, target->pixel + b * SCREEN_PIXEL_BLOCK, SCREEN_PIXEL_BLOCK)))
            image->block[b] = old;
        else {
            uint8_t *pixels = screen_allocate(SCREEN_PIXEL_BLOCK);
            if (!pixels) return NULL;
            memcpy(pixels, target->pixel + b * SCREEN_PIXEL_BLOCK, SCREEN_PIXEL_BLOCK);
            image->block[b] = pixels;
        }
        target->dirty[b] = 0;
    }
    target->snapshot = image; return image;
}
/* The retained recipe is also the explanation of each native surface cache.
 * Captured source images outlive asset reuse; no layer stores a game pointer. */
static const FrameScreenPixels *source_image(const uint8_t *pixels, unsigned size)
{
    if (size > 65536u * 8u) return NULL;
    unsigned blocks = (size + SCREEN_PIXEL_BLOCK - 1) / SCREEN_PIXEL_BLOCK;
    size_t bytes = sizeof(FrameScreenPixels) + blocks * sizeof(const uint8_t *);
    FrameScreenPixels *image = screen_allocate(bytes);
    if (!image) return NULL;
    memset(image, 0, bytes); image->size = size;
    for (unsigned b = 0; b < (size + SCREEN_PIXEL_BLOCK - 1) / SCREEN_PIXEL_BLOCK; b++) {
        uint8_t *block = screen_allocate(SCREEN_PIXEL_BLOCK);
        if (!block) return NULL;
        unsigned count = size - b * SCREEN_PIXEL_BLOCK;
        if (count > SCREEN_PIXEL_BLOCK) count = SCREEN_PIXEL_BLOCK;
        memset(block, 0, SCREEN_PIXEL_BLOCK); memcpy(block, pixels + b * SCREEN_PIXEL_BLOCK, count);
        image->block[b] = block;
    }
    return image;
}
static FrameScreenLayer *screen_layer(unsigned kind, uint16_t destination, unsigned width,
                                      unsigned height, unsigned pitch, unsigned channels)
{
    FrameScreenLayer *layer = screen_allocate(sizeof *layer);
    if (!layer) return NULL;
    memset(layer, 0, sizeof *layer); layer->previous = screen_layers;
    layer->kind = (uint8_t)kind; layer->channels = (uint8_t)channels;
    layer->destination.origin = (unsigned)destination * 8u;
    unsigned row_width = pre2_vga.attr[0x10] & 0x40 ? 320u : (pre2_vga.crtc[1] >= 79 ? 640u : 320u);
    if (height == 1 && !pitch && width && !(layer->destination.origin % row_width) && !(width % row_width)) {
        height = width / row_width; width = row_width; pitch = row_width;
    }
    layer->destination.width = width; layer->destination.height = height;
    layer->destination.pitch = pitch; layer->destination.row_mask = 65536u * 8u - 1;
    layer->source.width = width; layer->source.height = height; layer->source.pitch = width;
    screen_layers = layer; return layer;
}
static void picture_layer(uint16_t destination, unsigned count, unsigned channels, int chain)
{
    static uint8_t decoded[65536u * 8u];
    FrameScreenLayer *layer = screen_layer(FRAME_SCREEN_IMAGE, destination,
                                          count * (chain ? 1 : 8), 1, 0, channels);
    if (!layer) return;
    if (chain) {
        layer->destination.origin = destination;
        layer->image = source_image(picture, count);
    } else {
        for (unsigned i = 0; i < count; i++) for (unsigned d = 0; d < 8; d++) {
            uint8_t c = 0; unsigned k = 0;
            for (unsigned p = 0; p < 4; p++) if (channels & (1u << p)) {
                c |= (uint8_t)(((picture[k + i] >> (7 - d)) & 1) << p); k += count;
            }
            decoded[i * 8 + d] = c;
        }
        layer->image = source_image(decoded, count * 8);
    }
    layer->source.width = layer->destination.width; layer->source.height = layer->destination.height;
}

static void author_plane(uint16_t dst, unsigned plane, uint8_t bits)
{
    uint8_t mask = (uint8_t)(1u << plane);
    for (unsigned d = 0; d < 8; d++) {
        unsigned at = (unsigned)dst * 8u + d;
        target_write(&authored, at, (uint8_t)((authored.pixel[at] & ~mask) | ((bits & (0x80u >> d)) ? mask : 0)));
    }
}
static void author_clear(uint16_t dst, unsigned count)
{
    for (unsigned i = 0; i < count; i++) {
        uint16_t at = (uint16_t)(dst + i); cleared[at] = 1;
        for (unsigned d = 0; d < 8; d++) target_write(&authored, (unsigned)at * 8u + d, 0);
    }
}
static void author_copy(uint16_t src, uint16_t dst)
{
    uint8_t dots[8]; memcpy(dots, authored.pixel + (unsigned)src * 8u, 8);
    for (unsigned d = 0; d < 8; d++) target_write(&authored, (unsigned)dst * 8u + d, dots[d]);
    cleared[dst] = cleared[src];
}

static void plane_byte(uint16_t dst, unsigned plane, uint8_t bits)
{
    author_plane(dst, plane, bits);
    uint8_t mask = (uint8_t)(1u << plane);
    for (int d = 0; d < 8; d++) {
        uint8_t *p = comp_px(dst, d);
        *p = (uint8_t)((*p & ~mask) | ((bits & (0x80u >> d)) ? mask : 0));
    }
}

static void copy_byte(uint16_t src, uint16_t dst)
{
    author_copy(src, dst);
    /* One whole indexed byte is the semantic latch-copy unit. */
    uint8_t dots[8];
    memcpy(dots, comp_px(src, 0), 8);
    memcpy(comp_px(dst, 0), dots, 8);
}

static void picture_event(const SceneEvent *e)
{
    uint16_t seg = (uint16_t)e->a[0], src = (uint16_t)e->a[1], dst = (uint16_t)e->a[2];
    unsigned n = (unsigned)e->a[3], mask = (unsigned)e->a[4], k = 0;
    screen_kind = dst == 0x5E80 ? FRAME_SCREEN_GAME_OVER : FRAME_SCREEN_PICTURE;
    for (unsigned i = 0; i < n; i++) cleared[(uint16_t)(dst + i)] = 0;
    if (e->a[5]) {
        for (unsigned i = 0; i < n; i++) picture[i] = rd8(seg, (uint16_t)(src + i));
        for (unsigned i = 0; i < n; i++) {
            uint16_t a = (uint16_t)(dst + i);
            comp_chain4[a & 3][a >> 2] = picture[i];
            target_write(&linear, a, picture[i]);
        }
    } else {
        for (unsigned p = 0; p < 4; p++) if (mask & (1u << p)) {
            for (unsigned i = 0; i < n; i++) picture[k + i] = rd8(seg, (uint16_t)(src + k + i));
            for (unsigned i = 0; i < n; i++) plane_byte((uint16_t)(dst + i), p, picture[k + i]);
            k += n;
        }
    }
    picture_layer(dst, n, mask, e->a[5]);
}

static void clear_event(const SceneEvent *e)
{
    screen_layer(FRAME_SCREEN_CLEAR, (uint16_t)e->a[0], (unsigned)e->a[1] * 8u, 1, 0, 15);
    author_clear((uint16_t)e->a[0], (unsigned)e->a[1]);
    for (unsigned i = 0; i < (unsigned)e->a[1]; i++)
        memset(comp_px((uint16_t)(e->a[0] + i), 0), 0, 8);
}

static void copy_event(const SceneEvent *e)
{
    if (e->a[0] == 0x3F40 && e->a[2] == 8000) screen_kind = FRAME_SCREEN_INTRO;
    FrameScreenLayer *layer = screen_layer(FRAME_SCREEN_COPY, (uint16_t)e->a[1], (unsigned)e->a[2] * 8u, 1, 0, 15);
    if (layer) { layer->source = layer->destination; layer->source.origin = (unsigned)(uint16_t)e->a[0] * 8u; }
    for (unsigned i = 0; i < (unsigned)e->a[2]; i++)
        copy_byte((uint16_t)(e->a[0] + i), (uint16_t)(e->a[1] + i));
}

static void char_event(const SceneEvent *e)
{
    screen_kind = FRAME_SCREEN_CREDITS;
    uint8_t c = (uint8_t)e->a[1], g = (uint8_t)(c - 0x30);
    if (g > 9) g = (uint8_t)(g - 2);
    for (unsigned i = 0; i < sizeof glyph; i++)
        glyph[i] = c == 0x20 ? 0 : rd8((uint16_t)e->a[0], (uint16_t)(g * 48u + i));
    uint8_t decoded[8 * 12] = {0};
    for (unsigned p = 0; p < 4; p++) for (unsigned r = 0; r < 12; r++) {
        plane_byte((uint16_t)(e->a[2] + r * 40), p, glyph[p * 12 + r]);
        for (unsigned x = 0; x < 8; x++) decoded[r * 8 + x] |= (uint8_t)(((glyph[p * 12 + r] >> (7 - x)) & 1) << p);
    }
    FrameScreenLayer *layer = screen_layer(FRAME_SCREEN_GLYPH, (uint16_t)e->a[2], 8, 12, 320, 15);
    if (layer) { layer->character = c; layer->image = source_image(decoded, sizeof decoded); }
}

static void fonts_event(const SceneEvent *e)
{
    screen_kind = FRAME_SCREEN_MENU; screen_epoch++;
    if (!++font_epoch) font_epoch = 1;
    for (unsigned i = 0; i < sizeof fonts; i++) fonts[i] = rd8((uint16_t)e->a[0], (uint16_t)i);
    screen_fonts = source_image(fonts, sizeof fonts);
    FrameScreenLayer *layer = screen_layer(FRAME_SCREEN_FONT, 0, 0, 0, 0, 12);
    if (layer) layer->image = screen_fonts;
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
        FrameScreenLayer *erase = screen_layer(FRAME_SCREEN_CLEAR, old, 24, 12, 320, 12);
        if (erase) erase->destination.row_mask = (unsigned)mask * 8u + 7u;
        uint8_t decoded[24 * 12] = {0};
        for (unsigned p = 0; p < 2; p++) for (unsigned r = 0; r < 12; r++) for (unsigned x = 0; x < 24; x++)
            decoded[r * 24 + x] |= (uint8_t)(((fonts[(uint16_t)(src + p * 48 + r * 3 + x / 8)] >> (7 - (x & 7))) & 1) << (p + 2));
        FrameScreenLayer *layer = screen_layer(FRAME_SCREEN_GLYPH, draw, 24, 12, 320, 12);
        if (layer) { layer->character = c; layer->destination.row_mask = (unsigned)mask * 8u + 7u; layer->image = source_image(decoded, sizeof decoded); }
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
    screen_kind = FRAME_SCREEN_WORLD_MAP; screen_epoch++;
    memset(reveal_frontier, 0, sizeof reveal_frontier);
    if (!++world_epoch) world_epoch = 1;
    for (unsigned i = 0; i < sizeof world; i++) world[i] = rd8((uint16_t)e->a[0], (uint16_t)i);
    for (unsigned y = 0; y < 200; y++) for (unsigned x = 0; x < 640; x++) {
        uint8_t c = 0;
        for (unsigned p = 0; p < 4; p++)
            c |= (uint8_t)(((world[p * 16000 + y * 80 + x / 8] >> (7 - (x & 7))) & 1) << p);
        target_write(&master, y * 640 + x, c);
    }
}

static void world_column(const SceneEvent *e)
{
    if (!world_epoch) return;
    uint16_t col = (uint16_t)(((uint16_t)e->a[0] >> 3) - 1), src = col, dst = col;
    for (unsigned p = 0; p < 4; p++) {
        if ((unsigned)e->a[1] & (1u << p)) {
            unsigned frontier = ((unsigned)(uint16_t)e->a[0] >> 3) * 8;
            if (frontier <= 640) reveal_frontier[p] = (uint16_t)frontier;
        }
        if ((unsigned)e->a[1] & (1u << p)) {
            uint8_t decoded[8 * 200];
            for (unsigned r = 0; r < 200; r++) for (unsigned x = 0; x < 8; x++)
                decoded[r * 8 + x] = (uint8_t)(((world[(uint16_t)(src + 80 * r)] >> (7 - x)) & 1) << p);
            FrameScreenLayer *layer = screen_layer(FRAME_SCREEN_COLUMN, (uint16_t)(dst & 0x1fff), 8, 200, 320, 1u << p);
            if (layer) { layer->destination.row_mask = 65535; layer->image = source_image(decoded, sizeof decoded); }
        }
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
    FrameScreenLayer *layer = screen_layer(FRAME_SCREEN_EDGE, (uint16_t)e->a[0], 328, 200, 320, 15);
    if (layer) { layer->previous_x = e->a[1]; layer->x = e->a[2]; layer->previous_y = e->a[3]; layer->y = e->a[4]; }
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
    screen_reset();
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


/* Screen/game handoffs share authored pages. Re-evaluate their native rectangle,
 * glyph and cel operations against this target so stale lower rows and byte
 * fringes remain defined without importing the compositor image. */
static void screen_copy_bytes(uint16_t src, uint16_t dst, unsigned width, unsigned rows, unsigned ss, unsigned ds)
{
    for (unsigned y = 0; y < rows; y++) {
        for (unsigned x = 0; x < width; x++) {
            author_copy((uint16_t)(src + x), (uint16_t)(dst + x));
        }
        src = (uint16_t)(src + ss); dst = (uint16_t)(dst + ds);
    }
}

static void screen_clear_bytes(uint16_t dst, unsigned width, unsigned rows, unsigned stride)
{
    for (unsigned y = 0; y < rows; y++, dst = (uint16_t)(dst + stride))
        author_clear(dst, width);
}

static void screen_mask_pixels(uint16_t dst, uint8_t bits, uint8_t col, int clear)
{
    for (int x = 0; x < 8; x++) if (bits & (0x80 >> x)) {
        unsigned at = (unsigned)dst * 8u + (unsigned)x;
        uint8_t *d = &authored.pixel[at];
        authored.dirty[at / SCREEN_PIXEL_BLOCK] = 1;
        if (clear) *d &= (uint8_t)~col; else *d |= col;
    }
}

static void screen_planar_asset(const uint8_t *src, uint16_t at, uint16_t dst, int width, int rows, int src_stride)
{
    int block = src_stride * rows;
    uint8_t decoded[320 * 24];
    for (int y = 0; y < rows; y++) for (int x = 0; x < width * 8; x++) {
        uint8_t c = 0;
        for (int p = 0; p < 4; p++)
            c |= (uint8_t)(((src[(uint16_t)(at + p * block + y * src_stride + x / 8)] >> (7 - x % 8)) & 1) << p);
        target_write(&authored, (unsigned)(uint16_t)(dst + 40 * y + x / 8) * 8u + (unsigned)(x % 8), c);
        decoded[y * width * 8 + x] = c;
    }
    if (screen_kind == FRAME_SCREEN_TALLY || screen_kind == FRAME_SCREEN_GAME_OVER || screen_kind == FRAME_SCREEN_INTRO) {
        FrameScreenLayer *layer = screen_layer(width < 40 ? FRAME_SCREEN_GLYPH : FRAME_SCREEN_IMAGE,
                                               dst, (unsigned)width * 8u, (unsigned)rows, 320, 15);
        if (layer) layer->image = source_image(decoded, (unsigned)(width * 8 * rows));
    }
}

static uint8_t screen_reverse_bits(uint8_t v)
{
    v = (uint8_t)((v >> 4) | (v << 4)); v = (uint8_t)(((v & 0xCC) >> 2) | ((v & 0x33) << 2));
    return (uint8_t)(((v & 0xAA) >> 1) | ((v & 0x55) << 1));
}

static uint8_t screen_rotate(uint8_t b, unsigned n) { return (uint8_t)((b >> n) | (b << ((8 - n) & 7))); }

static uint16_t screen_sprite_pass(const GameCel *c, uint16_t src, uint16_t dst, int mirror, int xs,
                            uint8_t vis, uint8_t rows, uint8_t left, uint8_t right,
                            int clip, int clear, uint8_t colour, uint8_t rowbytes)
{
    uint8_t ch = (uint8_t)(0xFFu << xs), rmask = (uint8_t)~ch;
    int count = rows ? rows : 256;
    for (int y = 0; y < count; y++) {
        if (!clip && !xs) {
            /* LOOP with CX=0 consumes a complete 64 KiB source window (quirk). */
            unsigned n = vis ? vis : 65536;
            uint16_t d = dst;
            for (unsigned x = 0; x < n; x++) {
                uint8_t b = game_cel_byte(c, src); src = (uint16_t)(src + (mirror ? -1 : 1));
                screen_mask_pixels(d++, mirror ? screen_reverse_bits(b) : b, colour, clear);
            }
            if (mirror) src = (uint16_t)(src + 2u * cbw(vis));
            dst = (uint16_t)(d - (uint16_t)(cbw(vis) - 40));
            continue;
        }
        uint8_t cl = vis, carry = 0, b;
        uint16_t d = dst;
        if (clip && left) {
            src = (uint16_t)(src + (mirror ? -(int)(cbw(left) - 1) : (int)(cbw(left) - 1)));
            b = game_cel_byte(c, src); src = (uint16_t)(src + (mirror ? -1 : 1));
            if (mirror) b = screen_reverse_bits(b);
            carry = b & rmask;
        } else {
            b = game_cel_byte(c, src); src = (uint16_t)(src + (mirror ? -1 : 1));
            if (mirror) b = screen_reverse_bits(b);
            screen_mask_pixels(d++, screen_rotate((uint8_t)(b & ch), xs), colour, clear);
            carry = b & rmask; cl--;
        }
        while (cl) {
            b = game_cel_byte(c, src); src = (uint16_t)(src + (mirror ? -1 : 1));
            if (mirror) b = screen_reverse_bits(b);
            screen_mask_pixels(d++, screen_rotate((uint8_t)((b & ch) | carry), xs), colour, clear);
            carry = b & rmask; cl--;
        }
        if (clip || xs) {
            /* Right clipping suppresses the carry write; a left-only clip keeps it. */
            uint8_t edge = clip && !left ? 0xFF : 0;
            uint8_t bits = (uint8_t)(carry & (uint8_t)~edge);
            screen_mask_pixels(d, screen_rotate(bits, xs), colour, clear);
        }
        if (mirror) {
            if (clip) src = (uint16_t)(src + cbw(rowbytes) + (uint8_t)(rowbytes - right));
            else src = (uint16_t)(src + 2u * cbw(vis));
        } else if (clip) src = (uint16_t)(src + cbw(right));
        dst = (uint16_t)(d - (uint16_t)(cbw(vis) - 40));
    }
    return src;
}

static void screen_on_sprite(const SceneEvent *e)
{
    unsigned frame = (unsigned)e->a[1]; if (frame >= 1024) return;
    const GameCel *c = &game_cels[frame];
    int mirror = e->a[2], pass = e->a[3], xs = e->a[4] & 7;
    uint16_t dst = (uint16_t)e->a[6], src = (uint16_t)e->a[7];
    uint8_t vis = (uint8_t)e->a[8], rows = (uint8_t)e->a[9];
    uint8_t left = (uint8_t)e->a[11], right = (uint8_t)(e->a[11] >> 8);
    uint8_t rowbytes = (uint8_t)(e->a[8] >> 8);
    int clip = (e->a[11] >> 16) & 255;
    uint16_t skip = (uint16_t)e->a[10];
    if (screen_kind == FRAME_SCREEN_INTRO || screen_kind == FRAME_SCREEN_GAME_OVER || screen_kind == FRAME_SCREEN_TALLY) {
        if (!screen_cels[frame] && c->blocks) screen_cels[frame] = source_image(c->blocks, 65536);
        FrameScreenLayer *layer = screen_layer(FRAME_SCREEN_CEL, dst, (unsigned)vis * 8u,
                                               rows ? rows : 256, 320, 15);
        if (layer) {
            layer->image = screen_cels[frame]; layer->asset = (uint16_t)frame;
            layer->x = e->a[4]; layer->y = e->a[5]; layer->mirror = (uint8_t)mirror;
            layer->pass = (uint8_t)pass; layer->phase = (uint8_t)xs;
            layer->clipped = (uint8_t)clip; layer->clip_left = left; layer->clip_right = right;
            layer->source.origin = src; layer->source.pitch = rowbytes;
            layer->source_skip = skip;
        }
    }
    if (!pre2_view.variant) {
      if (mirror) src = (uint16_t)(src - 1 + (clip ? cbw(rowbytes) : cbw(vis)));
      if (pass != 16 || clip) src = screen_sprite_pass(c, src, dst, mirror, xs, vis, rows, left, right, clip, 1, 15, rowbytes);
    }
    if (pass && !pre2_view.variant) {
        if (pass == 16) src = (uint16_t)(src - skip);
        for (int p = 0; p < (pass == 16 && !clip ? 1 : 4); p++) {
            src = (uint16_t)(src + skip);
            src = screen_sprite_pass(c, src, dst, mirror, xs, vis, rows, left, right, clip, 0,
                              pass == 16 ? 15 : (uint8_t)(1 << p), rowbytes);
        }
    }

}

static void screen_on_hud(const SceneEvent *e)
{
    uint16_t dst = (uint16_t)e->a[2]; int type = e->a[0], idx = e->a[1];
    if (type >= 2 && (uint16_t)(dst - DW(DS_DrawPage)) < 7040) screen_kind = FRAME_SCREEN_TALLY;
    if (type == 0) {
        screen_planar_asset(game_hud, 0x7B0, (uint16_t)(dst + 0x1B80), 40, 23, 40);
    } else if (type == 1) {
        screen_planar_asset(game_hud, 0x7D1, (uint16_t)(dst + 0x1BA1), 6, 23, 40);
    } else if (type == 2) screen_planar_asset(game_hud, (uint16_t)(0x1610 + idx * 96), dst, 2, 12, 2);
    else if (type == 3) screen_planar_asset(game_hud, (uint16_t)(0x1C70 + idx * 88), dst, 2, 11, 2);
    else {
        unsigned f = (uint16_t)(0xF1 + (int8_t)idx);
        if (f < 1024 && game_cels[f].blocks) screen_planar_asset(game_cels[f].blocks, 22, dst, 2, 11, 2);
    }
}

static void screen_on_ring(const SceneEvent *e)
{
    (void)e;
    uint16_t shake = DW(DS_ShakeRows), shake_bytes = (uint16_t)(40u * DB(DS_ShakeRows));
    uint16_t si = (uint16_t)(DW(DS_RingTileOffset) + 40u * DB(DS_FineScroll));
    uint16_t di = (uint16_t)(DW(DS_DrawPage) + shake_bytes);
    uint8_t dh = (uint8_t)(DB(DS_ColumnRing) * 2), dl = (uint8_t)((20 - DB(DS_ColumnRing)) * 2);
    uint16_t n = (uint16_t)(192 - (int16_t)(int8_t)DB(DS_FineScroll) - (DW(DS_RowRing) << 4)), rest;
    if (n < 176) {
        rest = (uint16_t)(176 - n - shake);
        if ((int16_t)rest < 0) { n = (uint16_t)(n + rest); rest = 0; }
    } else { n = (uint16_t)(176 - shake); rest = 0; }
    if (!n) { n = rest; rest = 0; }
    for (int part = 0; part < 2; part++) {
        if (part) { if (!rest) break; si = (uint16_t)(0x3F40 + dh); n = rest; }
        do {
            screen_copy_bytes(si, di, dl, 1, 0, 0); si = (uint16_t)(si + dl); di = (uint16_t)(di + dl);
            si = (uint16_t)(si - 40);
            screen_copy_bytes(si, di, dh, 1, 0, 0); si = (uint16_t)(si + dh + 40); di = (uint16_t)(di + dh);
        } while (--n);
    }
    screen_clear_bytes(DW(DS_DrawPage), (shake_bytes >> 1) * 2, 1, 0);
}

static void screen_on_upload(const SceneEvent *e)
{
    uint16_t seg = (uint16_t)e->a[0];
    unsigned block = e->kind == SCENE_GAME_STASH ? 0x6240 : 8000;
    uint16_t dst = e->kind == SCENE_GAME_STASH ? 0x9DC0 : 0x7E80;
    for (unsigned a = 0; a < block; a++) for (int x = 0; x < 8; x++) {
        uint8_t c = 0;
        for (unsigned p = 0; p < 4; p++) c |= (uint8_t)(((rd8(seg, (uint16_t)(a + p * block)) >> (7 - x)) & 1) << p);
        target_write(&authored, (unsigned)(uint16_t)(dst + a) * 8u + (unsigned)x, c);
    }
}

static void screen_on_blackout(const SceneEvent *e)
{
    uint16_t x = (uint16_t)e->a[0], w = (uint16_t)e->a[1], y = (uint16_t)e->a[2];
    uint16_t d = (uint16_t)(e->a[3] + 40 * (uint8_t)y + (x >> 3));
    if ((x & 7) || w >= 8) { screen_mask_pixels(d++, (uint8_t)(255 >> (x & 7)), 15, 1); w = (uint16_t)(w + (x & 7)); }
    /* Quirk: the full-byte count loses one even on the aligned head path. */
    if (w >> 3) { screen_clear_bytes(d, (w >> 3) - 1, 1, 0); d = (uint16_t)(d + (w >> 3) - 1); }
    if (w & 7) screen_mask_pixels(d, (uint8_t)~(255 >> (w & 7)), 15, 1);
}

static void screen_on_foreground(const SceneEvent *e)
{
    uint16_t cell = (uint16_t)e->a[0], tile = (uint16_t)e->a[1];
    uint16_t src = (uint16_t)(DW(0x8167 + 2 * tile) << 7);
    uint8_t row = (uint8_t)((cell >> 8) - DB(DS_CameraRow)), col = (uint8_t)(cell - DB(DS_CameraCol));
    uint16_t dst = (uint16_t)(DW(DS_DrawPage) + row * 640 + col * 2 - 40 * DB(DS_FineScroll));
    for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) {
        uint8_t colour = 0;
        for (int p = 0; p < 4; p++) colour |= (uint8_t)(((game_front[(uint16_t)(src + p * 32 + y * 2 + x / 8)] >> (7 - x % 8)) & 1) << p);
        if (colour) target_write(&authored, (unsigned)(uint16_t)(dst + y * 40 + x / 8) * 8u + (unsigned)(x % 8), colour);
    }
}

void draw_screens_observe(const SceneEvent *e)
{
    switch (e->kind) {
    case SCENE_MODE_SET:
        memset(&transition, 0, sizeof transition);
        if (!(e->a[0] & 0x80)) {
            author_clear(0, 65536);
            for (unsigned a = 0; a < linear.size; a++) target_write(&linear, a, 0);
            screen_kind = FRAME_SCREEN_EMPTY; screen_epoch++;
            screen_layers = NULL; screen_fonts = NULL;
        }
        break;
    case SCENE_GAME_ASSETS: memset(screen_cels, 0, sizeof screen_cels); break;
    case SCENE_GAME_COPY:
        screen_copy_bytes((uint16_t)e->a[0], (uint16_t)e->a[1], e->a[2], e->a[3], e->a[4], e->a[5]); break;
    case SCENE_GAME_CLEAR:
        screen_clear_bytes((uint16_t)e->a[0], e->a[1], e->a[2], e->a[3]);
        if (e->a[1] == 8000) screen_kind = FRAME_SCREEN_TALLY;
        break;
    case SCENE_GAME_HUD: screen_on_hud(e); break;
    case SCENE_GAME_SPRITE: screen_on_sprite(e); break;
    case SCENE_GAME_BACKDROP: case SCENE_GAME_STASH: screen_on_upload(e); break;
    case SCENE_GAME_RING: if (!pre2_view.variant) screen_on_ring(e); break;
    case SCENE_GAME_FOREGROUND: if (!pre2_view.variant) screen_on_foreground(e); break;
    case SCENE_GAME_BLACKOUT: screen_on_blackout(e); break;
    case SCENE_GAME_TILE:
        for (unsigned y = 0; y < 16; y++) for (unsigned x = 0; x < 16; x++) {
            uint8_t c = game_tiles[e->a[0] & 255][y * 16 + x];
            if (e->a[2] < 2 || c)
                target_write(&authored, (unsigned)(uint16_t)(e->a[1] + 40 * y + x / 8) * 8u + x % 8, c);
        }
        break;
    case SCENE_GAME_TILES:
        for (unsigned t = 0; t < 256; t++) for (unsigned y = 0; y < 16; y++) for (unsigned x = 0; x < 16; x++)
            target_write(&authored, (unsigned)(uint16_t)(0x5E80 + 32 * t + y * 2 + x / 8) * 8u + x % 8, game_tiles[t][y * 16 + x]);
        break;
    case SCENE_GAME_PANEL: {
        uint16_t dst = 0x3F58, map = (uint16_t)(20 + e->a[0] * 6);
        for (unsigned row = 0; row < 7; row++) {
            for (unsigned column = 0; column < 6; column++) {
                unsigned tile = rd8(DW(DS_LevelSegment), map++);
                for (unsigned y = 0; y < 16; y++) for (unsigned x = 0; x < 16; x++)
                    target_write(&authored, (unsigned)(uint16_t)(dst + 40 * y + x / 8) * 8u + x % 8, game_tiles[tile][y * 16 + x]);
                dst = (uint16_t)(dst + 2);
            }
            map = (uint16_t)(map + 250); dst = (uint16_t)(dst + 0x274);
        }
        break;
    }
    case SCENE_GAME_RESTORE: {
        uint16_t src = (uint16_t)e->a[0], dst = (uint16_t)e->a[1];
        for (unsigned y = 0; y < 16; y++) {
            screen_copy_bytes(src, dst, 2, 1, 0, 0);
            src = (uint16_t)(src + 40); dst = (uint16_t)(dst + 40);
            if (dst >= 0x5D40) dst = (uint16_t)(dst - 0x1E00);
        }
        break;
    }
    case SCENE_GAME_DOT:
        if (!pre2_view.variant) for (unsigned row = 0; row < (e->a[3] == 2 ? 2u : 1u); row++)
            screen_mask_pixels((uint16_t)(e->a[0] + row * 40), (uint8_t)(e->a[1] >> (row * 8)), e->a[4] ? 14 : (uint8_t)e->a[2], 0);
        break;
    default: break;
    }
}

static uint8_t screen_colour(unsigned c)
{
    const VgaState *v = &pre2_vga;
    uint8_t a = v->attr[c & 15] & 63;
    if (v->attr[0x10] & 0x80) return (uint8_t)((a & 15) | ((v->attr[0x14] & 15) << 4));
    return (uint8_t)(a | ((v->attr[0x14] & 12) << 4));
}

void draw_screens_publish(Pre2Frame *frame, uint16_t start)
{
    FrameScreenView *v = &frame->screen_view;
    const VgaState *hw = &pre2_vga;
    memset(v, 0, sizeof *v);
    v->kind = (uint8_t)screen_kind; v->epoch = screen_epoch;
    v->width = frame->w; v->height = frame->h; v->blank = frame->blank;
    v->border = screen_colour(hw->attr[0x11]);
    for (unsigned c = 0; c < 16; c++) v->colour[c] = screen_colour(c);
    int text = !(hw->gc[6] & 1);
    v->indexed256 = !!(hw->attr[0x10] & 0x40);
    if (text) { v->border = 0; v->blank = 1; }
    /* Rows are a semantic projection into native authoring images. Capture the
     * latched origin, not the start requested for the next retrace. The supported
     * planar BIOS modes are byte-addressed; mode 13 is linear indexed colour. */
    unsigned vde = (hw->crtc[0x12] | ((hw->crtc[7] & 2) << 7) | ((hw->crtc[7] & 64) << 3)) + 1;
    unsigned char_h = (hw->crtc[9] & 31) + 1, dbl = hw->crtc[9] & 128 ? 2 : 1;
    unsigned group = dbl * (char_h <= 2 ? char_h : 1);
    unsigned compare = hw->crtc[0x18] | ((hw->crtc[7] & 16) << 4) | ((hw->crtc[9] & 64) << 3);
    unsigned pitch = hw->crtc[0x13] * 2u, row_scan = 0, y = 0;
    uint16_t origin = start;
    unsigned pan = hw->attr[0x13] & 7;
    unsigned active = (hw->crtc[1] + 1u) * (v->indexed256 ? 4u : 8u);
    for (unsigned scan = 0; scan < vde && y < frame->h; scan++) {
        if (scan == compare) { origin = 0; row_scan = 0; }
        if (!(scan % group)) {
            FrameScreenRow *r = &v->rows[y++];
            unsigned p = scan >= compare && (hw->attr[0x10] & 32) ? 0 : pan;
            r->count = (uint16_t)(active < frame->w ? active : frame->w);
            if (v->indexed256) {
                r->origin = ((unsigned)origin * 4u + p / 2) & 65535u; r->mask = 65535u;
            } else {
                uint16_t mask = 65535, base = origin;
                if (!(hw->crtc[0x17] & 1)) { mask &= (uint16_t)~0x2000; base = (uint16_t)((base & ~0x2000) | ((row_scan & 1) << 13)); }
                if (!(hw->crtc[0x17] & 2)) { mask &= (uint16_t)~0x4000; base = (uint16_t)((base & ~0x4000) | ((row_scan & 2) << 13)); }
                r->origin = (unsigned)base * 8u + p; r->mask = (unsigned)mask * 8u + 7u;
                r->side_clear = 1;
                for (unsigned x = 0; x < frame->w / 8; x++)
                    if (!cleared[(uint16_t)((base + x) & mask)]) { r->side_clear = 0; break; }
            }
        }
        if (dbl == 1 || (scan & 1)) if (++row_scan >= char_h) { row_scan = 0; origin = (uint16_t)(origin + pitch); }
    }
    v->pan_x = (v->kind == FRAME_SCREEN_MENU || v->kind == FRAME_SCREEN_WORLD_MAP)
        ? (double)(v->rows[0].origin & v->rows[0].mask) : 0;
    v->pan_y = 0;
    memcpy(v->revealed, reveal_frontier, sizeof v->revealed);
    v->layers = screen_layers; v->font_banks = screen_fonts;
    v->surface = target_snapshot(v->indexed256 ? &linear : &authored);
    if (v->kind == FRAME_SCREEN_WORLD_MAP) v->master = target_snapshot(&master);
    v->valid = !model_failed && v->surface && !(frame->game.valid && frame->game.world_valid);
    if (!v->indexed256 && !(hw->crtc[0x17] & 64) && !text) v->valid = 0;
}
