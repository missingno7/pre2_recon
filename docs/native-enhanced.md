# Enhanced gameplay rendering

`native/enh_render.h` is the presentation API. The renderer consumes an immutable
`FrameGameView`, its native `EnhLevel`, palette, top-left world camera, dimensions,
and options. It writes palette indices into a caller-owned buffer and copies the
six-bit palette and pixel mask into `EnhImage`. It reads no game memory or faithful
playfield pixels, consumes no randomness, and makes no simulation or publication calls.
There is no host switch in this change.

```c
EnhImage image = {0};
image.index = buffer;
image.capacity = buffer_bytes;
image.stride = width;
EnhOptions options = { ENH_BACK_MIRROR, ENH_HUD_CENTRE, positions, position_count };
const EnhLevel *level = enh_level_lookup(frame.game.level_epoch);
int result = enh_render(&frame.game, level, frame.palette, frame.pel_mask,
                        camera_x, camera_y, width, height, &options, &image);
```

Use this path when `frame.game.valid && frame.game.world_valid`, with no iris,
column reveal or staggered wipe in `frame.transition`. Palette fades can use the
enhanced image and the published palette. The clear/tally surfaces and the pages
left blackened after the iris are explicitly marked `world_valid = 0`; a phase
name of `MainFrameLoop` alone does not identify gameplay. Transitions and screen
families remain the responsibility of their respective presenter.

The camera arguments are world pixels and may be fractional; indexed tiles sample
the floored camera, and sprite placements floor the translated fractional anchor.
The camera is the viewport's left/top, so symmetric widening usually subtracts
`(width - 320) / 2` from the simulation camera X. Positive shake moves world tiles
and sprites down and blacks the exposed top rows. The sprite list already contains
the shake displacement; it is not added a second time. Dots retain their captured
fine-scroll/shake convention and channel OR operation.

Dimensions are checked in the range 1..16384, with stride and capacity overflow
checks. `ENH_HUD_NONE` makes the whole height playfield; the other HUD options reserve
the bottom 24 rows and put the native 320-pixel strip at the left, centre or right.
The strip's surrounding area repeats its interior border columns. Height 176 with
HUD disabled is the playfield parity configuration; height 200 with HUD enabled
contains the original 176-row field. Backdrop options are a central 320x200 image
with black sides, reflected horizontal extension, or nearest-neighbour horizontal
stretch. Vertical pixels outside its 200 decoded rows are black.

`EnhPosition` supplies an absolute world motion anchor keyed by `FrameSprite.id`.
Current cel, pose offset, mirror and pass remain discrete. The renderer never guesses
identity from displacement. Screen-space sprites (frame 309) ignore camera and anchor
overrides. Every sprite in the captured list is reconsidered against the requested
viewport; partially clipped cels continue into the margin. Simulation-culled actors
are absent from that list and are not synthesized or activated by this renderer.

## Asset and frame ownership

At sprite-load completion, `assets_game.c` copies the native indexed cels and clear
masks into a shared immutable cel set. The retained source windows cover the original
clipped flash reads past a cel. These are decoded source assets, never textures baked
by drawing against trial backgrounds. The renderer applies clear masks and colour OR
operations explicitly, including blink-off and flash. Its destination is a checked
Cartesian image; no game page addresses or ring storage are used for rendering.

At `SCENE_GAME_LEVEL`, capture creates an epoch-keyed asset containing map dimensions,
load-time map cells (including secret patches), decoded tiles and classes, animation
tables and flags, decoded foreground graphics, screen-fixed backdrop, HUD source,
and the shared cel set. Zero pixels of a masked tile encode its transparent mask;
class zero copies all pixels and class one samples only the backdrop.

At a ring-window event, capture compares map rows against the preceding native
snapshot and copies only changed rows. Each view references the resulting immutable
row table. Old row tables and rows remain owned by the level. This covers cell
changes, bridge dips, cave passage changes and scenery restoration without storing
a whole map per frame. A snapshot allocation occurs only when map content changes.

The frame additionally owns 240 evaluated world-cell tile paints, the remap in force,
ordered sprite instances, ordered foreground requests, ordered dots with their OR
colour and position relative to the sprite pass, and a page-local 320x24 HUD strip.
The HUD is the permitted captured-strip representation: publication copies it from
the selected native page so partial updates and front-page writes remain coherent.
No faithful playfield image is copied into the enhanced model.

Tile paints are necessary semantic state: newly loaded edge cells use raw tile cels
until the next animation pass; animation updates eleven rows whereas edge loading
can fill twelve; level-nine panels patch the visual world without changing collision
map cells. Capture privately resolves the old ring locations into world cells and
backdrop sampling origins. The renderer uses those evaluated selections in the
original cell window and samples the map/remap elsewhere. It preserves these facts
without exposing the faithful compositor's page layout to the renderer.

Captured foreground requests preserve the original gates and clipping inside the
simulation aperture. Outside that aperture the renderer regenerates foreground
world-cell requests around the listed sprites, using the original neighbour pattern,
at any viewport height or width. Position overrides also move the neighbourhood.
The foreground is applied after sprites and before weather dots, across the viewport.

