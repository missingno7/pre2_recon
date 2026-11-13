# Gameplay renderer — rgame

Implemented semantic events 100–120 in `c/scene_game.h` and indexed-pixel handlers in
`native/draw_game.c`. No native gameplay handler reads VGA registers, latches or planes.
Original raster operations, simulation mutations and video traffic remain in place.

Coverage: tile uploads and level stash; backdrop upload/restore; all tile classes; row,
column, animation, overlay and cell-change callers; ring rotation, fine scroll and shake;
foreground near sprites; level-nine panel; all sprite passes; consumed effect dots,
fireflies and flakes; HUD background, BONUS restoration, both fonts and sprite letters,
both-page copies; entry/cave wipes; accumulating iris; death bounce and tally drawing.

Native-owned assets are captured at the completed sprite load and tile-upload events.
Sprite cels have decoded colour/coverage pixels, plus owned source windows for unchecked
reads past a cel. HUD/foreground source bytes are owned copies. Backdrop and stash uploads
become indexed pixels in the native store. Successful level loads advance the level epoch.

Per-page semantic views carry draw-time camera, shake, main-loop sequence, ordered sprite
passes, anchors/placement and consumed points. Identities observe empty slots, child/burst/
debris allocation events, and child/projected-list owner changes. Room cuts retain identity;
level loads and checkpoint entry reset it. Covered/screen handoffs invalidate gameplay views.

## Preserved quirks

- Sprite coverage clears during blink-off; flash skips the unclipped clear but clipped
  flash clears and performs four white passes. Equality at the right byte edge selects
  clipping. Mirrored source traversal, shifted carry/fringe masks, signed byte strides,
  zero-count loops and 16-bit source/destination wraps are preserved. Frame 309 bypasses
  the camera and normal culls, including negative visible byte counts and rows.
- Backdrop destinations wrap inside the ring, while sources do not. Opaque/mixed tile
  blits retain their original destination arithmetic. The ring is persistent, not rebuilt
  as a conventional viewport.
- Iris head/tail arithmetic can clear beyond the requested width; spans crossing the
  right edge wrap into the next scanline. The two pages accumulate their destructive edits.
- The column reveal repeats the first middle strip and leaves the far-left strip alone.
  The staggered wipe has nine stages, including its overlapping central bands.
- Firefly even timers OR channels 1–3 and preserve the existing channel-zero bit. Flakes
  retain the original two-row mask and camera-column subtraction. Particle dots are
  captured after motion and before their records become FFFFh.
- HUD sprite letters do not dirty the HUD cache; the original score/life/pip quirks stay
  in the translation. HUD background capture occurs after its draw and before its copy:
  emitting a complete two-page update earlier produced a real extra display frame.

## Verification (2026-11-13)

`python tools/verify.py`: PASS, 86,428-byte program image exact; MZ fields equal. Existing
full-file relocation-order difference remains (five header bytes).

`python tools/difftest.py effects render sprites level objects`: 4,493/4,493 cases equal.
`python tools/nativetest.py stepping nullvga sqz`: 3/3 PASS.
`python tools/nativetest.py game_sprites`: 4,176 bounded cases PASS, all 460 shipped cels,
both mirror states and all three pass modes; full edge matrix for cels 0 and 309.
`python tools/nativetest.py game_operations`: 875 completed-operation store comparisons
PASS (ring, tiles, foreground, panel, HUD, iris, consumed particles and weather).

`game_levels` reaches all sixteen level assets, enters bonus levels through a constructed
warp-table destination, and forces death/checkpoint respawn in level zero. Each run covers
twelve seconds after the first gameplay frame. **Pixels: 16/16 levels equal.** Whole-store
differences during atomic semantic operations versus intermediate reference plane passes
are reported, not hidden. Completed-operation store comparisons above are exact.

Canonical `render_parity` results with the screen renderer still a stub in this worktree:

| Scenario | SetLevelVideoMode differing frames | MainFrameLoop differing frames |
|---|---:|---:|
| none | 0 / 105 | 0 / 926 |
| warp | 0 / 105 | 0 / 1977 |
| attract | 0 / 106 | 0 / 4326 |
| ending | 0 / 238 | 200 / 1669 (first display frame 2387: ending picture) |
| gameover | 0 / 238 | 18 / 1699 (first display frame 2591: game-over screen) |
| castle | 0 / 238 | 259 / 1561 (first display frame 3171: castle picture) |

Those remaining MainFrameLoop differences are screen-family operations nested in the
level transition. Startup/front-end/map phases also differ until rscreens is integrated.
No screen-owned function was edited. Full canonical `render_parity` consequently fails.

## Required core integration

The existing gameplay-view and transition hooks do not receive the latched
start address. `DisplayPage` is the requested page and can already refer to the following
image. `game_levels` deliberately fails its semantic check: for example, level zero at
simulation frame 5 publishes view 5 while the pixels show view 4; 18 mismatches in that run.
All sixteen levels expose this; their pixels remain equal.

Requested permission for the following change outside rgame's assigned files; not applied:

1. In `native/compositor.h` and `.c`, give `compositor_game_view` and
   `compositor_transition` a second argument, `uint16_t start`.
2. In `compositor_publish`, pass its existing `start` to both hooks.
3. In rgame's `publish_view` / `publish_transition`, accept `start` and select the
   corresponding page. Both the views and transition states already have per-page storage.

This is necessary to finish the published semantic view. The all-level test compares the
published sequence/camera/shake against independently recorded page stamps using the actual
latched start; it does not relax the check to the requested page. It reports pixel and
semantic results separately. The core should also combine transition-family hooks when
rscreens installs its fade/map transition publisher; gameplay supplies iris/column/block
progress through `compositor_transition` today.

Not claimed: all-frame parity of unintegrated screen code, exhaustive boss/cave gameplay
trajectories, or whole-store parity during every intermediate plane pass.
