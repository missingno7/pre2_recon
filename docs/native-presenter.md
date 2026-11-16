# Native presenter

`native/presenter.{h,c}` and `native/present_motion.c` consume copies of published
`Pre2Frame`s. They contain no SDL, machine, game-memory, input, audio, random-number
or compositor calls. Rendering the same stream at another refresh rate cannot
change the simulation. This is presentation-only widening: actors outside the
original simulation's draw list are not activated or synthesized.

The host defaults to **faithful with every smoothing option off**.

| Host option | Presentation |
|---|---|
| `--mode faithful` | Published indexed image and palette as is. |
| `--mode enhanced` | `enh_render` at 320x200 on valid ordinary world views; faithful for other surfaces. With smoothing off, gameplay including HUD equals faithful. |
| `--mode widescreen` | Enhanced world at the requested width, 200 rows. Other screens and non-world surfaces retain their original image, centered with literal black margins. |
| `--aspect 16:9`, `16:10`, `21:9`, `window` | Widescreen aspect; default 16:9. Width is height times aspect, rounded up to an even pixel, at least the original width. Window mode uses the current drawable aspect. Width is bounded at 2048. |
| `--interpolate` | Interpolate world anchors by ID and generation, plus simulation camera. Faithful mode uses the enhanced renderer for these gameplay images. |
| `--smooth-camera` | Implies interpolation. Presentation camera follows the player identity with damping. |
| `--camera-tau SECONDS` | Positive time constant, at most 10 seconds; default .12. Larger is softer. Applies directly to both axes; no overriding recenter constant. |
| `--smooth-transitions` | Sample original transition steps continuously using logical time and published endpoint images/palettes. |
| `--hud left`, `centre`, `right` | Override the native HUD grouping. Default `ENH_HUD_ANCHORED` places each semantic element at its own corner in a wide view; only the art's plain frame pattern extends. The host CLI retains its existing three override spellings. |
| `--backdrop black`, `mirror`, `stretch` | Extension policy for bounded backdrop pictures (default black). Audited seamless mountain/cloud and cave-rock pictures wrap by their source period; see native-enhanced.md. |

The original host's uncorrected 320:200 pixel aspect is retained. Consequently
16:10 is 320 pixels wide at height 200, 16:9 is 356, and 21:9 is 468; these switches
do not silently change to 4:3 pixel correction. High-resolution non-world screens
keep their native height and width. `--reference` selects the diagnostic reference
path and disables presenter options. Headless/turbo runs need no display or audio
device and still exercise the presenter.

## Ownership and clocks

Create one `Presenter` per consumer, push publications in increasing `t_ns`, then
call `presenter_render(p, bound, window_aspect, &image)` at display refresh. Image
capacity is in **pixels**. The output includes dimensions, sampled logical time,
camera, interpolation alpha, transition progress/radius and fallback bits. The
caller owns the XRGB buffer. Output black margins are literal RGB black, independent
of DAC entry zero. Bad dimensions, insufficient buffers and times before the oldest
retained publication return -1, leaving the caller free to keep its previous image.

The presenter copies 32 CRT publications, retaining the two consecutive distinct
simulation views needed around the requested time (and further bounded history for
transitions). Repeated displays of the same `game.sim_frame` retain the time it was
**first displayed**. They do not restart interpolation. The delay is the fixed
`PRESENTER_DELAY_NS = floor(3 * 16384 * 1e9 / 1193182)`, about 41.194 ms: one original
simulation frame of three PIT ticks, **not** a fixed host update frequency or three
CRT frames. No smoothing means no delay. Interpolation or smooth transitions applies
this delay to the entire presentation so screens and world handoffs stay ordered.
Insufficient history holds a published endpoint; it never extrapolates.

Assets/maps are borrowed immutable session objects. Destroy/reset the presenter
before `compositor_init`, or explicitly retain its frames' assets across a session
reset. `presenter_push`, render and reset are serialized by the caller; the native
module does not make the asset registry thread-safe. It uses the retained asset
pointer, never an epoch lookup that could resolve into another session.

