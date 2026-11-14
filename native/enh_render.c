#include "enh_render.h"
#include "level_regions.h"
#include <math.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

EnhLevelPolicy enh_level_policy(unsigned level)
{
    Pre2RegionPolicy p = pre2_region_backing_policy(level);
    EnhLevelPolicy out = { p.first_column, p.end_column,
                           p.fixed_aperture, p.graphics_columns };
    /* This is the lifetime asset domain, not the current room's aperture.
     * LEVEL6's lower encounter is wider than its tower; its rectangles live
     * in pre2_region_for_view. Only the two 20-column bands are global. */
    out.fixed_room = level == 9 || level == 14;
    return out;
}

static int floor_int(double x, int *out)
{
    if (!isfinite(x) || x < -1000000 || x > 1000000) return -1;
    *out = (int)floor(x); return 0;
}
static int divfloor(int x, int d) { return x >= 0 ? x / d : -((-x + d - 1) / d); }
static uint8_t reverse(uint8_t b)
{
    b = (uint8_t)((b >> 4) | (b << 4)); b = (uint8_t)(((b & 0xcc) >> 2) | ((b & 0x33) << 2));
    return (uint8_t)(((b & 0xaa) >> 1) | ((b & 0x55) << 1));
}
static uint8_t rotate(uint8_t b, unsigned n) { return (uint8_t)((b >> n) | (b << ((8 - n) & 7))); }
static uint8_t cel_byte(const EnhCel *c, uint16_t off)
{
    if (!c->pixels || !c->coverage || !c->size || off >= c->size * 5) return c->blocks[off];
    unsigned block = off / c->size, at = (off % c->size) * 8;
    uint8_t bits = 0;
    for (int x = 0; x < 8; x++) if (block ? !!(c->pixels[at + x] & (1 << (block - 1))) : c->coverage[at + x]) bits |= (uint8_t)(128 >> x);
    return bits;
}
static void bits(EnhImage *im, int w, int h, int bx, int y, uint8_t mask, uint8_t colour, int clear)
{
    if (y < 0 || y >= h) return;
    for (int i = 0; i < 8; i++) if ((mask & (128 >> i)) && bx + i >= 0 && bx + i < w) {
        uint8_t *p = im->index + (size_t)y * im->stride + bx + i;
        if (clear) *p &= (uint8_t)~colour; else *p |= colour;
    }
}
/* Apply the source masks directly. Unlike an alpha texture, blink clears and
 * flash ORs channels; shifted erase carries retain the original fringe exactly.
 * Destination coordinates are bounded Cartesian pixels, with no page/ring layout. */
