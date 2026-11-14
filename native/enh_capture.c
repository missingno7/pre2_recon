#include "enh_capture.h"
#include "draw_game_assets.h"
#include "compositor.h"
#include "../c/pre2.h"
#include "../c/view.h"
#include <stdlib.h>
#include <string.h>

typedef struct Allocation { struct Allocation *next; } Allocation;
typedef struct CelSet { unsigned refs; EnhCel cels[1024]; } CelSet;
typedef struct Dependency { struct Dependency *next; const EnhLevel *asset; } Dependency;
typedef struct LevelOwner {
    EnhLevel asset;
    struct LevelOwner *next;
    unsigned refs;
    Allocation *allocations;
    CelSet *sprites;
    const EnhMap *map;
    Dependency *dependencies;
} LevelOwner;
static LevelOwner *levels, *current;
static CelSet *sprites;
static uint8_t backdrop[64000];
static int panel_selector = -1;
static uint64_t hash(const void *p, size_t n, uint64_t h);
static FrameTilePaint ring[12][20];
typedef struct SurfaceState {
    EnhSurface value;
    const EnhSurface *published;
    FrameSprite redraw[FRAME_MAX_SPRITES];
    int active, dirty;
} SurfaceState;
static SurfaceState surfaces[2];

typedef struct HudState { uint16_t count; uint8_t frame; FrameHudElement elements[FRAME_MAX_HUD]; } HudState;
static HudState hud_pages[2];
static void hud_append(HudState *s, FrameHudElement e)
{
    if (e.x0 >= e.x1 || e.y0 >= e.y1) return;
    if (s->count == FRAME_MAX_HUD) abort(); /* capture must never silently discard a draw */
    s->elements[s->count++] = e;
}
static FrameHudElement hud_clip(FrameHudElement e, int x0, int y0, int x1, int y1)
{
    if (e.x0 < x0) e.x0 = (int16_t)x0;
    if (e.y0 < y0) e.y0 = (int16_t)y0;
    if (e.x1 > x1) e.x1 = (int16_t)x1;
    if (e.y1 > y1) e.y1 = (int16_t)y1;
    return e;
}
/* Remove only the overwritten area, retaining the asset origins of fragments.
 * This models dirty field writes and front-page copies without sampling pixels. */
static void hud_erase(HudState *s, int x0, int y0, int x1, int y1)
{
    HudState old = *s; s->count = 0;
    for (unsigned i = 0; i < old.count; i++) {
        FrameHudElement e = old.elements[i], q = hud_clip(e, x0, y0, x1, y1);
        if (q.x0 >= q.x1 || q.y0 >= q.y1) { hud_append(s, e); continue; }
        hud_append(s, hud_clip(e, e.x0, e.y0, e.x1, q.y0));
        hud_append(s, hud_clip(e, e.x0, q.y1, e.x1, e.y1));
        hud_append(s, hud_clip(e, e.x0, q.y0, q.x0, q.y1));
        hud_append(s, hud_clip(e, q.x1, q.y0, e.x1, q.y1));
    }
}
static uint8_t hud_frame_pixel(int x, int y)
{
    uint8_t c = 0;
    for (int p = 0; p < 4; p++) c |= (uint8_t)(((game_hud[0x7b0+p*920+y*40+x/8] >> (7-x%8)) & 1) << p);
    return c;
}
static int hud_frame_detail(int x)
{
    for (int y = 0; y < 23; y++) if (hud_frame_pixel(x,y) != hud_frame_pixel(1,y)) return 1;
    return 0;
}
/* The field owning an element: the one containing its horizontal centre. Glyphs and clears
 * never straddle a field boundary (their destinations are inside one field's span). */