The SDL host owns a simulation worker. That thread initializes the machine fiber,
steps to the wall-clock logical bound, runs scripted input and submits native PCM.
The SDL/main thread pumps input, drains a bounded publication queue and renders at
vsync, including refreshes with no new CRT frame. Only bounded frame/input copies
hold the bridge mutex; rendering and vsync never do. Overflow drops old **pictures**.
It cannot block waiting for presentation or change the logical target. Input is
queued to the worker; no main-thread code accesses a running machine. The worker
is joined before reports, demo saving, presenter destruction and SDL shutdown.
Headless runs use the same simulation function synchronously at 60 Hz logical
slices. Audio generation, timestamps, rate and SDL resampling are unchanged.

## Motion and camera

For an open interpolation interval the newer view supplies the sprite set,
animation/cel, pose offsets, mirror, pass, tiles, HUD, palette and dots. Matching
requires both ID and generation. New sprites appear at their newer position;
despawned sprites disappear. Screen-space sprites ignore anchor overrides and
camera movement. At alpha 0/1 the complete respective endpoint view is used, with
no position overrides (including its discrete sprite set and foreground requests).
This endpoint convention necessarily has a discrete set change just after alpha 0.

The sole motion-distance guard is the named **256-world-pixel Euclidean teleport
threshold**, applied only to an already matched identity within one view interval.
It does not replace identity and does not suppress legitimate 32/64-pixel motion.
Camera cuts and epoch/asset changes suppress interpolation across the boundary.
Discrete dots retain their original colour/channel-OR behavior; no extra snow or
fireflies are generated. Position overrides go through `EnhOptions.positions`, so
the renderer regenerates foreground across the actual viewport.

The player is render **slot 1**, DS:4F1Ch in the original (`draw_game.c` computes
the slot from `RenderSlots`, and `RunIrisClose` retains that record). Camera follow
looks up its published ID each time, then uses the interpolated anchor. The target
is that anchor at the playfield center; no guessed per-animation body offset is
added. A first-order exponential damp uses logical elapsed seconds and the selected
tau. The camera is deterministic for a given time sequence; different sampling
sequences may yield slightly different damping integration. Repeating a timestamp
does not advance it. Cut, epoch change, player replacement and backwards seeks
reseed it. Shake is not integrated into camera state: `enh_render` applies captured
shake after the camera; sprite placements already carry it.

Bounds use `enh_level_policy`, the map row extent and the view's row camera limit.
The column limit is conditional: within its ordinary aperture it bounds the region;
past it the original simulation camera is retained as evidence of the current
region, rather than declaring that limit a universal world edge. The audit's
restricted stages (05h, 09h, 0Eh), nonzero cave follow gate, horizontal-follow mask
02h, policy-fixed rooms and regions narrower than the requested width keep the
**original moving 320-pixel aperture**, centered with black sides. This includes
05h's lower encounter without pretending its initial camera is an arena boundary.
Forced-scroll mask 04h follows interpolated simulation Y. A parked camera alone is
not classified as a fixed room. No global level-0Dh 149-pixel cap is imported.

Region annotations are incomplete for some zero-gate caves and staged 0Dh art;
the geometry audit's unresolved polygons remain an upstream content-policy limit.
Nothing in the presenter widens collision, activation, pickup budgets or camera
state in the game.

## Transitions and explicit fallbacks

Presentation time is delayed, so both interpolation endpoints have already been
displayed by the simulation. It never shows an unpublished destination. It does not
drain a sequence and then run an independent wall-duration animation. Consecutive
steps must share kind, phase, geometry, epoch and display format. Duplicate step
publications do not reset time. Published intermediate DAC writes are their own
palette endpoints. At a distinct original endpoint's presentation timestamp the
faithful transition image is reproduced; progress/radius is bounded by observed
endpoints, never extrapolated. No blending crosses a phase/level/format boundary.

