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
    uint8_t  margin_only;   /* source snapshot: behind regular sprites, outside simulation window only */
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
struct EnhSurface;

/* Opaque asset draws in page order. The clip is in the original HUD's 320x24
 * coordinates; copies preserve it independently of the command's asset origin. */
typedef struct FrameHudElement {
    int16_t x, y, x0, y0, x1, y1;
    uint16_t index;
    uint8_t kind; /* 0 static art/label, 1 BONUS, 2 glyph, 3 digit, 4 item/letter, 5 frame pattern, 6 clear */
    uint8_t field; /* FrameHudField that owns it: wide layouts move whole fields, never single glyphs */
} FrameHudElement;
/* The status bar's fields, from UpdateHud's destinations (c/level.c) in the 320x23 strip:
 * lives count glyph at strip byte 16Dh (x 40), score digits from 171h (x 72: six formatted
 * digits plus the fixed trailing 0, seven 16-pixel cells to x 184), energy pips from 181h
 * (x 200, three cells), BONUS letters in the 48x23 box ClearBonusLetterRow restores at 21h
 * (x 264). Each field owns its icon/label art; the columns between fields are the plain bar
 * pattern. Spans: lives 0..64, score 64..192, energy 192..256, BONUS 256..320. */
typedef enum FrameHudField { HUD_FIELD_LIVES, HUD_FIELD_SCORE, HUD_FIELD_ENERGY, HUD_FIELD_BONUS, HUD_FIELDS } FrameHudField;
static const int16_t frame_hud_field_x[HUD_FIELDS + 1] = { 0, 64, 192, 256, 320 };
#define FRAME_MAX_HUD 256
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
    uint16_t hud_count;
    uint8_t hud_frame; /* the page has received the status-frame pattern */
    FrameHudElement hud_elements[FRAME_MAX_HUD];
    uint8_t hud[320 * 24]; /* verification reference only; never a drawing source */
    const struct EnhSurface *surface; /* retained destructive page operations */
} FrameGameView;

/* Transition state, for presentation that wants to smooth the original's discrete steps. */
typedef struct FrameTransition {
    uint8_t kind;                   /* 0 none, 1 iris, 2 column reveal, 3 block wipe, 4 palette fade, 5 map scroll */
    uint8_t phase;                  /* 0 closing, 1 covered, 2 opening */
    uint16_t step, steps;           /* original discrete progress */
    int16_t cx, cy, radius;         /* iris centre/radius (screen pixels) */
} FrameTransition;

/* Native screen authoring surfaces and their semantic projection. Assets are
 * immutable, reference-counted native objects (never pointers into the DOS machine).
 * A surface is the retained result of picture/glyph/fill/copy operations; it is
 * independent of the faithful compositor and its published image. Retain/release
 * copied views through enh_screens.h; retained images survive compositor_init(). */
struct FrameScreenPixels;
typedef enum FrameScreenLayerKind {
    FRAME_SCREEN_IMAGE, FRAME_SCREEN_CLEAR, FRAME_SCREEN_COPY,
    FRAME_SCREEN_GLYPH, FRAME_SCREEN_FONT, FRAME_SCREEN_COLUMN,
    FRAME_SCREEN_EDGE, FRAME_SCREEN_CEL
} FrameScreenLayerKind;
/* Rectangles in a native authoring image (pitch in pixels). A row's first
 * pixel wraps by row_mask; individual pixels wrap by the image size. */
typedef struct FrameScreenRect {
    uint32_t origin, width, height, pitch, row_mask;
} FrameScreenRect;
typedef struct FrameScreenLayer {
    const struct FrameScreenLayer *previous;
    const struct FrameScreenPixels *image;
    FrameScreenRect source, destination;
    uint8_t kind, channels, character;
    uint16_t asset, source_skip;
    uint8_t mirror, pass, phase, clipped, clip_left, clip_right;
    int32_t x, y, previous_x, previous_y;
} FrameScreenLayer;
typedef enum FrameScreenKind {
    FRAME_SCREEN_EMPTY, FRAME_SCREEN_PICTURE, FRAME_SCREEN_MENU,
    FRAME_SCREEN_WORLD_MAP, FRAME_SCREEN_CREDITS, FRAME_SCREEN_INTRO,
    FRAME_SCREEN_TALLY, FRAME_SCREEN_GAME_OVER
} FrameScreenKind;
typedef struct FrameScreenRow {
    uint32_t origin;                 /* authoring-surface pixel origin */
    uint32_t mask;                   /* wrapping surface extent minus one */
    uint16_t count;                  /* original active columns */
    uint8_t side_clear;              /* a whole-row clear also covers wide sides */
} FrameScreenRow;
typedef struct FrameScreenView {
    uint8_t valid, kind, blank, border;
    uint16_t width, height;
    uint32_t epoch;
    double pan_x, pan_y;             /* published pan, in pixels / rows */
    uint16_t revealed[4];            /* map master columns uploaded in each channel */
    uint8_t colour[16], indexed256;
    const struct FrameScreenPixels *surface;
    const struct FrameScreenPixels *master; /* stamped 640 x 200 map */
    const struct FrameScreenPixels *font_banks;
    const FrameScreenLayer *layers;  /* bounded operations since the previous publication; newest first */
    FrameScreenRow rows[FRAME_MAX_H];
} FrameScreenView;

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
    FrameScreenView screen_view;
} Pre2Frame;

/* XRGB8888 of a frame (the faithful presenter's palette lookup). */
void frame_to_xrgb(const Pre2Frame *f, uint32_t *out);
#endif
