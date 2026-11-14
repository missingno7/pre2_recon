/* Faithful gameplay operations on indexed pixels. No hardware register or latch model. */
#include "compositor.h"
#include "draw_game_assets.h"
#include "enh_capture.h"
#include "../c/pre2.h"
#include "../c/game.h"
#include "../c/view.h"
#include "../c/pools.h"
#include "../c/scene_game.h"
#include <string.h>

static FrameGameView pages[2];
static FrameTransition transitions[2];
static uint32_t epoch, generation[FRAME_ID_SLOT_STRIDE];
static uint8_t alive[FRAME_ID_SLOT_STRIDE], cuts[2];
static uint16_t owners[FRAME_ID_SLOT_STRIDE];
static uint32_t point_dot_generation[256], fly_dot_generation[20];
static uint8_t fly_dot_alive[20];
static const Pre2PoolKind capture_pool_order[] = { PRE2_POOL_CHILD, PRE2_POOL_BURST, PRE2_POOL_PROJECTION, PRE2_POOL_DEBRIS, PRE2_POOL_POINT, PRE2_POOL_TERRAIN };
static const unsigned capture_pool_original[] = { PRE2_POOL_ORIGINAL_CHILD, PRE2_POOL_ORIGINAL_BURST,
    PRE2_POOL_ORIGINAL_PROJECTION, PRE2_POOL_ORIGINAL_DEBRIS, PRE2_POOL_ORIGINAL_POINT, PRE2_POOL_ORIGINAL_TERRAIN };
/* Reserve a fixed identity range per role. Dynamic growth of an earlier pool must
 * never renumber a later role's ids in already-published frames. */
#define CAPTURE_ROLE_SLOT_STRIDE 256u
static unsigned capture_extension_slot(unsigned role, unsigned index)
{
    return 116u + role * CAPTURE_ROLE_SLOT_STRIDE + index - capture_pool_original[role];
}
static int capture_identity(uint16_t at, unsigned *slot, int *owner_at)
{
    if (at >= DS_RenderSlots && at < DS_RenderSlots + 116 * 18 && !((at - DS_RenderSlots) % 18)) {
        *slot = (at - DS_RenderSlots) / 18;
        if (pre2_view.variant) {
            *owner_at = 0;
            for (unsigned p = 0; p < sizeof capture_pool_order / sizeof capture_pool_order[0]; p++)
                for (unsigned i = 0; i < capture_pool_original[p]; i++)
                    if (pre2_pool_slot(capture_pool_order[p], i) == at) {
                        *owner_at = capture_pool_order[p] == PRE2_POOL_CHILD ? 6 :
                            ((capture_pool_order[p] == PRE2_POOL_PROJECTION || capture_pool_order[p] == PRE2_POOL_TERRAIN) ? 9 : 0);
                        return 1;
                    }
        } else {
            *owner_at = *slot >= 11 && *slot < 23 ? 6 :
                ((at >= 0x52E8 && at < 0x5450) || (at >= 0x5570 && at < 0x55EE) ? 9 : 0);
        }
        return 1;
    }
    if (!pre2_view.variant) return 0;
    for (unsigned p = 0; p < sizeof capture_pool_order / sizeof capture_pool_order[0]; p++) {
        Pre2PoolKind kind = capture_pool_order[p]; unsigned count = pre2_pool_count(kind);
        for (unsigned i = capture_pool_original[p]; i < count; i++) {
            uint16_t candidate = pre2_pool_slot(kind, i);
            if (candidate == at) {
                *slot = capture_extension_slot(p, i);
                *owner_at = kind == PRE2_POOL_CHILD ? 6 :
                    ((kind == PRE2_POOL_PROJECTION || kind == PRE2_POOL_TERRAIN) ? 9 : 0);
                return *slot < FRAME_ID_SLOT_STRIDE;
            }
        }
    }
    return 0;
}
static void reset_dot_identities(void)
{
    memset(point_dot_generation, 0, sizeof point_dot_generation);
    memset(fly_dot_generation, 0, sizeof fly_dot_generation);
    memset(fly_dot_alive, 0, sizeof fly_dot_alive);
}
static uint64_t dot_identity(unsigned kind, uint16_t source, unsigned piece, uint32_t gen)
{
    uint32_t low = ((kind & 15u) << 28) | ((uint32_t)source << 4) | (piece & 15u);
    return ((uint64_t)gen << 32) | low;
}
static int point_dot_index(uint16_t source)
{
    unsigned n = pre2_pool_count(PRE2_POOL_POINT);
    if (n > 256) n = 256;
    for (unsigned i = 0; i < n; i++) if (pre2_pool_slot(PRE2_POOL_POINT, i) == source) return (int)i;
    return -1;
}
static int page_index(uint16_t p) { return !!(p & 0x2000); }
static FrameGameView *draw_view(void) { return &pages[page_index(DW(DS_DrawPage))]; }