The registry owns level assets and all map versions until `compositor_init()` resets
the native session. Copies of frames are cheap and borrow those immutable objects.
Call `enh_level_retain(frame.game.assets)` before retaining a frame across a session
reset, then `enh_level_release` after its last use. Epoch lookup is session-scoped;
after reset use the retained frame's `assets` pointer, since epoch numbers may repeat.
One retained level owns all its
map versions and shared cels. `enh_levels` holds an early frame across all sixteen
session resets and verifies that its referenced contents never change. Registry
storage grows with loaded epochs and changed map rows during a session; no per-display
asset allocation is performed. The API is synchronous, like the native compositor.

## Geometry facts and remaining policy work

`enh_level_policy(level)` is a data table. Its default permits backing columns
0..255; map rows come from `MapRows`. Sampling outside those bounds is black and
never wraps into the next row. For level nine, the table ends playable columns at
20 and marks graphics columns: `DrawLevel9Panel` reads its 6x7 source panels starting
at column `20 + selector*6`, then paints them into the playfield. These sources must
not become wide world scenery.

The per-frame `room` is `ScrollFollowGate` (set by `TryCaveTrigger`), and `level_flags`
are captured without reinterpretation: mask 2 suppresses horizontal follow and mask
4 forces vertical scrolling. `camera_limit_row` is `max(MapRows-11,0)` from
`ScrollViewDown`. `camera_limit_column` retains `CameraLeftLimit`: `ScrollViewRight`
conditionally switches from it to 236 based on player X, so it is **not** advertised
as an unconditional presentation-camera clamp. The renderer accepts any presentation
camera; the coming presenter chooses follow/clamp behaviour from these facts.

The separate geometry audit can refine this table. Tower/0Eh room partitions, the
level-0Dh underfloor staging cap, and cave-specific playable rectangles have not been
imported from the old port as established facts. A zero `fixed_room` policy imposes
no restriction; it is not a proof that every backing cell is playable. Those refinements
are an integration item for that audit, not a faithful-image fallback.

## Validation

`tests/native/enh_levels.c` repeats the sixteen `game_levels` schedules and death/
checkpoint case. `enh_scenarios.c` repeats the six `render_parity` schedules. They
compare every eligible display frame's 320x176 indexed playfield with the faithful
native frame, which is independently compared with reference VGA scan-out. Differences
print frame/level, pixel count and, in the all-level test, their bounding box. Both
tests fail on any ordinary gameplay difference. Excluded transition/non-world frame
counts are reported separately; stale animation selections and panels are reproduced,
not excluded.

| Coverage | Equal gameplay frames | Different frames / pixels | Excluded frames |
|---|---:|---:|---|
| Sixteen level schedules | 12,276 | 0 / 0 | 565 column reveal, 377 staggered wipe |
| Six scenario schedules | 10,566 | 0 / 0 | 59 iris, 200 column reveal, 34 staggered wipe, 717 iris-history/tally surfaces |
| Total | 22,842 | 0 / 0 | 1,952 outside enhanced gameplay |

The non-world count includes the resource-load hold after the iris: clearing the
transition controller does not restore the destructively blackened pages. No
ordinary gameplay mismatch is excused as a stale-ring or rendering quirk.

`enh_levels` also renders real snapshots repeatedly at 480/640/864 with fractional
cameras, extra vertical coverage and all three backdrop policies. It checks repeat
equality, row padding, buffer guards and retained-frame immutability. `enh_wide` adds
111 deterministic synthetic cases covering all HUD/backdrop policies, negative and
past-map cameras, sprite continuation at both original edges, foreground in the
margin, three sprite passes and identity-keyed position overrides. Separate assertions
check the override's actual output position, blink/flash channel operations, backdrop
sampling, HUD placement, palette and invalid buffer/camera rejection. The real-level
run checks 1,005 wide cases, giving 1,116 guarded cases including the synthetic matrix.

`enh_effects` constructs the maximum shipped wind (255), twenty particles and twenty
even-timer fireflies: all 551 ordered dots render pixel-equal, including preservation
of backdrop channel zero. It also captures 1,856 foreground requests and proves that
a map edit copies only the changed row while an earlier frame remains unchanged.
The frame capacities cover the 255-flake script ceiling and the foreground walk's
116 slots times four rows times four neighbours; dense weather is not truncated at
the former 256-dot limit.

The MinGW GCC 12.2 toolchain cannot link `-fsanitize=address,undefined` (`-lasan` and
`-lubsan` unavailable). Explicit bounds checks in the renderer, output guards and
untouched row-padding assertions are used instead. `present_purity` and
`production_frames` hash the extended records through `enh_view_hash`, excluding
native pointers and incorporating map contents and an immutable asset-content hash.

Final commands (2026-11-13):

- `python tools/nativetest.py`: **18/18 PASS**, including all enhanced tests,
  `render_parity`, `game_levels`, `present_purity` and `production_frames`.
- `python tools/nativetest.py enh_wide`: **PASS** after the final foreground-override
  fix; moving an anchor regenerates its requests without replaying the old locations.
- `python tools/difftest.py effects render sprites level objects`: **4,493/4,493 equal**.
- `python tools/verify.py`: **86,428-byte program image exact**, MZ fields equal.
  The existing five full-file header-byte differences are relocation ordering.
- `git diff --check`: **PASS**.

Production/reference publication remains equal for 30,825 display frames. No ASM,
translated game logic, assets, host switches, or faithful pixel operations changed.
