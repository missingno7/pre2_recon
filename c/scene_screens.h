/* Screen presentation events (kinds 400..699; see scene.h). Owned by the screens renderer
 * work: pictures (mode 0Dh/12h/13h), fades that rewrite pixels, map and callback screens,
 * fonts and text, intro scene, game over, credits. */
#ifndef SCENE_SCREENS_H
#define SCENE_SCREENS_H
#include "scene.h"
enum SceneScreensKind {
    /* src segment, src offset, destination, bytes per plane, plane mask, chain4.
     * Captures the source into native ownership before any subsequent asset reuse. */
    SCENE_SCREEN_PICTURE = 400,
    /* destination, byte count: colour-zero fill in all planes. */
    SCENE_SCREEN_CLEAR,
    /* source, destination, byte count: forward semantic copy (overlap and 16-bit wrap). */
    SCENE_SCREEN_COPY,
    /* ALLFONTS segment, character, destination: 8x12 credits glyph (space clears). */
    SCENE_SCREEN_CHAR,
    /* Shifted-font asset capture: segment (all eight phase banks, 64 KiB). */
    SCENE_SCREEN_FONT_BANKS,
    /* string DS offset, wrap mask, cursor, advance, font bank, current start, previous start. */
    SCENE_SCREEN_STRING,
    /* Stamped world-map asset capture: segment (four planes, 80 bytes x 200 rows). */
    SCENE_SCREEN_WORLD_MAP,
    /* World-map rolling column plane pass: old scroll x, plane mask.
     * Destination wraps at 8 KiB per row; inter-plane address carry is preserved. */
    SCENE_SCREEN_WORLD_COLUMN,
    /* Callback-map edge copies: start, previous x, current x, previous y, current y. */
    SCENE_SCREEN_MAP_EDGES,
    /* Transition: kind (0 none, 4 palette fade, 5 map scroll), phase, step, steps, x, y.
     * Fade steps are passes (32 opening / 16 closing); map x/y are the published pixel
     * scroll position. World reveal has 640 steps; cyclic callback scroll has steps=0. */
    SCENE_SCREEN_TRANSITION,
};

/* Native presentation hook. The integrator chains this to its gameplay transition hook. */
struct FrameTransition;
void draw_screens_transition(struct FrameTransition *t);
#endif