static void capture_view(FrameGameView *v)
{
    v->valid = 1; v->level = DB(DS_CurrentLevel); v->level_epoch = epoch;
    v->sim_frame = pre2_frames_run;
    v->camera_x = (int32_t)DW(DS_CameraCol) * 16;
    v->camera_y = (int32_t)DW(DS_CameraRow) * 16 + DB(DS_FineScroll);
    v->shake_rows = (int16_t)DW(DS_ShakeRows); v->view_w = pre2_view.width_px; v->view_h = pre2_view.height_px;
    v->camera_cut = cuts[page_index(DW(DS_DrawPage))];
    cuts[page_index(DW(DS_DrawPage))] = 0;
    enh_capture_view(v);
}
/* The view of the page the CRTC latched for this frame (start address = page offset; the page
 * just requested by FlipDisplayPage may not be on screen yet). */
static void publish_view(FrameGameView *v, uint16_t start)
{
    *v = pages[page_index(start)];
    if (strcmp(pre2_phase, "MainFrameLoop") && strcmp(pre2_phase, "SetLevelVideoMode()")) v->valid = 0;
    enh_capture_publish(v, start);
}
static void publish_transition(FrameTransition *t, uint16_t start)
{
    if (strcmp(pre2_phase, "MainFrameLoop") && strcmp(pre2_phase, "SetLevelVideoMode()")) return;
    if (transitions[page_index(start)].kind) *t = transitions[page_index(start)];
}

static void copy_bytes(uint16_t src, uint16_t dst, unsigned width, unsigned rows, unsigned ss, unsigned ds)
{
    for (unsigned y = 0; y < rows; y++) {
        for (unsigned x = 0; x < width; x++) {
            uint8_t px[8]; memcpy(px, comp_px((uint16_t)(src + x), 0), 8);
            memcpy(comp_px((uint16_t)(dst + x), 0), px, 8);
        }
        src = (uint16_t)(src + ss); dst = (uint16_t)(dst + ds);
    }
}
static void clear_bytes(uint16_t dst, unsigned width, unsigned rows, unsigned stride)
{
    for (unsigned y = 0; y < rows; y++, dst = (uint16_t)(dst + stride))
        for (unsigned x = 0; x < width; x++) memset(comp_px((uint16_t)(dst + x), 0), 0, 8);
}
/* A decoded bit-mask operation. Mask bit 7 is the left dot, colour is an indexed colour
 * or a channel subset. This preserves byte fringe effects without emulating a video bus. */