static uint8_t hud_field(int x0, int x1)
{
    int c = (x0 + x1) / 2; uint8_t f = 0;
    while (f + 1 < HUD_FIELDS && c >= frame_hud_field_x[f + 1]) f++;
    return f;
}
void enh_capture_hud(unsigned kind, unsigned index, uint16_t dst)
{
    if (dst >= 0x3f40) return;
    unsigned page = !!(dst & 0x2000);
    int rel = dst & 0x1fff, x = (rel % 40) * 8, y = rel / 40 - 176;
    int w = 16, h = kind == 2 ? 12 : 11;
    if (kind < 2) { x = kind ? 264 : 0; y = 0; w = kind ? 48 : 320; h = 23; }
    FrameHudElement e = { (int16_t)x, (int16_t)y, (int16_t)x, (int16_t)y,
        (int16_t)(x+w), (int16_t)(y+h), (uint16_t)index, (uint8_t)kind, hud_field(x, x+w) };
    e = hud_clip(e, 0, 0, 320, 24);
    if (e.x0 >= e.x1 || e.y0 >= e.y1) return;
    HudState *s = &hud_pages[page]; hud_erase(s, e.x0, e.y0, e.x1, e.y1);
    /* Separate the repeated frame pattern from its static labels/icon/edge
     * details. Art columns differing from the plain border belong to the field
     * whose span contains them (the face, LIVES/SCORE/ENERGY labels, the BONUS
     * box and the strip's end borders), so a field moves with its own art. */
    if (!kind) {
        s->frame = 1;
        e.kind = 5;
        for (uint8_t f = 0; f < HUD_FIELDS; f++) {
            e.field = f; hud_append(s, hud_clip(e, frame_hud_field_x[f], 0, frame_hud_field_x[f+1], 24));
        }
        for (int x = 0; x < 320;) {
            if (!hud_frame_detail(x)) { x++; continue; }
            int left = x++; while (x < 320 && hud_frame_detail(x)) x++;
            e.kind = 0;
            for (uint8_t f = 0; f < HUD_FIELDS; f++) {   /* a run crossing a boundary would split */
                FrameHudElement part = hud_clip(e, left, 0, x, 23);
                part = hud_clip(part, frame_hud_field_x[f], 0, frame_hud_field_x[f+1], 23);
                if (part.x0 < part.x1) { part.field = f; hud_append(s, part); }
            }
        }
    }
    else hud_append(s, e);
}
static void hud_copy_rect(uint16_t src, uint16_t dst, int w, int h)
{
    int sx = (src & 0x1fff) % 40 * 8, sy = (src & 0x1fff) / 40 - 176;
    int dx = (dst & 0x1fff) % 40 * 8, dy = (dst & 0x1fff) / 40 - 176;
    if (dy >= 24 || dy+h <= 0) return;
    HudState old = hud_pages[!!(src & 0x2000)], *s = &hud_pages[!!(dst & 0x2000)];
    hud_erase(s, dx, dy < 0 ? 0 : dy, dx+w, dy+h > 24 ? 24 : dy+h);
    if (!old.frame) {
        FrameHudElement clear = {.x0=(int16_t)dx,.y0=(int16_t)dy,.x1=(int16_t)(dx+w),.y1=(int16_t)(dy+h),.kind=6,.field=hud_field(dx,dx+w)};
        hud_append(s, hud_clip(clear, 0, 0, 320, 24));
    }
    for (unsigned i = 0; i < old.count; i++) {
        FrameHudElement e = hud_clip(old.elements[i], sx, sy, sx+w, sy+h);
        e.x += (int16_t)(dx-sx); e.x0 += (int16_t)(dx-sx); e.x1 += (int16_t)(dx-sx);
        e.y += (int16_t)(dy-sy); e.y0 += (int16_t)(dy-sy); e.y1 += (int16_t)(dy-sy);
        hud_append(s, hud_clip(e, 0, 0, 320, 24));
    }
}
void enh_capture_hud_copy(uint16_t src, uint16_t dst, unsigned w, unsigned rows, unsigned ss, unsigned ds)
{
    if (src >= 0x3f40 || dst >= 0x3f40) return;
    if (w == 8000) { hud_pages[!!(dst & 0x2000)] = hud_pages[!!(src & 0x2000)]; return; }
    if (w == 0x398) {
        uint8_t frame = hud_pages[!!(src & 0x2000)].frame;
        hud_copy_rect(src, dst, 320, 23); hud_pages[!!(dst & 0x2000)].frame = frame; return;
    }
    if (ss == 40 && ds == 40) { hud_copy_rect(src, dst, (int)w*8, (int)rows); return; }
    for (unsigned y = 0; y < rows; y++) {
        hud_copy_rect(src, dst, (int)w*8, 1); src = (uint16_t)(src+ss); dst = (uint16_t)(dst+ds);
    }
}
void enh_capture_hud_clear(uint16_t dst, unsigned w, unsigned rows, unsigned stride)
{
    if (dst >= 0x3f40) return;
    if (w == 8000) { memset(&hud_pages[!!(dst & 0x2000)], 0, sizeof hud_pages[0]); return; }
    for (unsigned y = 0; y < rows; y++, dst = (uint16_t)(dst+stride)) {
        int rel = dst & 0x1fff, x = rel % 40 * 8, row = rel / 40 - 176;
        if (row >= 0 && row < 24) {
            HudState *s = &hud_pages[!!(dst & 0x2000)];
            hud_erase(s, x, row, x+(int)w*8, row+1);
            FrameHudElement clear = {.x0=(int16_t)x,.y0=(int16_t)row,.x1=(int16_t)(x+w*8),.y1=(int16_t)(row+1),.kind=6,.field=hud_field(x,x+(int)w*8)};
            hud_append(s, hud_clip(clear, 0, 0, 320, 24));
        }
    }
}
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
static void depends_on(LevelOwner *o, const EnhLevel *asset)
{
    if (!asset || asset == &o->asset) return;
    for (Dependency *d = o->dependencies; d; d = d->next) if (d->asset == asset) return;
    Dependency *d = owned(o, sizeof *d); d->asset = asset; d->next = o->dependencies; o->dependencies = d;
    enh_level_retain(asset);
}
static void capture_hud_view(FrameGameView *v, unsigned page)
{
    HudState *s = &hud_pages[page]; v->hud_count = s->count; v->hud_frame = s->frame;
    memcpy(v->hud_elements, s->elements, s->count * sizeof s->elements[0]);
}
void enh_capture_surface_hide(void) { memset(surfaces, 0, sizeof surfaces); }
void enh_capture_surface_ring(unsigned page) { memset(&surfaces[page], 0, sizeof surfaces[page]); }
static const EnhSurface *surface_snapshot(unsigned page)
{
    SurfaceState *s = &surfaces[page];
    if (!s->active || !current) return NULL;
    if (!s->published || s->dirty) {
        EnhSurface *p = owned(current, sizeof *p); *p = s->value;
        p->owner = &current->asset;
        if (p->base) depends_on(current, p->base->owner);
        if (p->reveal) depends_on(current, p->reveal->owner);
        if (p->redraw_count) {
            FrameSprite *sprites_copy = owned(current, p->redraw_count * sizeof *sprites_copy);
            memcpy(sprites_copy, s->redraw, p->redraw_count * sizeof *sprites_copy); p->redraw = sprites_copy;
        }
        s->published = p; s->dirty = 0;
    }
    return s->published;
}
static const EnhSurface *surface_source(FrameGameView pages[2], unsigned page)
{
    if (!current) return NULL;
    if (surfaces[page].active) return surface_snapshot(page);
    EnhSurface *p = owned(current, sizeof *p);
    p->owner = &current->asset;
    FrameGameView *v = owned(current, sizeof *v); *v = pages[page]; v->surface = NULL;
    /* A cleared page can precede the first ring pass. It still owns the loaded
     * HUD assets; absence of a world means literal indexed black underneath. */
    if (!v->assets) { v->assets = &current->asset; v->level_epoch = current->asset.epoch; v->level = current->asset.level; }
    depends_on(current, v->assets); capture_hud_view(v, page); p->world = v; return p;
}
static SurfaceState *surface_begin(FrameGameView pages[2], unsigned page)
{
    SurfaceState *s = &surfaces[page];
    if (!s->active) {
        const EnhSurface *base = surface_source(pages, page);
        memset(s, 0, sizeof *s); s->value.base = base; s->active = 1;
    }
    s->dirty = 1; return s;
}
static void row_union(EnhRowMask *r, unsigned left, unsigned right)
{
    if (left >= right) return;
    for (unsigned i = 0; i < r->count;) {
        if (right < r->left[i] || left > r->right[i]) { i++; continue; }
        if (left > r->left[i]) left = r->left[i];
        if (right < r->right[i]) right = r->right[i];
        r->count--; r->left[i] = r->left[r->count]; r->right[i] = r->right[r->count];
    }
    if (r->count == ENH_ROW_SPANS) abort();
    r->left[r->count] = (uint16_t)left; r->right[r->count++] = (uint16_t)right;
}
static void surface_clear_linear(SurfaceState *s, unsigned at, unsigned end)
{
    while (at < end && at < 64000) {
        unsigned row = at / 320, next = (row+1)*320; if (next > end) next = end;
        row_union(&s->value.rows[row], at%320, next-row*320); at = next;
    }
}
void enh_capture_surface_clear(FrameGameView pages[2], uint16_t dst, unsigned w, unsigned rows, unsigned stride)
{
    unsigned page = !!(dst & 0x2000), rel = dst & 0x1fff;
    if (!current || rel >= 8000 || (w != 8000 && !(w == 2 && rows == 10 && stride == 40))) return;
    SurfaceState *s = surface_begin(pages, page);
    if (s->value.reveal) {
        const EnhSurface *base = surface_snapshot(page);
        memset(s, 0, sizeof *s); s->value.base = base; s->active = s->dirty = 1;
    }
    if (w == 8000) {
        memset(&s->value, 0, sizeof s->value); s->value.redraw_count = 0;
        return;
    }
    for (unsigned y = 0; y < rows; y++) surface_clear_linear(s, (rel+y*stride)*8, (rel+y*stride+w)*8);
}
void enh_capture_surface_copy(FrameGameView pages[2], uint16_t src, uint16_t dst, unsigned w, unsigned rows)
{
    unsigned sp = !!(src & 0x2000), dp = !!(dst & 0x2000);
    if (!current || sp == dp) return;
    if (w == 8000) { surfaces[dp] = surfaces[sp]; return; }
    if (w != 2 || rows != 176) return;
    SurfaceState *s = surface_begin(pages, dp);
    if (!s->value.reveal) s->value.reveal = surface_source(pages, sp);
    s->value.columns |= 1u << ((dst & 0x1fff) / 2);
}
void enh_capture_surface_blackout(FrameGameView pages[2], unsigned x, unsigned w, unsigned y, uint16_t page)
{
    if (!current) return;
    SurfaceState *s = surface_begin(pages, !!(page & 0x2000));
    unsigned at = y*320 + (x/8)*8;
    /* Literal BlackoutSpan head/run/tail arithmetic. The unaligned head can
     * clear more than width, and the linear destination may enter the next row. */
    if ((x & 7) || w >= 8) { surface_clear_linear(s, at+(x&7), at+8); at += 8; w += x&7; }
    if (w >> 3) { unsigned n = ((w >> 3)-1)*8; surface_clear_linear(s, at, at+n); at += n; }
    if (w & 7) surface_clear_linear(s, at, at+(w&7));
}
static void iris_outside_row(SurfaceState *s, uint16_t y, uint16_t boundary, int cx)
{
    if (y >= 200) return;
    int left = (int16_t)((uint16_t)(cx*2)-boundary), right = (int16_t)boundary;
    if (left < 0 && (!(s->value.outside_valid[y]&1) || left>s->value.outside_left[y])) {
        s->value.outside_left[y]=(int16_t)left; s->value.outside_valid[y]|=1;
    }
    if (right >= 320 && (!(s->value.outside_valid[y]&2) || right<s->value.outside_right[y])) {
        s->value.outside_right[y]=(int16_t)right; s->value.outside_valid[y]|=2;
    }
}
void enh_capture_surface_iris(FrameGameView pages[2], uint16_t page, int cx, int cy, unsigned radius)
{
    if (!current) return;
    SurfaceState *s=surface_begin(pages,!!(page&0x2000));
    /* BlackoutSpan rejects boundaries outside 320 before emitting an event.
     * Re-evaluate RunIrisClose's SAME 65-point integer walk solely for those
     * unseen margins. Accepted spans still determine every original pixel. */
    uint16_t xs[65],ys[65],top=(uint16_t)(cy*2),lowest=0x7d0;
    if(top<240) top=240;
    int count=0;
    for(unsigned angle=0;angle<=64;angle++) {
        uint16_t product=(uint16_t)(DB(DS_byte_7090+angle)*(uint8_t)radius);
        uint16_t y=(uint16_t)(((int16_t)product>>6)+cy);
        if((int16_t)y<(int16_t)lowest && (int16_t)y<(int16_t)top) {
            lowest=y; ys[count]=y;
            product=(uint16_t)(DB(DS_SineTable+angle)*(uint8_t)radius);
            xs[count++]=(uint16_t)(((int16_t)product>>6)+cx);
        }
    }
    /* The point after the newly written table is stale. The original equality
     * test can consume it and exit; capture it before this step overwrites rows. */
    uint16_t sentinel=DW(DS_word_6B14+count*2);
    int next=0; uint16_t boundary=(uint16_t)cx,y=top;
    for(unsigned guard=0;guard<65536;guard++) {
        if(y==(next<count?ys[next]:sentinel)) {
            if(next==count) break;
            boundary=xs[next++];
        }
        iris_outside_row(s,y,boundary,cx);
        iris_outside_row(s,(uint16_t)(cy*2-y),boundary,cx);
        if(!--y) break;
    }
}
void enh_capture_surface_sprites(unsigned page)
{
    SurfaceState *s = &surfaces[page];
    if (s->active) { s->value.redraw_count = 0; s->dirty = 1; }
}
void enh_capture_surface_sprite(unsigned page, const FrameSprite *sprite)
{
    SurfaceState *s = &surfaces[page];
    if (s->active && s->value.redraw_count < FRAME_MAX_SPRITES) {
        s->redraw[s->value.redraw_count++] = *sprite; s->dirty = 1;
    }
}
void enh_capture_surface_publish(FrameGameView *v, uint16_t start)
{
    v->surface = surface_snapshot(!!(start & 0x2000));
    if (v->surface) {
        if (!v->valid) {
            /* Entry wipes can precede the first world pass. A semantic black
             * page still carries the current simulation aperture's dimensions. */
            v->view_w=pre2_view.width_px; v->view_h=pre2_view.height_px;
        }
        v->valid = 1;
        if (!v->assets && current) { v->assets = &current->asset; v->level_epoch = current->asset.epoch; v->level = current->asset.level; }
    }
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
    for (Dependency *d = o->dependencies; d; d = d->next) enh_level_release(d->asset);
    Allocation *p = o->allocations;
    while (p) { Allocation *next = p->next; free(p); p = next; }
    cels_release(o->sprites); free(o);
}
void enh_capture_reset(void)
{
    while (levels) { LevelOwner *next = levels->next; enh_level_release(&levels->asset); levels = next; }
    current = NULL; cels_release(sprites); sprites = NULL; memset(backdrop, 0, sizeof backdrop);
    memset(ring, 0, sizeof ring);
    memset(hud_pages, 0, sizeof hud_pages);
    memset(surfaces, 0, sizeof surfaces);
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
    /* LevelStart sets mode 0Dh before loading ordinary levels, clearing both
     * pages. Bonus loads deliberately skip that reset and keep their HUD. */
    if (a->level < 10) memset(hud_pages, 0, sizeof hud_pages);
    a->columns = 256; a->rows = DB(DS_MapRows); a->policy = enh_level_policy(a->level);
    for (unsigned i = 0; i < 65536; i++) a->map[i] = rd8(DW(DS_LevelSegment), (uint16_t)i);
    memcpy(a->tiles, game_tiles, sizeof a->tiles); memcpy(a->backdrop, backdrop, sizeof backdrop);
    memcpy(a->hud_source, game_hud, sizeof a->hud_source);
    for (int period = 1; period <= 160; period++) {
        if (320 % period) continue;
        int equal = 1;
        for (int y = 0; y < 200 && equal; y++) for (int x = period; x < 320; x++)
            if (a->backdrop[y*320+x] != a->backdrop[y*320+x%period]) { equal = 0; break; }
        if (equal) { a->backdrop_period = (uint16_t)period; break; }
    }
    /* Full-picture seams, audited in the decoded assets (not inferred from a
     * parked camera). Mountain/cloud 0/1/2/9/E and cave rock 7/8 continue at
     * the join. Trees, snow peaks, the food vignette and D's cloud do not.
     * No shipped picture consists of identical smaller horizontal periods. */
    static const uint8_t seamless[16] = {1,1,1,0,0,0,0,1,1,1,0,0,0,0,1,0};
    if (!a->backdrop_period && a->level < 16 && seamless[a->level]) a->backdrop_period = 320;
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
    a->content_hash = hash(&a->backdrop_period, sizeof a->backdrop_period, a->content_hash);
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
    capture_hud_view(v, !!(start & 0x2000));
    for (int y = 0; y < 24; y++) memcpy(v->hud + y * 320, comp_px((uint16_t)(start + (y + 176) * 40), 0), 320);
}
static uint64_t hash(const void *p, size_t n, uint64_t h)
{
    const uint8_t *s = p; while (n--) { h ^= *s++; h *= UINT64_C(1099511628211); } return h;
}
static uint64_t surface_hash(const EnhSurface *s, uint64_t h);
uint64_t enh_view_hash(const FrameGameView *v, uint64_t h)
{
    FrameGameView copy = *v; copy.assets = NULL; copy.map = NULL; copy.surface = NULL;
    h = hash(&copy, sizeof copy, h);
    if (v->map && v->assets) {
        for (int y = 0; y < v->assets->rows; y++) h = hash(v->map->rows[y], 256, h);
        h = hash(&v->assets->content_hash, sizeof v->assets->content_hash, h);
    }
    return surface_hash(v->surface, h);
}
static uint64_t surface_hash(const EnhSurface *s, uint64_t h)
{
    uint8_t present = s != NULL; h = hash(&present, sizeof present, h);
    if (!s) return h;
    EnhSurface copy = *s;
    copy.owner = NULL; copy.world = NULL; copy.base = copy.reveal = NULL; copy.redraw = NULL;
    h = hash(&copy, sizeof copy, h);
    present = s->world != NULL; h = hash(&present, sizeof present, h);
    if (s->world) h = enh_view_hash(s->world, h);
    h = surface_hash(s->base, h); h = surface_hash(s->reveal, h);
    if (s->redraw_count) h = hash(s->redraw, s->redraw_count * sizeof *s->redraw, h);
    return h;
}
