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
#define FRAME_MAX_SPRITES 128
#define FRAME_MAX_DOTS 256

/* One sprite as the faithful pass drew it (draw order = array order). */
typedef struct FrameSprite {
    uint16_t slot;          /* render-slot index (0 = the topmost record) */
    uint32_t id;            /* stable instance identity: slot + generation (see gen) */
    uint32_t gen;           /* bumps when the slot is reused for a new instance */
    uint16_t frame;         /* sprite frame number */
    uint8_t  mirror, mode;  /* mode: 0 clear only (blink off), 1 normal, 2 flash */
    int16_t  world_x, world_y;      /* the record's position (its anchor) */
    int16_t  screen_x, screen_y;    /* top-left where the faithful pass placed it */
    uint8_t  screen_space;  /* drawn without the camera (frame 309, HUD-like records) */
} FrameSprite;

/* A plotted point: particle, firefly or flake. */
typedef struct FrameDot { int16_t x, y; uint8_t colour; uint8_t kind; } FrameDot;

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