| Kind | Projection / fallback |
|---|---|
| Iris (1) | Native retained world plus the accumulated per-page blackout intervals, including original head/run/tail arithmetic, row crossing and sprite redraw. At 320 every original step equals the faithful reference. An interval reaching an original edge extends to the view edge. Between compatible steps smoothing uses an interpolated circle over the native source, then redraws retained sprites. Missing world data reports bit 2. In a wide layout the HUD rows are redrawn from the strip's last state before the iris and cut by the published circle: the original blacks its spans out of the 320 strip, which moved fields cannot map back. |
| Column reveal (2) | Native departure and already-drawn destination worlds; actual copied columns select between them. Original column = floor(x * 20 / width), preserving the repeated middle strip and untouched far-left strip. With smoothing it is one curtain opening from the centre over the whole width: the half-width is the original's right edge (16 of 320 pixels per step, continuous between steps) and the left side mirrors it instead of trailing by one strip, so the far-left strip the original never copies opens with the rest. The destination is the complete page the original copies from. |
| Staggered block wipe (3) | Native displayed source plus actual 16x10 clears. Whole-row bands extend across the view, including overlapping central bands. Death, cave and bonus-warp closes share this path. With smoothing the two bands (ten rows per step from the top and bottom edges) close towards the middle at a steady rate between steps. |
| Palette fade (4) | Per-entry interpolation in expanded RGB between consecutive published palettes with identical indexed image and pixel mask. Index changes or mismatched phases keep the published palette/image until a compatible pair exists. |
| Map scroll (5) | Interpolate published X/Y with bilinear RGB sampling only if the complete overlap is an exact translation and palettes agree. New edge pixels come from the already-published endpoint. Stationary text/markers, ring repairs or capture/CRTC phase mismatch fail validation and retain the faithful frame (fallback bit 4). A semantic world-map/background/overlay split is needed to cover those cases. |

The circle remains an intentional between-step presentation choice; original step
endpoints use the exact native span model, and both paths preserve the post-mask
sprite pass. The semantic source model and per-level backdrop audit are documented
in `native-enhanced.md`. Transition destinations are frozen from completed native
draw events, never reconstructed from future game state.

Every non-faithful output that retains **any faithful pixels** sets fallback bit 8.
Missing gameplay-world data also sets bit 1; missing iris data sets bit 2; rejected
map-scroll projection sets bit 4. Native gameplay and kinds 1/2/3 clear bit 8 only
after a successful complete native render. The unchanged restricted-aperture policy
can produce literal black margins, but its interior is native. Blank/loading,
clear/tally and other screen families remain explicit faithful fallbacks in this
worker's scope; their separate screen-native integration can clear bit 8 when it
replaces their pixels. No gameplay transition needs those fallbacks.

## Checks and review images

`python tools/nativetest.py presenter_motion presenter_transitions presenter_purity presenter_shots`
builds the SDL-free consumers with the normal strict C warnings.

* Motion tests cover IDs, generations, spawn/despawn, alpha endpoints, screen-space,
  cuts/epochs, the teleport guard, camera determinism, effective strength and bounds.
* Transition tests cover faithful endpoints, monotonic bounded progress/radius,
  per-entry fades, masks and validated map translation with overlay fallback.
* Purity repeats the existing four `present_purity` schedules at 60/144/240 Hz,
  rendering all three modes with all smoothing options enabled. It compares memory
  at every main-loop/phase change, every machine counter, all PCM and published
  pixels/palettes/views/transitions. It also compares unsmoothed faithful/enhanced
  images at each publication. The first run: 386,040 renders, 20,323 exact image pairs.
* Shots repeat level 1/castle, gorilla and level-F schedules, verify whole 320x200
  gameplay/HUD parity, and write `build/presenter_shots/contact.bmp` and individual
  BMPs. Read `manifest.txt` in row-major order. The cave is explicitly constructed
  from level 1 assets at the audit's camera 00DCh / arrival 07E5h / gate 1; it is not
  an assertion that an interactive route was played. Map/fade images may demonstrate
  a documented fallback. The iris image is sampled inside an observed interval.