static void mask_pixels(uint16_t dst, uint8_t bits, uint8_t col, int clear)
{
    for (int x = 0; x < 8; x++) if (bits & (0x80 >> x)) {
        uint8_t *d = comp_px(dst, x);
        if (clear) *d &= (uint8_t)~col; else *d |= col;
    }
}
static void planar_asset(const uint8_t *src, uint16_t at, uint16_t dst, int width, int rows, int src_stride)
{
    int block = src_stride * rows;
    for (int y = 0; y < rows; y++) for (int x = 0; x < width * 8; x++) {
        uint8_t c = 0;
        for (int p = 0; p < 4; p++)
            c |= (uint8_t)(((src[(uint16_t)(at + p * block + y * src_stride + x / 8)] >> (7 - x % 8)) & 1) << p);
        *comp_px((uint16_t)(dst + 40 * y + x / 8), x % 8) = c;
    }
}
static void tile_pixels(unsigned tile, uint16_t dst, int masked)
{
    enh_capture_tile(dst, (uint8_t)tile, masked ? 2 : 0);
    for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) {
        uint8_t c = game_tiles[tile & 255][y * 16 + x];
        uint8_t *d = comp_px((uint16_t)(dst + 40 * y + x / 8), x % 8);
        if (!masked || c) *d = c;
    }
}
static void on_restore(const SceneEvent *e)
{
    enh_capture_restore((uint16_t)e->a[0], (uint16_t)e->a[1]);
    uint16_t s = (uint16_t)e->a[0], d = (uint16_t)e->a[1];
    for (int y = 0; y < 16; y++) {
        copy_bytes(s, d, 2, 1, 0, 0);
        s = (uint16_t)(s + 40); d = (uint16_t)(d + 40);
        if (d >= 0x5D40) d = (uint16_t)(d - 0x1E00);
    }
}
static void on_tiles(const SceneEvent *e)
{
    (void)e; game_tiles_capture();
    for (int t = 0; t < 256; t++) for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++)
        *comp_px((uint16_t)(0x5E80 + 32 * t + y * 2 + x / 8), x % 8) = game_tiles[t][y * 16 + x];
}
static void on_upload(const SceneEvent *e)
{
    if (e->kind == SCENE_GAME_BACKDROP) enh_capture_backdrop((uint16_t)e->a[0]);
    uint16_t seg = (uint16_t)e->a[0];
    unsigned block = e->kind == SCENE_GAME_STASH ? 0x6240 : 8000;
    uint16_t dst = e->kind == SCENE_GAME_STASH ? 0x9DC0 : 0x7E80;
    for (unsigned a = 0; a < block; a++) for (int x = 0; x < 8; x++) {
        uint8_t c = 0;
        for (unsigned p = 0; p < 4; p++) c |= (uint8_t)(((rd8(seg, (uint16_t)(a + p * block)) >> (7 - x)) & 1) << p);
        *comp_px((uint16_t)(dst + a), x) = c;
    }
}
static void on_ring(const SceneEvent *e)
{
    (void)e;
    FrameGameView *v = draw_view(); memset(v, 0, sizeof *v); capture_view(v);
    v->world_valid = 1;
    memset(&transitions[page_index(DW(DS_DrawPage))], 0, sizeof transitions[0]);
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
            copy_bytes(si, di, dl, 1, 0, 0); si = (uint16_t)(si + dl); di = (uint16_t)(di + dl);
            si = (uint16_t)(si - 40);
            copy_bytes(si, di, dh, 1, 0, 0); si = (uint16_t)(si + dh + 40); di = (uint16_t)(di + dh);
        } while (--n);
    }
    clear_bytes(DW(DS_DrawPage), (shake_bytes >> 1) * 2, 1, 0);
}
static uint8_t reverse_bits(uint8_t v)
{
    v = (uint8_t)((v >> 4) | (v << 4)); v = (uint8_t)(((v & 0xCC) >> 2) | ((v & 0x33) << 2));
    return (uint8_t)(((v & 0xAA) >> 1) | ((v & 0x55) << 1));
}
static uint8_t rotate(uint8_t b, unsigned n) { return (uint8_t)((b >> n) | (b << ((8 - n) & 7))); }

