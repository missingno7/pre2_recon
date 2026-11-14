/* Gameplay presentation events (kinds 100..399; see scene.h). Owned by the gameplay renderer
 * work: level view, tile ring, sprites, foreground, particles, fireflies, flakes, HUD,
 * in-level transitions (iris, wipes, death, tally). */
#ifndef SCENE_GAME_H
#define SCENE_GAME_H
#include "scene.h"
enum SceneGameKind {
    /* No arguments: capture expanded sprite frames, HUD and foreground assets. */
    SCENE_GAME_ASSETS = 100,
    /* No arguments: capture the explicitly mirrored tile cache after an upload pass. */
    SCENE_GAME_TILES,
    /* segment: four 8000-byte planar backdrop blocks, uploaded at 7E80h. */
    SCENE_GAME_BACKDROP,
    /* segment: level stash, four consecutive 6240h-byte blocks with 16-bit source wrap. */
    SCENE_GAME_STASH,
    /* source, destination, byte width, rows, source stride, destination stride. */
    SCENE_GAME_COPY,
    /* destination, byte width, rows, stride: clear to colour zero. */
    SCENE_GAME_CLEAR,
    /* tile, destination, class: opaque copy, backdrop-only (handled separately), masked overlay. */
    SCENE_GAME_TILE,
    /* backdrop source, destination: two bytes x 16 rows; destination wraps in the ring. */
    SCENE_GAME_RESTORE,
    /* No arguments: ring window, camera, fine scroll and shake from current draw state. */
    SCENE_GAME_RING,
    /* map cell, tile: foreground overlay at its original clipped destination. */
    SCENE_GAME_FOREGROUND,
    /* slot, frame, mirror, pass (0 clear, 1 colour, 16 flash), screen x, top y, dest,
     * source offset from the cel, visible bytes | row bytes << 8,
     * visible rows, pass source skip, packed clipping (left | right<<8 | flag<<16 | edge<<24).
     * Captured before the blitter mutates scratch. */
    SCENE_GAME_SPRITE,
    /* No arguments: observe empty render slots for generation tracking. */
    SCENE_GAME_SPRITES_BEGIN,
    /* type (0 background, 1 bonus background, 2 glyph, 3 digit, 4 sprite letter), index, dest. */
    SCENE_GAME_HUD,
    /* dest, mask (flakes: second row in bits 8..15), colour, kind (0 particle, 1 firefly, 2 flake), operation
     * (0 OR colour, 1 firefly even-timer: OR channels 1..3, preserve channel zero).
     * Wide events add byte-origin screen x/y and source DATA record before disposal. */
    SCENE_GAME_DOT,
    /* x, width, y, draw page: original byte-granular iris span, including its fringe quirk. */
    SCENE_GAME_BLACKOUT,
    /* kind, phase, step, steps, cx, cy, radius (FrameTransition units). */
    SCENE_GAME_TRANSITION,
    /* reset identities (0 room change, 1 respawn): camera cut on the next ring capture. */
    SCENE_GAME_CUT,
    /* No arguments: successful level load; increments the native asset/map epoch. */
    SCENE_GAME_LEVEL,
    /* panel selector: level-nine 6x7 tile panel. */
    SCENE_GAME_PANEL,
    /* slot address: an allocation/reuse boundary, even if empty was never displayed. */
    SCENE_GAME_ALLOC,
    /* No arguments: gameplay pixels have been covered or handed off to a screen family. */
    SCENE_GAME_HIDE,
};
#endif