static uint16_t pass(const EnhCel *c, uint16_t src, int mirror, int xs, int bx, int top,
                     int vis, int rows, int left, int right, int clip, int clear,
                     uint8_t colour, EnhImage *im, int w, int h)
{
    uint8_t ch = (uint8_t)(255u << xs), low = (uint8_t)~ch;
    for (int y = 0; y < rows; y++) {
        int d = bx, n = vis; uint8_t carry = 0, b;
        if (!clip && !xs) {
            for (int x = 0; x < vis; x++) {
                b = cel_byte(c, src); src = (uint16_t)(src + (mirror ? -1 : 1));
                bits(im, w, h, d, top + y, mirror ? reverse(b) : b, colour, clear); d += 8;
            }
        } else {
            if (clip && left) {
                src = (uint16_t)(src + (mirror ? -(left - 1) : left - 1));
                b = cel_byte(c, src); src = (uint16_t)(src + (mirror ? -1 : 1)); if (mirror) b = reverse(b);
                carry = b & low;
            } else {
                b = cel_byte(c, src); src = (uint16_t)(src + (mirror ? -1 : 1)); if (mirror) b = reverse(b);
                bits(im, w, h, d, top + y, rotate(b & ch, (unsigned)xs), colour, clear); d += 8;
                carry = b & low; n--;
            }
            while (n-- > 0) {
                b = cel_byte(c, src); src = (uint16_t)(src + (mirror ? -1 : 1)); if (mirror) b = reverse(b);
                bits(im, w, h, d, top + y, rotate((b & ch) | carry, (unsigned)xs), colour, clear); d += 8;
                carry = b & low;
            }
            bits(im, w, h, d, top + y, clip && !left ? 0 : rotate(carry, (unsigned)xs), colour, clear);
        }
        if (mirror) src = (uint16_t)(src + (clip ? c->row + c->row - right : 2 * vis));
        else if (clip) src = (uint16_t)(src + right);
    }
    return src;
}
static void sprite(const FrameSprite *s, const EnhCel *c, int x, int y, int w, int h, EnhImage *im)
{
    if (!c->blocks || !c->row || !c->h || x >= w || x + c->w < 0 || y >= h || y + c->h <= 0) return;
    int rows = c->h, skip = 0, src = 0, rb = c->w / 8, vis = rb, left = 0, right = 0, clip = 0;
    if (y < 0) { rows += y; src = -y * rb; skip = src; y = 0; }
    if (y + rows > h) { skip += (y + rows - h) * rb; rows = h - y; }
    int bx = divfloor(x, 8) * 8, xs = x - bx;
    if (x < 0) { left = -divfloor(x, 8); vis -= left; clip = 1; bx = 0; }
    else if (bx / 8 + vis >= (w + 7) / 8) { right = bx / 8 + vis - (w + 7) / 8; vis -= right; clip = 1; }
    /* A left clip can leave zero complete bytes and still draw the carry. */
    if (vis < 0 || rows <= 0) return;
    if (s->mirror) src += rb - 1;
    uint16_t at = (uint16_t)src;
    if (s->mode != 2 || clip) at = pass(c, at, s->mirror, xs, bx, y, vis, rows, left, right, clip, 1, 15, im, w, h);
    if (s->mode) {
        if (s->mode == 2) at = (uint16_t)(at - skip);
        for (int p = 0; p < (s->mode == 2 && !clip ? 1 : 4); p++) {
            at = (uint16_t)(at + skip);
            at = pass(c, at, s->mirror, xs, bx, y, vis, rows, left, right, clip, 0,
                      s->mode == 2 ? 15 : (uint8_t)(1 << p), im, w, h);
        }
    }
}
static uint8_t back(const EnhLevel *a, int x, int y, int w, EnhBackdrop mode)
{
    if (y < 0 || y >= 200) return 0;
    int sx = x - (w - 320) / 2;
    if (a->backdrop_period) { sx %= a->backdrop_period; if (sx < 0) sx += a->backdrop_period; }
    else if (mode == ENH_BACK_STRETCH) sx = (int)((int64_t)x * 320 / w);
    else if (mode == ENH_BACK_MIRROR) { sx %= 640; if (sx < 0) sx += 640; if (sx >= 320) sx = 639 - sx; }
    if (sx < 0 || sx >= 320) return 0;
    return a->backdrop[y * 320 + sx];
}
static void dots(const FrameGameView *v, unsigned before, int dx, int dy, int w, int h, EnhImage *im)
{
    for (unsigned i = 0; i < v->dot_count; i++) {
        const FrameDot *d = &v->dots[i]; if (d->before_sprite != before) continue;
        int x = d->x + dx, y = d->y + dy;
        if (x >= 0 && x < w && y >= 0 && y < h) im->index[(size_t)y * im->stride + x] |= d->or_colour;
    }
}
static void front_tile(const EnhLevel *a, unsigned tile, int ox, int oy, int w, int h, EnhImage *im,
                       const FrameGameView *exclude, int cx, int cy)
{
    for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) {
        if (exclude && ox + x + cx >= exclude->camera_x && ox + x + cx < exclude->camera_x + 320 &&
            oy + y + cy >= exclude->camera_y && oy + y + cy < exclude->camera_y + 176) continue;
        uint8_t c = a->front[tile][y * 16 + x];
        if (c && ox + x >= 0 && ox + x < w && oy + y >= 0 && oy + y < h) im->index[(size_t)(oy + y) * im->stride + ox + x] = c;
    }
}
static uint8_t hud_pixel(const EnhLevel *a, const FrameHudElement *e, int x, int y)
{
    if (e->kind == 6) return 0;
    unsigned at, stride, rows;
    const uint8_t *src = a->hud_source;
    if (e->kind < 2 || e->kind == 5) {
        at = e->kind == 1 ? 0x7d1 : 0x7b0; stride = 40; rows = 23;
        if (e->kind == 5) x = 1;
    }
    else if (e->kind == 2) { at = 0x1610 + e->index * 96; stride = 2; rows = 12; }
    else if (e->kind == 3) { at = 0x1c70 + e->index * 88; stride = 2; rows = 11; }
    else {
        unsigned frame = (uint16_t)(0xf1 + (int8_t)e->index);
        if (!a->cels || frame >= 1024 || !a->cels[frame].blocks) return 0;
        src = a->cels[frame].blocks; at = 22; stride = 2; rows = 11;
    }
    uint8_t c = 0;
    for (unsigned p = 0; p < 4; p++)
        c |= (uint8_t)(((src[(uint16_t)(at + p*stride*rows + y*stride + x/8)] >> (7-x%8)) & 1) << p);
    return c;
}
/* Anchored layout: lives keeps the left corner, the score (which spans the
 * original centre) stays centred, energy and BONUS keep the right corner. */
