#include "enh_render.h"
#include "level_regions.h"
#include <math.h>
#include <string.h>
#include <limits.h>

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
    if (mode == ENH_BACK_STRETCH) sx = (int)((int64_t)x * 320 / w);
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
int enh_render(const FrameGameView *v, const EnhLevel *a, const uint8_t palette[256][3], uint8_t pel_mask,
               double camera_x, double camera_y, int w, int h, const EnhOptions *opt, EnhImage *im)
{
    EnhOptions defaults = { ENH_BACK_BLACK, ENH_HUD_NONE, NULL, 0 }; if (!opt) opt = &defaults;
    int cx, cy;
    if (!v || !v->valid || !v->world_valid || !a || v->assets != a || !v->map || !palette || !im || !im->index ||
        w <= 0 || h <= 0 || w > 16384 || h > 16384 || im->stride < (size_t)w ||
        im->stride > SIZE_MAX / (size_t)h || im->capacity < im->stride * (size_t)h ||
        v->sprite_count > FRAME_MAX_SPRITES || v->dot_count > FRAME_MAX_DOTS || v->foreground_count > FRAME_MAX_FOREGROUND ||
        a->rows > 256 || a->policy.end_column > 256 ||
        (opt->position_count && !opt->positions) || floor_int(camera_x, &cx) || floor_int(camera_y, &cy)) return -1;
    int ph = h - (opt->hud == ENH_HUD_NONE ? 0 : 24); if (ph < 0) return -1;
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
    if (opt->hud != ENH_HUD_NONE) {
        int ox = opt->hud == ENH_HUD_LEFT ? 0 : opt->hud == ENH_HUD_RIGHT ? w - 320 : (w - 320) / 2;
        for (int y = 0; y < 24; y++) for (int x = 0; x < w; x++) {
            int sx = x - ox; if (sx < 0) sx = 1; if (sx >= 320) sx = 318;
            im->index[(size_t)(ph + y) * im->stride + x] = v->hud[y * 320 + sx];
        }
    }
    return 0;
}
