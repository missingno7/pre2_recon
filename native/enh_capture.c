#include "enh_capture.h"
#include "draw_game_assets.h"
#include "compositor.h"
#include "../c/pre2.h"
#include "../c/view.h"
#include <stdlib.h>
#include <string.h>

typedef struct Allocation { struct Allocation *next; } Allocation;
typedef struct CelSet { unsigned refs; EnhCel cels[1024]; } CelSet;
typedef struct LevelOwner {
    EnhLevel asset;
    struct LevelOwner *next;
    unsigned refs;
    Allocation *allocations;
    CelSet *sprites;
    const EnhMap *map;
} LevelOwner;
static LevelOwner *levels, *current;
static CelSet *sprites;
static uint8_t backdrop[64000];
static int panel_selector = -1;
static uint64_t hash(const void *p, size_t n, uint64_t h);
static FrameTilePaint ring[12][20];
static FrameTilePaint *ring_cell(uint16_t destination)
{
    unsigned rel = (uint16_t)(destination - 0x3f40);
    if (rel >= 7680 || rel % 640 >= 40 || (rel & 1)) return NULL;
    return &ring[rel / 640][(rel % 640) / 2];
}
void enh_capture_tile(uint16_t dst, uint8_t tile, uint8_t cls)
{
    FrameTilePaint *p = ring_cell(dst); if (!p) return;
    p->valid = 1; p->tile = tile; p->cls = cls;
}
void enh_capture_restore(uint16_t src, uint16_t dst)
{
    FrameTilePaint *p = ring_cell(dst); if (!p) return;
    int rel = (int16_t)(src - 0x7e80), row = rel >= 0 ? rel / 40 : -((-rel + 39) / 40);
    p->valid = 1; p->cls = 1; p->tile = 0; p->back_x = (int16_t)((rel - row * 40) * 8); p->back_y = (int16_t)row;
}
static void apply_panel_paint(FrameGameView *v);
void enh_capture_panel(FrameGameView *v, unsigned panel)
{
    panel_selector = (int)panel;
    if (pre2_view.variant) apply_panel_paint(v);
}
static void apply_panel_paint(FrameGameView *v)
{
    if (panel_selector < 0 || !v || !v->valid || v->level != 9) return;
    uint16_t map = (uint16_t)(20 + panel_selector * 6);
    for (unsigned y = 0; y < 7; y++) {
        for (unsigned x = 0; x < 6; x++) {
            unsigned px = 12 + x;
            FrameTilePaint *p = &v->paint[y * 20 + px];
            p->valid = 1;
            p->cls = 0;
            p->tile = rd8(DW(DS_LevelSegment), map++);
            p->cell = (uint16_t)(((DB(DS_CameraRow) + y) << 8) | (uint8_t)(DB(DS_CameraCol) + px));
        }
        map = (uint16_t)(map + 250);
    }
}
static void *checked(size_t n) { void *p = calloc(1, n); if (!p) abort(); return p; }
static void *owned(LevelOwner *o, size_t n)
{
    Allocation *p = checked(sizeof *p + n); p->next = o->allocations; o->allocations = p; return p + 1;
}
static void cels_release(CelSet *s)
{
    if (!s || --s->refs) return;
    for (int i = 0; i < 1024; i++) { free((void *)s->cels[i].blocks); free((void *)s->cels[i].pixels); free((void *)s->cels[i].coverage); }
    free(s);
}
void enh_level_retain(const EnhLevel *a) { if (a) ((LevelOwner *)a)->refs++; }
void enh_level_release(const EnhLevel *a)
{
    LevelOwner *o = (LevelOwner *)a;
    if (!o || --o->refs) return;
    Allocation *p = o->allocations;
    while (p) { Allocation *next = p->next; free(p); p = next; }
    cels_release(o->sprites); free(o);
}
void enh_capture_reset(void)
{
    while (levels) { LevelOwner *next = levels->next; enh_level_release(&levels->asset); levels = next; }
    current = NULL; cels_release(sprites); sprites = NULL; memset(backdrop, 0, sizeof backdrop);
    memset(ring, 0, sizeof ring);
    panel_selector = -1;
}
void enh_capture_cels(void)
{
    cels_release(sprites); sprites = checked(sizeof *sprites); sprites->refs = 1;
    for (int i = 0; i < 1024; i++) {
        const GameCel *s = &game_cels[i]; EnhCel *d = &sprites->cels[i];
        d->w = s->w; d->h = s->h; d->row = s->row; d->size = s->size;
        if (s->blocks) {
            uint8_t *p = checked(65536); memcpy(p, s->blocks, 65536); d->blocks = p;
            p = checked(s->size * 8); memcpy(p, s->pixels, s->size * 8); d->pixels = p;
            p = checked(s->size * 8); memcpy(p, s->coverage, s->size * 8); d->coverage = p;
        }
    }
}
void enh_capture_backdrop(uint16_t seg)
{
    for (int y = 0; y < 200; y++) for (int x = 0; x < 320; x++) {
        uint8_t c = 0;
        for (int p = 0; p < 4; p++) c |= (uint8_t)(((rd8(seg, (uint16_t)(p * 8000 + y * 40 + x / 8)) >> (7 - x % 8)) & 1) << p);
        backdrop[y * 320 + x] = c;
    }
}
const EnhLevel *enh_level_lookup(uint32_t epoch)
{
    for (LevelOwner *p = levels; p; p = p->next) if (p->asset.epoch == epoch) return &p->asset;
    return NULL;
}
void enh_capture_level(uint32_t epoch)
{
    memset(ring, 0, sizeof ring);
    panel_selector = -1;
    LevelOwner *o = checked(sizeof *o); o->refs = 1; o->next = levels; levels = current = o;
    EnhLevel *a = &o->asset; a->epoch = epoch; a->level = DB(DS_CurrentLevel);
    a->columns = 256; a->rows = DB(DS_MapRows); a->policy = enh_level_policy(a->level);
    for (unsigned i = 0; i < 65536; i++) a->map[i] = rd8(DW(DS_LevelSegment), (uint16_t)i);
    memcpy(a->tiles, game_tiles, sizeof a->tiles); memcpy(a->backdrop, backdrop, sizeof backdrop);
    memcpy(a->hud_source, game_hud, sizeof a->hud_source);
    o->sprites = sprites; if (sprites) { sprites->refs++; a->cels = sprites->cels; }
    for (int t = 0; t < 256; t++) {
        a->tile_class[t] = DB(DS_TileClassTable + t); a->flags[t] = DB(DS_TileFlagsTable + t);
        a->animated[t] = DB(DS_AnimTileFlags + t);
        for (int p = 0; p < 3; p++) a->animation[p][t] = DB(0x6688 + p * 256 + t);
        uint16_t src = (uint16_t)(DW(0x8167 + t * 2) << 7);
        for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) {
            uint8_t c = 0;
            for (int p = 0; p < 4; p++) c |= (uint8_t)(((game_front[(uint16_t)(src + p * 32 + y * 2 + x / 8)] >> (7 - x % 8)) & 1) << p);
            a->front[t][y * 16 + x] = c;
        }
    }
    EnhMap *m = owned(o, sizeof *m);
    for (int y = 0; y < 256; y++) m->rows[y] = a->map + y * 256;
    o->map = m;
    a->content_hash = hash(a->map, sizeof a->map, UINT64_C(1469598103934665603));
    a->content_hash = hash(a->tiles, sizeof a->tiles, a->content_hash);
    a->content_hash = hash(a->tile_class, sizeof a->tile_class, a->content_hash);
    a->content_hash = hash(a->flags, sizeof a->flags, a->content_hash);
    a->content_hash = hash(a->animation, sizeof a->animation, a->content_hash);
    a->content_hash = hash(a->animated, sizeof a->animated, a->content_hash);
    a->content_hash = hash(a->front, sizeof a->front, a->content_hash);
    a->content_hash = hash(a->backdrop, sizeof a->backdrop, a->content_hash);
    a->content_hash = hash(a->hud_source, sizeof a->hud_source, a->content_hash);
    if (a->cels) for (int i = 0; i < 1024; i++) {
        const EnhCel *c = &a->cels[i];
        a->content_hash = hash(&c->w, sizeof c->w, a->content_hash);
        a->content_hash = hash(&c->h, sizeof c->h, a->content_hash);
        if (c->blocks) a->content_hash = hash(c->blocks, 65536, a->content_hash);
    }
}
void enh_capture_view(FrameGameView *v)
{
    if (!current) return;
    LevelOwner *o = current; EnhMap *m = NULL;
    for (int y = 0; y < o->asset.rows; y++) {
        uint8_t row[256];
        for (int x = 0; x < 256; x++) row[x] = rd8(DW(DS_LevelSegment), (uint16_t)(y * 256 + x));
        if (!memcmp(row, o->map->rows[y], 256)) continue;
        if (!m) { m = owned(o, sizeof *m); *m = *o->map; }
        uint8_t *p = owned(o, 256); memcpy(p, row, 256); m->rows[y] = p;
    }
    if (m) o->map = m;
    v->assets = &o->asset; v->map = o->map;
    for (int t = 0; t < 256; t++) v->animation[t] = DB(DW(DS_AnimRemapPtr) + t);
    v->fine_scroll = DB(DS_FineScroll); v->level_flags = DB(DS_LevelFlags);
    v->room = DB(DS_ScrollFollowGate);
    v->foreground_enabled = !!(CW(CS_OptionBits) & 2);
    v->camera_limit_column = DW(DS_CameraLeftLimit); v->camera_limit_row = (uint16_t)(o->asset.rows > 11 ? o->asset.rows - 11 : 0);
    if (pre2_view.variant) memset(v->paint, 0, sizeof v->paint);
    else for (int y = 0; y < 12; y++) for (int x = 0; x < 20; x++) {
        FrameTilePaint *p = &v->paint[y * 20 + x];
        *p = ring[(y + DW(DS_RowRing)) % 12][(x + DW(DS_ColumnRing)) % 20];
        p->cell = (uint16_t)(((DB(DS_CameraRow) + y) << 8) | (uint8_t)(DB(DS_CameraCol) + x));
    }
    if (pre2_view.variant) apply_panel_paint(v);
}
void enh_capture_publish(FrameGameView *v, uint16_t start)
{
    if (!v->valid) return;
    for (int y = 0; y < 24; y++) memcpy(v->hud + y * 320, comp_px((uint16_t)(start + (y + 176) * 40), 0), 320);
}
static uint64_t hash(const void *p, size_t n, uint64_t h)
{
    const uint8_t *s = p; while (n--) { h ^= *s++; h *= UINT64_C(1099511628211); } return h;
}
uint64_t enh_view_hash(const FrameGameView *v, uint64_t h)
{
    FrameGameView copy = *v; copy.assets = NULL; copy.map = NULL;
    h = hash(&copy, sizeof copy, h);
    if (v->map && v->assets) {
        for (int y = 0; y < v->assets->rows; y++) h = hash(v->map->rows[y], 256, h);
        h = hash(&v->assets->content_hash, sizeof v->assets->content_hash, h);
    }
    return h;
}