static int hud_field_offset(unsigned field, int w)
{
    return field == HUD_FIELD_LIVES ? 0 : field == HUD_FIELD_SCORE ? (w-320)/2 : w-320;
}
static void hud(const FrameGameView *v, const EnhLevel *a, EnhHud mode, int w, int top, EnhImage *im)
{
    /* Only the art's plain border/interior column is repeated. Static labels
     * remain asset draws, and dirty glyphs keep their original corner. */
    FrameHudElement frame = {0};
    for (int y = 0; y < 24; y++) for (int x = 0; x < w; x++)
        im->index[(size_t)(top+y)*im->stride+x] = y < 23 && v->hud_frame ? hud_pixel(a, &frame, 1, y) : 0;
    for (unsigned i = 0; i < v->hud_count; i++) {
        const FrameHudElement *e = &v->hud_elements[i];
        if (e->kind == 6 && e->x0 == 0 && e->x1 == 320) {
            for (int y = e->y0; y < e->y1; y++) if (y >= 0 && y < 24)
                memset(im->index+(size_t)(top+y)*im->stride, 0, (size_t)w);
            continue;
        }
        int ox = mode == ENH_HUD_LEFT ? 0 : mode == ENH_HUD_RIGHT ? w-320 :
                 mode == ENH_HUD_ANCHORED ? hud_field_offset(e->field, w) : (w-320)/2;
        for (int y = e->y0; y < e->y1; y++) for (int x = e->x0; x < e->x1; x++)
            if (x+ox >= 0 && x+ox < w && y >= 0 && y < 24)
                im->index[(size_t)(top+y)*im->stride+x+ox] = hud_pixel(a, e, x-e->x, y-e->y);
    }
}
/* dx/dy are presentation displacement from each source world's own camera.
 * The old room and the already-drawn arrival can have unrelated cameras. */
