/* The published presentation model (docs/native.md, "Presentation").
 *
 * One Pre2Frame per displayed CRT frame (70.1 Hz, at its display start in logical time),
 * immutable once published: the faithful image as palette indices with the palette in force,
 * and, while a gameplay view is on screen, its semantic description for presentation
 * enhancements (interpolation, widescreen, smooth camera) that draw from assets themselves.
 * Nothing here refers to DOS addresses or VGA registers; drawing a frame zero, one or many
 * times changes nothing in the game.
 */
#ifndef PRE2_FRAME_H
#define PRE2_FRAME_H
#include <stdint.h>

#define FRAME_MAX_W 640
#define FRAME_MAX_H 480
#define FRAME_MAX_SPRITES 1024
/* Wide identities reserve more than the maximum logical pool slots. */
#define FRAME_ID_SLOT_STRIDE 2048u
/* Shipped wind scripts reach 255 flakes (up to three dots each), plus twenty
 * consumed particles and twenty fireflies in the same world pass. Wide mode
 * bounds one-shot points at 256; all bounded sources fit within 1024 dots. */
#define FRAME_MAX_DOTS 1024

/* One sprite as the faithful pass drew it (draw order = array order). */
typedef struct FrameSprite {
    uint16_t slot;          /* render-slot index (0 = the topmost record) */
    uint32_t id;            /* stable instance identity: slot + generation (see gen) */
    uint32_t gen;           /* bumps when the slot is reused for a new instance */
    uint16_t frame;         /* sprite frame number */
    uint8_t  mirror, mode;  /* mode: 0 clear only (blink off), 1 normal, 2 flash */
    int32_t  world_x, world_y;      /* the record's position (its anchor) */
    int32_t  screen_x, screen_y;    /* top-left where the faithful pass placed it */
    uint8_t  screen_space;  /* drawn without the camera (frame 309, HUD-like records) */
} FrameSprite;

/* A plotted point: particle, firefly or flake. */
typedef struct FrameDot {
    int32_t x, y;
    uint64_t id;                 /* stable source + generation + piece ordinal (wide variant) */
    uint32_t gen;
    uint8_t colour, kind, or_colour;
    uint16_t before_sprite;
} FrameDot;

struct EnhLevel;
struct EnhMap;
typedef struct FrameForeground { uint16_t cell; uint8_t tile; } FrameForeground;
/* Last evaluated tile paint at a world cell. Edge loads intentionally use the
 * raw animation cel until the next animation pass. Panel patches live here too. */
typedef struct FrameTilePaint { uint16_t cell; uint8_t tile, cls; int16_t back_x, back_y; uint8_t valid; } FrameTilePaint;
/* 116 slots, at most four rows by four neighbours per original request walk. */
#define FRAME_MAX_FOREGROUND (116 * 16)

typedef struct FrameGameView {
    uint8_t  valid;
    uint8_t  level;                 /* CurrentLevel */
    uint32_t level_epoch;           /* bumps at each level load (asset/map generation) */
    uint32_t sim_frame;             /* main-loop frame that produced this view */
    int32_t  camera_x, camera_y;    /* simulation camera in world pixels (column*16, row*16 + fine) */
    int16_t  shake_rows;
    uint16_t view_w, view_h;        /* the playfield (320 x 176) */
    uint8_t  camera_cut;            /* the camera jumped (respawn, warp, room change) */
    uint16_t sprite_count, dot_count;
    FrameSprite sprites[FRAME_MAX_SPRITES];
    FrameDot dots[FRAME_MAX_DOTS];
    /* Immutable native level/map references; see enh_render.h for retention.
     * Rows are shared until changed. Neither reference points into game memory. */
    const struct EnhLevel *assets;
    const struct EnhMap *map;
    uint8_t animation[256], fine_scroll, level_flags, room;
    uint8_t foreground_enabled, world_valid; /* 0 = clear/tally surface, not a world view */
    uint16_t camera_limit_column; /* original conditional right-scroll threshold, not a hard maximum */
    uint16_t camera_limit_row, foreground_count;
    FrameTilePaint paint[240]; /* semantic 20 x 12 cells; no pixel/page addresses */
    FrameForeground foreground[FRAME_MAX_FOREGROUND];
    uint8_t hud[320 * 24]; /* actual page-local HUD, including partial updates */
} FrameGameView;

/* Transition state, for presentation that wants to smooth the original's discrete steps. */
typedef struct FrameTransition {
    uint8_t kind;                   /* 0 none, 1 iris, 2 column reveal, 3 block wipe, 4 palette fade, 5 map scroll */
    uint8_t phase;                  /* 0 closing, 1 covered, 2 opening */
    uint16_t step, steps;           /* original discrete progress */
    int16_t cx, cy, radius;         /* iris centre/radius (screen pixels) */
} FrameTransition;

typedef struct Pre2Frame {
    uint64_t seq;                   /* display frame number */
    uint64_t t_ns;                  /* logical time of its display start */
    uint16_t w, h;                  /* 320x200 or 640x480 (640x350) */
    uint8_t  blank;                 /* output blanked (shows the border colour) */
    uint8_t  index[FRAME_MAX_W * FRAME_MAX_H];   /* DAC indices */
    uint8_t  palette[256][3];       /* 6-bit RGB in force at display */
    uint8_t  pel_mask;
    const char *screen;             /* the top-level step showing it (diagnostics) */
    FrameTransition transition;
    FrameGameView game;
} Pre2Frame;

/* XRGB8888 of a frame (the faithful presenter's palette lookup). */
void frame_to_xrgb(const Pre2Frame *f, uint32_t *out);
#endif