/* One mask/colour image pass over a cel, including byte clipping and its surviving carry.
 * The clipped flash quirk performs a clear plus four white passes, using different blocks. */
static uint16_t sprite_pass(const GameCel *c, uint16_t src, uint16_t dst, int mirror, int xs,
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
                mask_pixels(d++, mirror ? reverse_bits(b) : b, colour, clear);
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
            if (mirror) b = reverse_bits(b);
            carry = b & rmask;
        } else {
            b = game_cel_byte(c, src); src = (uint16_t)(src + (mirror ? -1 : 1));
            if (mirror) b = reverse_bits(b);
            mask_pixels(d++, rotate((uint8_t)(b & ch), xs), colour, clear);
            carry = b & rmask; cl--;
        }
        while (cl) {
            b = game_cel_byte(c, src); src = (uint16_t)(src + (mirror ? -1 : 1));
            if (mirror) b = reverse_bits(b);
            mask_pixels(d++, rotate((uint8_t)((b & ch) | carry), xs), colour, clear);
            carry = b & rmask; cl--;
        }
        if (clip || xs) {
            /* Right clipping suppresses the carry write; a left-only clip keeps it. */
            uint8_t edge = clip && !left ? 0xFF : 0;
            uint8_t bits = (uint8_t)(carry & (uint8_t)~edge);
            mask_pixels(d, rotate(bits, xs), colour, clear);
        }
        if (mirror) {
            if (clip) src = (uint16_t)(src + cbw(rowbytes) + (uint8_t)(rowbytes - right));
            else src = (uint16_t)(src + 2u * cbw(vis));
        } else if (clip) src = (uint16_t)(src + cbw(right));
        dst = (uint16_t)(d - (uint16_t)(cbw(vis) - 40));
    }
    return src;
}
static void on_sprite(const SceneEvent *e)
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
    if (!pre2_view.variant) {
      if (mirror) src = (uint16_t)(src - 1 + (clip ? cbw(rowbytes) : cbw(vis)));
      if (pass != 16 || clip) src = sprite_pass(c, src, dst, mirror, xs, vis, rows, left, right, clip, 1, 15, rowbytes);
    }
    if (pass && !pre2_view.variant) {
        if (pass == 16) src = (uint16_t)(src - skip);
        for (int p = 0; p < (pass == 16 && !clip ? 1 : 4); p++) {
            src = (uint16_t)(src + skip);
            src = sprite_pass(c, src, dst, mirror, xs, vis, rows, left, right, clip, 0,
                              pass == 16 ? 15 : (uint8_t)(1 << p), rowbytes);
        }
    }
    FrameGameView *v = draw_view();
    unsigned slot; int owner_at;
    if (capture_identity((uint16_t)e->a[0], &slot, &owner_at) && v->sprite_count < FRAME_MAX_SPRITES) {
        if (owner_at) {
            uint16_t owner = DW(e->a[0] + owner_at);
            if (owners[slot] != owner) alive[slot] = 0;
            owners[slot] = owner;
        }
        if (!alive[slot]) { generation[slot]++; alive[slot] = 1; }
        FrameSprite *s = &v->sprites[v->sprite_count++]; memset(s, 0, sizeof *s);
        s->slot = (uint16_t)slot; s->gen = generation[slot];
        s->id = generation[slot] * (pre2_view.variant ? FRAME_ID_SLOT_STRIDE : 128u) + slot;
        s->frame = (uint16_t)frame; s->mirror = (uint8_t)mirror; s->mode = pass == 16 ? 2 : (uint8_t)pass;
        s->screen_x = e->a[4]; s->screen_y = e->a[5]; s->screen_space = frame == 309;
        s->world_x = (int16_t)DW(e->a[0]); s->world_y = (int16_t)DW(e->a[0] + 2);
    }
}
static void on_sprites_begin(const SceneEvent *e)
{
    (void)e;
    FrameGameView *v = draw_view(); v->sprite_count = 0;
    if (pre2_view.variant) for (unsigned i = 0; i < 20; i++)
        if (DW(0x6EA9 + i * 8) == 0x55AA) fly_dot_alive[i] = 0;
    for (unsigned i = 0; i < 116; i++) {
        uint16_t at = (uint16_t)(DS_RenderSlots + i * 18);
        if (DW(at + 4) == 0xFFFF) alive[i] = 0;
        /* Child ownership and the two projected lists are explicit reuse signals.
         * Ambient/terrain slots can change owners without ever showing an empty frame. */
        unsigned identity; int owner_at;
        if (pre2_view.variant) (void)capture_identity(at, &identity, &owner_at);
        else owner_at = i >= 11 && i < 23 ? 6 : ((at >= 0x52E8 && at < 0x5450) || (at >= 0x5570 && at < 0x55EE) ? 9 : 0);
        if (owner_at) {
            uint16_t owner = DW(at + owner_at);
            if (owners[i] != owner) alive[i] = 0;
            owners[i] = owner;
        }
    }
    if (pre2_view.variant) for (unsigned p = 0; p < sizeof capture_pool_order / sizeof capture_pool_order[0]; p++) {
        unsigned count = pre2_pool_count(capture_pool_order[p]);
        for (unsigned i = capture_pool_original[p]; i < count; i++) {
            unsigned slot = capture_extension_slot(p, i);
            uint16_t at = pre2_pool_slot(capture_pool_order[p], i);
            if (slot < FRAME_ID_SLOT_STRIDE && DW(at + 4) == 0xFFFF) alive[slot] = 0;
        }
    }
}
static void on_hud(const SceneEvent *e)
{
    uint16_t dst = (uint16_t)e->a[2]; int type = e->a[0], idx = e->a[1];
    if (!pre2_view.variant && type >= 2 && (uint16_t)(dst - DW(DS_DrawPage)) < 7040) draw_view()->world_valid = 0;
    if (type == 0) {
        planar_asset(game_hud, 0x7B0, (uint16_t)(dst + 0x1B80), 40, 23, 40);
    } else if (type == 1) {
        planar_asset(game_hud, 0x7D1, (uint16_t)(dst + 0x1BA1), 6, 23, 40);
    } else if (type == 2) planar_asset(game_hud, (uint16_t)(0x1610 + idx * 96), dst, 2, 12, 2);
    else if (type == 3) planar_asset(game_hud, (uint16_t)(0x1C70 + idx * 88), dst, 2, 11, 2);
    else {
        unsigned f = (uint16_t)(0xF1 + (int8_t)idx);
        if (f < 1024 && game_cels[f].blocks) planar_asset(game_cels[f].blocks, 22, dst, 2, 11, 2);
    }
}
static void on_dot(const SceneEvent *e)
{
    FrameGameView *v = draw_view();
    int rows = e->a[3] == 2 ? 2 : 1;
    unsigned kind = (uint8_t)e->a[3];
    uint16_t source = e->n >= 8 ? (uint16_t)e->a[7] : 0;
    uint32_t gen = 0; int source_index = -1;
    if (pre2_view.variant && e->n >= 8) {
        if (kind == 0) {
            source_index = point_dot_index(source);
            if (source_index >= 0 && source_index < 256) {
                if (++point_dot_generation[source_index] == 0) ++point_dot_generation[source_index];
                gen = point_dot_generation[source_index];
            }
        } else if (kind == 1 && source >= 0x6EA9 && source < 0x6F49 && (source - 0x6EA9) % 8 == 0) {
            source_index = (source - 0x6EA9) / 8;
            if (!fly_dot_alive[source_index]) {
                if (++fly_dot_generation[source_index] == 0) ++fly_dot_generation[source_index];
                fly_dot_alive[source_index] = 1;
            }
            gen = fly_dot_generation[source_index];
        } else if (kind == 2) gen = epoch ? epoch : 1;
    }
    for (int row = 0; row < rows; row++) {
        uint16_t dst = (uint16_t)(e->a[0] + row * 40);
        uint8_t bits = (uint8_t)(e->a[1] >> (row * 8));
        /* Even firefly timers OR only channels 1..3. Channel zero retains its backdrop
         * bit: the original OR operation applies after its zero set/reset value. */
        if (!pre2_view.variant) mask_pixels(dst, bits, e->a[4] ? 14 : (uint8_t)e->a[2], 0);
        uint16_t rel = (uint16_t)(dst - DW(DS_DrawPage));
        unsigned piece = 0;
        for (int x = 0; x < 8; x++) if ((bits & (0x80 >> x)) && v->dot_count < FRAME_MAX_DOTS) {
            FrameDot *d = &v->dots[v->dot_count++];
            d->x = pre2_view.variant && e->n >= 7 ? e->a[5] + x : (int32_t)((rel % 40) * 8 + x);
            d->y = pre2_view.variant && e->n >= 7 ? e->a[6] + row : (int32_t)(rel / 40);
            d->colour = pre2_view.variant ? 0 : *comp_px(dst, x); d->kind = (uint8_t)e->a[3];
            d->or_colour = e->a[4] ? 14 : (uint8_t)e->a[2]; d->before_sprite = v->sprite_count;
            d->id = pre2_view.variant && e->n >= 8 ? dot_identity(kind, source, (unsigned)row * 8 + piece, gen) : 0;
            d->gen = pre2_view.variant && e->n >= 8 ? gen : 0;
            piece++;
        }
    }
}
static void on_blackout(const SceneEvent *e)
{
    uint16_t x = (uint16_t)e->a[0], w = (uint16_t)e->a[1], y = (uint16_t)e->a[2];
    uint16_t d = (uint16_t)(e->a[3] + 40 * (uint8_t)y + (x >> 3));
    if ((x & 7) || w >= 8) { mask_pixels(d++, (uint8_t)(255 >> (x & 7)), 15, 1); w = (uint16_t)(w + (x & 7)); }
    /* Quirk: the full-byte count loses one even on the aligned head path. */
    if (w >> 3) { clear_bytes(d, (w >> 3) - 1, 1, 0); d = (uint16_t)(d + (w >> 3) - 1); }
    if (w & 7) mask_pixels(d, (uint8_t)~(255 >> (w & 7)), 15, 1);
}
static void on_foreground(const SceneEvent *e)
{
    FrameGameView *v = draw_view();
    if (v->foreground_count < FRAME_MAX_FOREGROUND) {
        FrameForeground *f = &v->foreground[v->foreground_count++]; f->cell = (uint16_t)e->a[0]; f->tile = (uint8_t)e->a[1];
    }
    uint16_t cell = (uint16_t)e->a[0], tile = (uint16_t)e->a[1];
    uint16_t src = (uint16_t)(DW(0x8167 + 2 * tile) << 7);
    uint8_t row = (uint8_t)((cell >> 8) - DB(DS_CameraRow)), col = (uint8_t)(cell - DB(DS_CameraCol));
    uint16_t dst = (uint16_t)(DW(DS_DrawPage) + row * 640 + col * 2 - 40 * DB(DS_FineScroll));
    for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) {
        uint8_t colour = 0;
        for (int p = 0; p < 4; p++) colour |= (uint8_t)(((game_front[(uint16_t)(src + p * 32 + y * 2 + x / 8)] >> (7 - x % 8)) & 1) << p);
        if (colour) *comp_px((uint16_t)(dst + y * 40 + x / 8), x % 8) = colour;
    }
}
static void on_panel(const SceneEvent *e)
{
    uint16_t dst = 0x3F58, map = (uint16_t)(20 + e->a[0] * 6);
    for (int y = 0; y < 7; y++) {
        for (int x = 0; x < 6; x++) { tile_pixels(rd8(DW(DS_LevelSegment), map++), dst, 0); dst = (uint16_t)(dst + 2); }
        map = (uint16_t)(map + 250); dst = (uint16_t)(dst + 0x274);
    }
}
static void dispatch(const SceneEvent *e)
{
    /* The faithful page store has a fixed 320-pixel layout. A wide simulation publishes its
     * semantic world pass directly to enh_render; only metadata capture is meaningful here. */
    if (pre2_view.variant) {
        switch (e->kind) {
        case SCENE_GAME_ASSETS: game_assets_capture(); break;
        case SCENE_GAME_TILES: game_tiles_capture(); break;
        case SCENE_GAME_BACKDROP: enh_capture_backdrop((uint16_t)e->a[0]); break;
        case SCENE_GAME_LEVEL:
            epoch++; enh_capture_level(epoch); memset(pages, 0, sizeof pages);
            cuts[0] = cuts[1] = 1; memset(alive, 0, sizeof alive); reset_dot_identities(); break;
        case SCENE_GAME_CUT:
            cuts[0] = cuts[1] = 1;
            if (e->n && e->a[0]) memset(alive, 0, sizeof alive);
            break;
        case SCENE_GAME_RING: {
            FrameGameView *v = draw_view(); memset(v, 0, sizeof *v); capture_view(v);
            v->world_valid = 1; break;
        }
        case SCENE_GAME_SPRITES_BEGIN: on_sprites_begin(e); break;
        case SCENE_GAME_SPRITE: on_sprite(e); break;
        case SCENE_GAME_DOT: on_dot(e); break;
        case SCENE_GAME_FOREGROUND: {
            FrameGameView *v = draw_view();
            if (v->foreground_count < FRAME_MAX_FOREGROUND) {
                FrameForeground *f = &v->foreground[v->foreground_count++];
                f->cell = (uint16_t)e->a[0]; f->tile = (uint8_t)e->a[1];
            }
            break;
        }
        case SCENE_GAME_HUD: on_hud(e); break;
        case SCENE_GAME_COPY: case SCENE_GAME_CLEAR: {
            /* The HUD keeps the original 320-pixel strip in every geometry: its background
             * and its copies to both pages arrive as rectangle copies/clears, which must
             * still land (other rectangles carry 320-layout coordinates of the ring and are
             * not meaningful in the variant). */
            uint16_t dst = (uint16_t)(e->kind == SCENE_GAME_COPY ? e->a[1] : e->a[0]);
            uint16_t rel = (uint16_t)(dst & 0x1FFF);
            if (rel >= 0x1B80 && rel < 0x1F40) {
                if (e->kind == SCENE_GAME_COPY) copy_bytes((uint16_t)e->a[0], (uint16_t)e->a[1], e->a[2], e->a[3], e->a[4], e->a[5]);
                else clear_bytes((uint16_t)e->a[0], e->a[1], e->a[2], e->a[3]);
            }
            break;
        }
        case SCENE_GAME_PANEL: enh_capture_panel(draw_view(), (unsigned)e->a[0]); break;
        case SCENE_GAME_ALLOC: {
            unsigned slot; int owner_at;
            if (capture_identity((uint16_t)e->a[0], &slot, &owner_at) && slot < FRAME_ID_SLOT_STRIDE)
                alive[slot] = 0;
            break;
        }
        case SCENE_GAME_HIDE: memset(pages, 0, sizeof pages); break;
        default: break;
        }
        return;
    }
    switch (e->kind) {
    case SCENE_GAME_ASSETS: game_assets_capture(); break;
    case SCENE_GAME_TILES: on_tiles(e); break;
    case SCENE_GAME_STASH: case SCENE_GAME_BACKDROP: on_upload(e); break;
    case SCENE_GAME_COPY:
        copy_bytes((uint16_t)e->a[0], (uint16_t)e->a[1], e->a[2], e->a[3], e->a[4], e->a[5]);
        if ((e->a[2] == 2 && e->a[3] == 176) || e->a[2] == 8000)
        {
            pages[page_index((uint16_t)e->a[1])] = pages[page_index((uint16_t)e->a[0])];
            transitions[page_index((uint16_t)e->a[1])] = transitions[page_index((uint16_t)e->a[0])];
        }
        break;
    case SCENE_GAME_CLEAR:
        clear_bytes((uint16_t)e->a[0], e->a[1], e->a[2], e->a[3]);
        if (e->a[1] == 8000 && (uint16_t)e->a[0] == DW(DS_DrawPage)) {
            FrameGameView *v = draw_view(); memset(v, 0, sizeof *v); capture_view(v);
        }
        break;
    case SCENE_GAME_TILE: tile_pixels(e->a[0], (uint16_t)e->a[1], e->a[2] >= 2); break;
    case SCENE_GAME_RESTORE: on_restore(e); break;
    case SCENE_GAME_RING: on_ring(e); break;
    case SCENE_GAME_FOREGROUND: on_foreground(e); break;
    case SCENE_GAME_SPRITE: on_sprite(e); break;
    case SCENE_GAME_SPRITES_BEGIN: on_sprites_begin(e); break;
    case SCENE_GAME_HUD: on_hud(e); break;
    case SCENE_GAME_DOT: on_dot(e); break;
    case SCENE_GAME_BLACKOUT: on_blackout(e); break;
    case SCENE_GAME_TRANSITION: {
        if (!e->a[0]) {
            /* Ending the iris does not restore its destructively blackened pages.
             * They remain non-world surfaces during resource loading and tally. */
            for (int i = 0; i < 2; i++) if (transitions[i].kind == 1) pages[i].world_valid = 0;
            memset(transitions, 0, sizeof transitions); break;
        }
        /* Iris edits the back page; reveal/wipe edit the currently displayed page. */
        int page = page_index(DW(e->a[0] == 1 ? DS_DrawPage : DS_DisplayPage));
        FrameTransition *t = &transitions[page];
        t->kind = (uint8_t)e->a[0]; t->phase = (uint8_t)e->a[1];
        t->step = (uint16_t)e->a[2]; t->steps = (uint16_t)e->a[3];
        t->cx = (int16_t)e->a[4]; t->cy = (int16_t)e->a[5]; t->radius = (int16_t)e->a[6];
        break;
    }
    case SCENE_GAME_LEVEL: epoch++; enh_capture_level(epoch); memset(pages, 0, sizeof pages); /* fall through */
    case SCENE_GAME_CUT:
        cuts[0] = cuts[1] = 1;
        if (e->kind == SCENE_GAME_LEVEL || (e->n && e->a[0])) memset(alive, 0, sizeof alive);
        break;
    case SCENE_GAME_PANEL: on_panel(e); break;
    case SCENE_GAME_HIDE: memset(pages, 0, sizeof pages); memset(transitions, 0, sizeof transitions); break;
    case SCENE_GAME_ALLOC: {
        unsigned slot = (uint16_t)(e->a[0] - DS_RenderSlots) / 18;
        if (slot < 116) alive[slot] = 0;
        break;
    }
    }
}
void draw_game_register(void)
{
    enh_capture_reset();
    game_assets_reset(); memset(pages, 0, sizeof pages); memset(transitions, 0, sizeof transitions);
    epoch = 0; memset(owners, 0, sizeof owners); memset(generation, 0, sizeof generation); memset(alive, 0, sizeof alive); cuts[0] = cuts[1] = 1;
    reset_dot_identities();
    for (int kind = SCENE_GAME_ASSETS; kind <= SCENE_GAME_HIDE; kind++) compositor_handle((uint16_t)kind, dispatch);
    compositor_game_view = publish_view; compositor_transition = publish_transition;
}