int enh_render_surface(const EnhSurface *s, const uint8_t palette[256][3], uint8_t pel_mask,
                       double dx, double dy, int w, int h, const EnhOptions *opt, EnhImage *im)
{
    if (!s) {
        for (int y = 0; y < h; y++) memset(im->index+(size_t)y*im->stride, 0, (size_t)w);
        return 0;
    }
    if (s->world) {
        const FrameGameView *v = s->world;
        if (v->valid && v->world_valid) {
            if (enh_render(v, v->assets, palette, pel_mask, v->camera_x+dx, v->camera_y+dy, w, h, opt, im)) return -1;
        } else {
            for (int y = 0; y < h; y++) memset(im->index+(size_t)y*im->stride, 0, (size_t)w);
            if (opt->hud != ENH_HUD_NONE && v->assets) hud(v, v->assets, opt->hud, w, h-24, im);
        }
    } else if (enh_render_surface(s->base, palette, pel_mask, dx, dy, w, h, opt, im)) return -1;
    for (int y = 0; y < 200 && y < h; y++) {
        const EnhRowMask *row = &s->rows[y];
        int oy = y < 176 ? y-(int)floor(dy) : y;
        if (oy < 0 || oy >= h) continue;
        for (unsigned i = 0; i < row->count; i++) {
            int left = row->left[i] ? row->left[i]-(int)floor(dx) : 0;
            int right = row->right[i] == 320 ? w : row->right[i]-(int)floor(dx);
            if (left < 0) left = 0;
            if (right > w) right = w;
            if (left < right) memset(im->index+(size_t)oy*im->stride+left, 0, (size_t)(right-left));
        }
        int aperture_left=-(int)floor(dx), aperture_right=320-(int)floor(dx);
        if (s->outside_valid[y]&1) {
            int edge=s->outside_left[y]-(int)floor(dx);
            if(edge>aperture_left) edge=aperture_left;
            if(edge>w) edge=w;
            if(edge>0) memset(im->index+(size_t)oy*im->stride,0,(size_t)edge);
        }
        if (s->outside_valid[y]&2) {
            int edge=s->outside_right[y]-(int)floor(dx);
            if(edge<aperture_right) edge=aperture_right;
            if(edge<0) edge=0;
            if(edge<w) memset(im->index+(size_t)oy*im->stride+edge,0,(size_t)(w-edge));
        }
    }
    if (s->reveal) {
        uint8_t *pixels = malloc((size_t)w*h); if (!pixels) return -1;
        EnhImage source = {0}; source.index = pixels; source.capacity = (size_t)w*h; source.stride = w;
        int rc = enh_render_surface(s->reveal, palette, pel_mask, dx, dy, w, h, opt, &source);
        if (!rc) for (int y = 0; y < h && y < 176; y++) for (int x = 0; x < w; x++)
            if (s->columns & (1u << ((int64_t)x*20/w))) im->index[(size_t)y*im->stride+x] = pixels[y*w+x];
        free(pixels); if (rc) return rc;
    }
    const EnhSurface *root = s;
    while (root && !root->world) root = root->reveal ? root->reveal : root->base;
    const EnhLevel *a = root && root->world ? root->world->assets : NULL;
    if (a && a->cels) for (unsigned i = 0; i < s->redraw_count; i++) {
        const FrameSprite *sp = &s->redraw[i];
        if (sp->frame < 1024) sprite(sp, &a->cels[sp->frame], sp->screen_x-(int)floor(dx),
            sp->screen_y-(int)floor(dy), w, h < 176 ? h : 176, im);
    }
    memcpy(im->palette, palette, sizeof im->palette); im->pel_mask = pel_mask;
    return 0;
}
int enh_render_iris(const FrameGameView *v, const uint8_t palette[256][3], uint8_t pel_mask,
                    double cx, double cy, int w, int h, const EnhOptions *opt,
                    double ix, double iy, double radius, EnhImage *im)
{
    FrameGameView base = *v;
    base.surface = v->surface ? v->surface->base : NULL;
    if (enh_render(&base, v->assets, palette, pel_mask, cx, cy, w, h, opt, im)) return -1;
    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++)
        if ((x-ix)*(x-ix)+(y-iy)*(y-iy) > radius*radius) im->index[(size_t)y*im->stride+x] = 0;
    if (v->surface && v->assets->cels) for (unsigned i = 0; i < v->surface->redraw_count; i++) {
        const FrameSprite *sp = &v->surface->redraw[i];
        if (sp->frame < 1024) sprite(sp, &v->assets->cels[sp->frame], sp->screen_x+v->camera_x-(int)floor(cx),
            sp->screen_y+v->camera_y-(int)floor(cy), w, h < 176 ? h : 176, im);
    }
    return 0;
}
static void surface_hud_mask(const EnhSurface *s, int w, int top, double dx, EnhImage *im)
{
    if (!s) return;
    surface_hud_mask(s->base, w, top, dx, im);
    for (int y = 176; y < 200; y++) {
      for (unsigned i = 0; i < s->rows[y].count; i++) {
        int left = s->rows[y].left[i], right = s->rows[y].right[i];
        left = left ? left-(int)floor(dx) : 0; right = right==320 ? w : right-(int)floor(dx);
        if (left < 0) left = 0;
        if (right > w) right = w;
        if (left < right) memset(im->index+(size_t)(top+y-176)*im->stride+left, 0, (size_t)(right-left));
      }
      if (s->outside_valid[y]&1) {
        int edge=s->outside_left[y]-(int)floor(dx); if(edge>w) edge=w;
        if(edge>0) memset(im->index+(size_t)(top+y-176)*im->stride,0,(size_t)edge);
      }
      if (s->outside_valid[y]&2) {
        int edge=s->outside_right[y]-(int)floor(dx); if(edge<0) edge=0;
        if(edge<w) memset(im->index+(size_t)(top+y-176)*im->stride+edge,0,(size_t)(w-edge));
      }
    }
}
int enh_render(const FrameGameView *v, const EnhLevel *a, const uint8_t palette[256][3], uint8_t pel_mask,
               double camera_x, double camera_y, int w, int h, const EnhOptions *opt, EnhImage *im)
{
    EnhOptions defaults = { ENH_BACK_BLACK, ENH_HUD_NONE, NULL, 0 }; if (!opt) opt = &defaults;
    int cx, cy;
    if (!v || !v->valid || (!v->world_valid && !v->surface) || !a || v->assets != a || (!v->map && !v->surface) || !palette || !im || !im->index ||
        w <= 0 || h <= 0 || w > 16384 || h > 16384 || im->stride < (size_t)w ||
        im->stride > SIZE_MAX / (size_t)h || im->capacity < im->stride * (size_t)h ||
        v->sprite_count > FRAME_MAX_SPRITES || v->dot_count > FRAME_MAX_DOTS || v->foreground_count > FRAME_MAX_FOREGROUND ||
        v->hud_count > FRAME_MAX_HUD ||
        a->rows > 256 || a->policy.end_column > 256 ||
        (opt->position_count && !opt->positions) || floor_int(camera_x, &cx) || floor_int(camera_y, &cy)) return -1;
    int ph = h - (opt->hud == ENH_HUD_NONE ? 0 : 24); if (ph < 0) return -1;
    if (v->surface) {
        int rc = enh_render_surface(v->surface, palette, pel_mask, camera_x-v->camera_x, camera_y-v->camera_y, w, h, opt, im);
        if (!rc && opt->hud != ENH_HUD_NONE) {
            hud(v, a, opt->hud, w, ph, im); surface_hud_mask(v->surface, w, ph, camera_x-v->camera_x, im);
        }
        return rc;
    }
    memcpy(im->palette, palette, sizeof im->palette); im->pel_mask = pel_mask;
    for (int y = 0; y < ph; y++) for (int x = 0; x < w; x++) {
        uint8_t c = back(a, x, y - v->shake_rows, w, opt->backdrop);
        int wx = cx + x, wy = cy + y - v->shake_rows;
        int col = divfloor(wx, 16), row = divfloor(wy, 16);
        if (col >= a->policy.first_column && col < a->policy.end_column && row >= 0 && row < a->rows) {
            unsigned t = v->map->rows[row][col];
            if (a->animated[t]) t = v->animation[t];
            unsigned cls = a->tile_class[t];
            int pc = col - v->camera_x / 16, pr = row - (v->camera_y - v->fine_scroll) / 16;
            if (pc >= 0 && pc < 20 && pr >= 0 && pr < 12) {
                const FrameTilePaint *paint = &v->paint[pr * 20 + pc];
                if (paint->valid) {
                    t = paint->tile; cls = paint->cls;
                    if (cls) {
                        int bx = paint->back_x + wx - col * 16, by = paint->back_y + wy - row * 16;
                        /* The backdrop is screen-fixed. Retarget the sampled patch
                         * when presentation moves away from the simulation camera. */
                        bx += v->camera_x - cx; by += v->camera_y - cy;
                        c = back(a, bx, by, w, opt->backdrop);
                    }
                }
            }
            uint8_t pixel = a->tiles[t][(wy - row * 16) * 16 + wx - col * 16];
            if (cls == 0 || (cls >= 2 && pixel)) c = pixel;
        } else c = 0;
        if (y < v->shake_rows) c = 0;
        im->index[(size_t)y * im->stride + x] = c;
    }
    int dx = v->camera_x - cx, dy = v->camera_y - cy;
    unsigned regular_count=v->sprite_count;
    while (regular_count && v->sprites[regular_count-1].margin_only) regular_count--;
    /* Appended source snapshots are a rear pass. The original sprite/dot order
     * and indices remain intact, including dots after the last regular sprite. */
    for (unsigned pass_index=0;pass_index<2;pass_index++) {
      unsigned begin=pass_index?0:regular_count, end=pass_index?regular_count:v->sprite_count;
      for (unsigned i=begin;i<end;i++) {
        if (pass_index) dots(v,i,dx,dy,w,ph,im);
        const FrameSprite *s=&v->sprites[i]; if (!a->cels || s->frame>=1024) continue;
        double px=s->screen_x+(s->screen_space?0:v->camera_x-camera_x);
        double py=s->screen_y+(s->screen_space?0:v->camera_y-camera_y);
        if (!s->screen_space) for (size_t k=0;k<opt->position_count;k++) if (opt->positions[k].id==s->id) {
            px+=opt->positions[k].x-s->world_x; py+=opt->positions[k].y-s->world_y; break;
        }
        int x,y; if (floor_int(px,&x) || floor_int(py,&y)) return -1;
        const EnhCel *cel=&a->cels[s->frame];
        if (x>=w || x+cel->w<0 || y>=ph || y+cel->h<=0) continue;
        if (!s->margin_only) sprite(s,cel,x,y,w,ph,im);
        else {
            /* Sprite erase/OR arithmetic is unchanged. Preserve its overlap with
             * the simulation window, including a byte-shift carry at either
             * edge; only pixels beyond that window belong to this rear pass. */
            uint8_t saved[320*176];
            int x0=dx<0?0:dx, x1=dx+320>w?w:dx+320;
            int y0=dy<0?0:dy, y1=dy+176>ph?ph:dy+176;
            if (x0<x1 && y0<y1) for (int r=y0;r<y1;r++)
                memcpy(saved+(r-y0)*320,im->index+(size_t)r*im->stride+x0,(size_t)(x1-x0));
            sprite(s,cel,x,y,w,ph,im);
            if (x0<x1 && y0<y1) for (int r=y0;r<y1;r++)
                memcpy(im->index+(size_t)r*im->stride+x0,saved+(r-y0)*320,(size_t)(x1-x0));
        }
      }
    }
    /* Foreground requests are world cells, not a raster padded to 320 pixels.
     * Outside the original aperture regenerate the sprite-neighbour requests. */
    if (v->foreground_enabled) for (unsigned i = 0; i < v->sprite_count; i++) {
        const FrameSprite *s = &v->sprites[i]; if (s->screen_space) continue;
        double ax = s->world_x, ay = s->world_y;
        for (size_t k = 0; k < opt->position_count; k++) if (opt->positions[k].id == s->id) { ax = opt->positions[k].x; ay = opt->positions[k].y; break; }
        int ix, iy; if (floor_int(ax, &ix) || floor_int(ay, &iy)) return -1;
        int col = divfloor(ix, 16), row = divfloor(iy, 16);
        for (int r = row - 3; r <= row - (iy % 16 == 0); r++) for (int c = col - 2; c <= col + 1; c++) {
            if (r < 0 || r >= a->rows || c < a->policy.first_column || c >= a->policy.end_column) continue;
            unsigned t = v->map->rows[r][c];
            if (a->flags[t] & 0x40) front_tile(a, t, c * 16 - cx, r * 16 - cy, w, ph, im,
                                             opt->position_count && !s->margin_only ? NULL : v, cx, cy);
        }
    }
    /* Overrides regenerate the complete neighbourhood; replaying old requests
     * too would leave foreground graphics at the previous sprite position. */
    for (unsigned i = 0; !opt->position_count && i < v->foreground_count; i++) {
        const FrameForeground *f = &v->foreground[i];
        int ox = (f->cell & 255) * 16 - cx, oy = (f->cell >> 8) * 16 - cy;
        front_tile(a, f->tile, ox, oy, w, ph, im, NULL, cx, cy);
    }
    dots(v, regular_count, dx, dy, w, ph, im);
    if (opt->hud != ENH_HUD_NONE) hud(v, a, opt->hud, w, ph, im);
    return 0;
}
