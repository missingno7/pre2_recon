/* Presentation-only world renderer. Camera denotes the TOP LEFT world pixel, not
 * the centre. Fractional coordinates use floor sampling of the indexed image. */
#ifndef PRE2_ENH_RENDER_H
#define PRE2_ENH_RENDER_H
#include "frame.h"
#include <stddef.h>

typedef struct EnhCel {
    uint16_t w, h, row;
    uint32_t size;
    const uint8_t *blocks; /* mask + four colour blocks; owned 64K source window */
    const uint8_t *pixels, *coverage; /* decoded indexed pixels and clear mask */
} EnhCel;
typedef struct EnhMap { const uint8_t *rows[256]; } EnhMap;
typedef struct EnhLevelPolicy {
    uint16_t first_column, end_column; /* half-open playable backing columns */
    uint8_t fixed_room, graphics_columns; /* nonzero = confirmed policy; zero imposes none */
} EnhLevelPolicy;
typedef struct EnhLevel {
    uint32_t epoch;
    uint8_t level, backdrop_scroll; /* 0 = screen fixed (RestoreTileBackdrop) */
    uint16_t backdrop_period; /* horizontal period / audited full-picture seam; 0 = user policy */
    uint16_t columns, rows;
    EnhLevelPolicy policy;
    uint8_t map[65536], tiles[256][256], tile_class[256], flags[256];
    uint8_t animation[3][256], animated[256], front[256][256];
    uint8_t backdrop[320 * 200], hud_source[65536];
    const EnhCel *cels; /* 1024 entries */
    uint64_t content_hash;
} EnhLevel;

typedef enum EnhBackdrop { ENH_BACK_BLACK, ENH_BACK_MIRROR, ENH_BACK_STRETCH } EnhBackdrop;
typedef enum EnhHud { ENH_HUD_NONE, ENH_HUD_LEFT, ENH_HUD_CENTRE, ENH_HUD_RIGHT, ENH_HUD_ANCHORED } EnhHud;
typedef struct EnhPosition { uint32_t id; double x, y; } EnhPosition;
typedef struct EnhOptions {
    EnhBackdrop backdrop;
    EnhHud hud; /* reserves the bottom 24 rows; NONE makes height all playfield */
    const EnhPosition *positions;
    size_t position_count;
} EnhOptions;
typedef struct EnhImage {
    uint8_t *index; size_t capacity, stride; /* caller-owned; stride >= width */
    uint8_t palette[256][3], pel_mask;
} EnhImage;

/* Destructive gameplay pages retain semantic sources and a union of cleared
 * intervals, never faithful pixels. A curtain selects columns of an already
 * drawn destination. Iris redraws retain the original post-blackout sprite pass. */
#define ENH_ROW_SPANS 4
typedef struct EnhRowMask { uint16_t count, left[ENH_ROW_SPANS], right[ENH_ROW_SPANS]; } EnhRowMask;
typedef struct EnhSurface {
    const EnhLevel *owner;
    const FrameGameView *world;
    const struct EnhSurface *base, *reveal;
    uint32_t columns;
    EnhRowMask rows[200];
    int16_t outside_left[200], outside_right[200];
    uint8_t outside_valid[200]; /* original iris boundaries outside the 320 aperture */
    const FrameSprite *redraw;
    uint16_t redraw_count;
} EnhSurface;
int enh_render_surface(const EnhSurface *surface, const uint8_t palette[256][3], uint8_t pel_mask,
                       double camera_dx, double camera_dy, int width, int height,
                       const EnhOptions *options, EnhImage *out);
int enh_render_iris(const FrameGameView *view, const uint8_t palette[256][3], uint8_t pel_mask,
                    double camera_x, double camera_y, int width, int height,
                    const EnhOptions *options, double x, double y, double radius, EnhImage *out);

/* Returns 0 on success, -1 for invalid dimensions/buffer/asset/coordinates.
 * No live game state, faithful image, or compositor storage is read. */
int enh_render(const FrameGameView *view, const EnhLevel *level,
               const uint8_t palette[256][3], uint8_t pel_mask,
               double camera_x, double camera_y, int width, int height,
               const EnhOptions *options, EnhImage *out);
/* Epoch lookup is scoped to the current compositor session. Across reset, use
 * the retained frame's assets pointer: a new session may reuse epoch numbers. */
const EnhLevel *enh_level_lookup(uint32_t epoch);
EnhLevelPolicy enh_level_policy(unsigned level);
/* The session registry owns assets until compositor_init. Retain an asset when
 * keeping any frames across that boundary; release after the last such frame. */
void enh_level_retain(const EnhLevel *level);
void enh_level_release(const EnhLevel *level);
/* Content hash (native pointers excluded), for purity and production checks. */
uint64_t enh_view_hash(const FrameGameView *view, uint64_t seed);
#endif