The required host smoke command returns **3** on its bounded time limit (success
for this harness), not zero. Full-suite, oracle and host results are recorded in
the task report; the presenter introduces no changes to ASM, translated game or
faithful capture/raster operations.

Validation, 2026-11-13:

* `python tools/nativetest.py`: **22/22 pass** (the existing 18 plus four presenter
  tests). All four presenter tests were rerun after the final bound guard: **4/4
  pass**, including 386,040 pure renders at 60/144/240 Hz and all seven shots.
* `python tools/verify.py`: **86,428-byte program image exact**, MZ fields equal;
  the existing five full-file header-byte differences are relocation ordering.
* `python tools/hostbuild.py`: **pass**. `build/host/pre2_sdl3.exe --assets assets
  --smoke 45 --headless --year 1995 --mode MODE` passes the bounded run for each
  mode (exit 3); widescreen also exercises 21:9 and all smoothing options. Every
  mode has 3,275 ticks, 3,153 display publications, 349,944 PCM bytes and memory
  digest `b32fdf580ab9c3b9`.
* A noninteractive SDL dummy-driver turbo smoke exercises the worker/main bridge
  with widescreen and all options: **55 frames shown / 3,153 published**, the same
  machine counts and digest. No visible window or interactive play was used.
* `git diff --check`: **pass**. Contact sheet visually inspected.

Gamehud validation adds `enh_hud`, `enh_hud_levels` and `enh_transitions`. The latter
two reuse the sixteen-level/death and six render schedules. Their presenter inputs
poison the faithful image and HUD strip: native 320 endpoints still equal the original,
and 320/356/640 with/without transition smoothing report zero fallback. Distinct
semantic surface updates also distinguish transition clocks even if faithful index
bytes happen to agree. Review images are in `build/gamehud_shots/`; the wipe image
is a death close using the same routine as cave entry, not an asserted walked route.

Gamehud checks, 2026-11-14:

* `python tools/verify.py`: 86,428 program bytes exact; MZ fields equal. The
  pre-existing five full-file header differences remain relocation ordering.
* `enh_hud_levels enh_transitions`: 24,753 exact HUD publications, 1,911 exact
  native destructive pages, and 148,518 zero-fallback presenter endpoint samples
  at 320/356/640 with/without smoothing. Compatible interval interiors are checked
  too. Both faithful indices and HUD reference bytes are poisoned before presenter
  use. All 59 iris publications also check the previously rejected margin boundary.
* `enh_hud`: 48 exact partial-update, copy, clear and page/placement checks.
* `presenter_purity`: 386,040 renders at 60/144/240 Hz, all modes/options;
  memory, every machine counter, PCM and published frames remain identical.
* `wide_sim`: all sixteen levels at 480/640, two runs each, pass. Entry black
  surfaces retain aperture dimensions even before the first world pass.
* `python tools/hostbuild.py`: pass. Four 45-second headless smokes (faithful,
  enhanced, widescreen 356, and 640-wide simulation with all smoothing) terminate
  with expected native status 3. The first three have digest `b32fdf580ab9c3b9`;
  true wide simulation has digest `469af867632dfdf5`. Logs are in the shots folder.

Legacy fixture migration: `tests/native/enh_wide.c` constructs only reference HUD
pixels. It must supply source assets and semantic elements under the new contract.
It is outside this worker's assigned test patterns and remains unchanged. The
prepared `build/gamehud_shots/enh_wide_fixture.patch` passes 138 guarded renders
(including anchored placement and periodic backdrop sampling) as a standalone copy.
All other native tests passed across the full-suite and targeted reruns; this one
fixture prevents claiming a complete passing suite until its owner integrates it.

The curtain review image is a native indexed endpoint replayed with the visible
palette subsequently observed for the same level. The scripted entry curtain runs
under a black DAC; this explicit diagnostic colouring shows its column geometry.
Production presentation always keeps the palette actually published at that time.
