#include "enh_screens.h"
#include <math.h>
#include <string.h>
#include <limits.h>

static uint8_t pixel(const FrameScreenPixels *image, uint32_t at)
{
    if (!image || at >= image->size) return 0;
    return image->block[at / SCREEN_PIXEL_BLOCK][at % SCREEN_PIXEL_BLOCK];
}

int enh_screens_render(const FrameScreenView *v,
                       const uint8_t palette[256][3], uint8_t pel_mask,
                       double pan_x, double pan_y, unsigned w, unsigned h,
                       EnhImage *out)
{
    if (!v || !v->valid || !palette || !out || !out->index || !v->surface ||
        w < v->width || w > 2048 || w < 320 || h != v->height || h > FRAME_MAX_H ||
        (v->width != 320 && (w != v->width || h != v->height)) ||
        !isfinite(pan_x) || !isfinite(pan_y) || fabs(pan_x) > INT_MAX / 4 ||
        fabs(pan_y) > INT_MAX / 1024 || out->stride < w ||
        out->stride > SIZE_MAX / (h ? h : 1) ||
        !h || out->capacity < (size_t)(h - 1) * out->stride + w) return -1;
    memcpy(out->palette, palette, sizeof out->palette); out->pel_mask = pel_mask;
    int dx = (int)floor(pan_x) - (int)v->pan_x;
    int dy = (int)floor(pan_y) - (int)v->pan_y;
    int pad = (int)(w - v->width) / 2;
    /* The unused sides of a finite picture are literal black, whereas row
     * clears use logical colour zero. All shipped screen palettes contain black. */
    uint8_t black = 0;
    for (unsigned i = 0; i < 256; i++) if (!(palette[i & pel_mask][0] |
        palette[i & pel_mask][1] | palette[i & pel_mask][2])) { black = (uint8_t)i; break; }
    for (unsigned y = 0; y < h; y++) {
        const FrameScreenRow *row = &v->rows[y];
        uint8_t *line = out->index + y * out->stride;
        for (unsigned x = 0; x < w; x++) {
            if (v->blank) { line[x] = v->border; continue; }
            int sx = (int)x - pad;
            uint8_t c = black;
            if (v->kind == FRAME_SCREEN_MENU) {
                /* The low channels form contiguous rows of a repaired linear
                 * ring, rather than a periodically repeated 320-column picture.
                 * Text stays in the authored aperture, including its byte fringe. */
                unsigned border = v->width - row->count;
                if (x >= w - border) { line[x] = v->border; continue; }
                uint32_t at = (uint32_t)((int64_t)row->origin + sx + dx + (int64_t)320 * dy) & row->mask;
                uint8_t bits = pixel(v->surface, at) & 3;
                if (sx >= 0 && sx < row->count)
                    bits |= pixel(v->surface, (row->origin + (unsigned)sx) & row->mask) & 12;
                c = v->colour[bits];
            } else if (v->kind == FRAME_SCREEN_WORLD_MAP) {
                /* Keep the original right reveal edge and eight-column border.
                 * Widen leftwards into the already uploaded part of the master.
                 * The authoring surface retains partial plane passes and the
                 * real ring's last row; it is an independent native draw target. */
                sx = (int)x - (int)(w - v->width);
                if (sx >= row->count) { line[x] = v->border; continue; }
                /* An interpolated pan displaces the sample (dx < 0): the columns it moves
                 * left of the surface row come from the master, never the ring's wrap. */
                if (sx + dx >= 0) {
                    uint8_t bits = pixel(v->surface, (uint32_t)((int64_t)row->origin + sx + dx + (int64_t)320 * dy) & row->mask) & 15;
                    int mx = (int)floor(pan_x) - (int)w + (int)x;
                    for (unsigned p = 0; p < 4; p++) if (mx >= v->revealed[p]) bits &= (uint8_t)~(1u << p);
                    c = v->colour[bits];
                } else {
                    int mx = (int)floor(pan_x) - (int)w + (int)x;
                    int my = (int)y + 1 + dy;
                    uint8_t bits = 0;
                    if (mx >= 0 && mx < 640 && my >= 0 && my < 200) {
                        uint8_t master = pixel(v->master, (unsigned)my * 640u + (unsigned)mx);
                        for (unsigned p = 0; p < 4; p++) if (mx < v->revealed[p]) bits |= master & (1u << p);
                    }
                    c = v->colour[bits];
                }
            } else if (sx >= 0 && sx < row->count) {
                uint8_t bits = pixel(v->surface, (row->origin + (unsigned)sx) & row->mask);
                c = v->indexed256 ? bits : v->colour[bits & 15];
            } else if (sx >= 0 && sx < v->width) c = v->border;
            else if (row->side_clear) c = v->colour[0];
            line[x] = c;
        }
    }
    return 0;
}
