/* Presentation capture (docs/native.md, "Presentation").
 *
 * Every original drawing operation reports itself at its original site, in program order,
 * with its semantic parameters (tile number, sprite frame and position, glyph, span...), to a
 * sink the native compositor installs. The game's own raster code keeps running unchanged as
 * the carrier of its simulation effects and its video bus time: with video memory modelled it
 * is the reference the compositor is compared with (tests/native/render_parity.c); in
 * production video memory is not modelled and only the compositor draws.
 *
 * Events are presentation output, not game state; an unset sink (the differential tests)
 * makes every emit a no-op. Kinds: core 1..99 here; gameplay 100..399 (c/scene_game.h);
 * screens and pictures 400..699 (c/scene_screens.h). Each kind documents its arguments.
 */
#ifndef SCENE_H
#define SCENE_H
#include <stdint.h>

#define SCENE_MAX_ARGS 12
typedef struct SceneEvent {
    uint16_t kind;
    uint16_t n;                      /* arguments used */
    int32_t a[SCENE_MAX_ARGS];
} SceneEvent;

extern void (*pre2_scene_sink)(const SceneEvent *e);
#define SCENE_ACTIVE (pre2_scene_sink != 0)

/* scene_emit(kind, n, a0, a1, ...): n int32 arguments. */
void scene_emit(uint16_t kind, int n, ...);

enum SceneCoreKind {
    /* BIOS mode set (INT 10h AH=00h): a0 = AL (mode | 80h = keep memory). Video memory is
     * cleared as the BIOS does (vga_bios_mode_set) unless bit 7 is set. */
    SCENE_MODE_SET = 1,
};
#endif
