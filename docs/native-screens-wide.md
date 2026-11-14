# Native screens at arbitrary width

Implemented in the `screens` worktree. `FrameScreenView` is additive to `Pre2Frame`
(as `screen_view`; `screen` remains the existing diagnostic phase string).
The screen renderer accepts 320..2048 columns at 200 rows. The 640-column creators
picture keeps its native 640x480 geometry; 640x350 geometry is also retained.
No ASM, translated C, gameplay renderer, presenter, host or assets were changed.
The authorized compositor calls are installed here, after event dispatch and at
the end of publication, so the tests use the production capture path. Presenter
rendering and retention integration remain with the supervisor.

## Model and source evidence

`draw_screens.c` captures each 400..409 operation at its existing native handler.
Pictures retain decoded indexed sources before the loader reuses its buffers.
Credits retain 8x12 indexed glyphs, including spaces. Menu text retains its shifted
24x12 glyph layers, previous-position channel clears, character and font banks.
Clear/copy layers describe their source/destination rectangles, pitch, channel
mask and first-pixel row wrap. Callback edge records retain both old/new X/Y.
World column records retain each actual plane pass and its indexed column source.

The model materializes those operations into **independent native authoring
surfaces**, with reference-counted immutable 512-pixel blocks shared between
publications. The surfaces preserve the complete composed state; `layers` describes
only the bounded batch since the previous publication, newest operation first.
A batch is capped at 256 layers or 2 MiB of newly captured sources plus one
operation. It is discarded from capture at publication, or checkpointed earlier
when that cap is reached. A consumer's retained batch remains immutable. These are
native image caches of the screen operations, not a capture of the faithful frame
or compositor store. Publication normalizes the latched origin, row wrapping,
active columns, attribute colour mapping, blank/border and full-row clear coverage
into `FrameScreenRow`s. The renderer only reads those immutable native images and
semantic projection; it has no game-memory, VGA, compositor or faithful-image input.
The recipe's coordinates refer to native indexed authoring images, in pixels.
Font-bank images retain font bytes; picture/glyph/column images retain indices.
Cel layers retain a native-owned source window plus placement, pass, mirror, phase
and clipping parameters, including the original clear/colour block reads.

Screen/game handoffs also need rectangle copies, HUD glyphs and native cel passes
from 100.. events. During ordinary gameplay, `draw_screens_observe` skips world
tile, ring, foreground, sprite and dot painting. It keeps only the two native HUD
bands current, including rectangle intersections, and publication caches two
semantic `FrameGameView`s with their HUD arrays cleared. A handoff regenerates the
320x176 world page from native `EnhLevel`/`EnhMap`/cel data through `enh_render`,
then captures the actual destructive operations, copies and screen cels. No
faithful index image or compositor pixels seed that page. Level references are
retained only for these two semantic pages and released on replacement/reset.

The first 16x10 clear starts a block-wipe handoff: its transition event follows
forty clears, so waiting for that event would wrongly regenerate the first twenty
cleared rows. Iris full-page copies, playfield text, full-page clears and screen
operations similarly establish the handoff before their operation is applied.
Screen uploads and stale HUD rows then retain their original page semantics.
Ordinary world publications have `valid=0` and no screen image/recipe allocation.

Evidence checked against the C translation and its ASM, rather than trusting the
Python port:

* `c/screens.c:RunCallbackMapScreen`, `AdvanceMapFrame`, `DrawFontString` and
  `src/MODULE4.ASM:RunCallbackMapScreen`, `DrawFontString`: only channels 0/1
  receive the background, and only channels 2/3 receive shifted menu text.
  Each display row is a contiguous strip of the repaired 65536-pixel linear ring,
  with successive rows 320 pixels apart. It is not a modulo-320 repeated picture.
* `c/screens.c:ShowWorldMapScrollIn`, `loc_9613` and
  `src/MODULE4.ASM:loc_9613..loc_969C`: the stamped master is 640x200, four
  16000-byte planes. The loop uploads column `(scroll_x >> 3) - 1`, with separate
  completed-plane events. `FrameScreenView.revealed[p]` is an exclusive frontier
  for each channel. The latched row origin supplies the published pan, including
  during a palette fade; the transition record alone would lose that position.
* The world projection has master X = published pan - 320 + screen X and master
  Y = screen Y + 1. The bottom row does not authorize wrapping into master row 0.
  Extra columns sample the finite master, with black outside its row extent.
* `c/screens.c:ShowMode13PictureFadeIn`, `ShowPresentTitle`, `ShowPlanarPicture`,
  `RunLevelIntroScene`, `RunGameOverScreen`, `CopyBackdropStrip`;
  `c/credits.c:CopyPlane`, `ShowIntroBody`, `ShowCreditsScreen`,
  `ShowLateYearCredits`, and their MODULE2/MODULE3/MODULE4 ASM sites establish the
  finite pictures, overlay/copy geometry, fonts and 640x480 plane pairs.
  The game-over clear/upload displacement and untouched lower 24 rows are kept.

The survey section 5 and Python `CarteEnh`/`MenuEnh` were read as hypotheses.
Their background/text split is confirmed. Their completed-CARTE override is **not
used**: the final upload is at scroll 632, giving an exclusive frontier of 632;
master columns 632..639 never become revealed. The final published pan is 639,
whose 312-column aperture reaches master column 630. The renderer retains that
right edge and eight-column border, widening left into already revealed content.
It never snaps to the full 640-column master. Per-plane frontiers also protect
publications occurring between the four upload passes.

## Side rules and API

```c
EnhImage image = {0};
image.index = pixels; image.capacity = capacity; image.stride = width;
const FrameScreenView *v = &frame->screen_view;
int result = enh_screens_render(v, frame->palette, frame->pel_mask,
                               v->pan_x, v->pan_y, width, v->height, &image);
```

* Menu/password/MODE: contiguous background-ring strips extend symmetrically.
  The original glyph layer stays centered and cannot leak text ghosts into the
  margins. The outer eight-column display border remains at the right edge.
* CARTE: the original aperture stays right-aligned, the extra width extends left,
  and each master channel is bounded by its actual upload frontier.
* Finite pictures, intro sprites, tally and credits retain authored positions in
  the centered original canvas. Picture sides are literal black from a black
  entry in the supplied palette. A full-row clear covers the sides with logical
  colour zero, which can differ from literal black. Blank output covers the whole
  width with the captured border. All shipped screen palettes have a black entry.
* Creators mode is rendered at 640x480, without treating its plane pairs as a
  320-column picture or stretching its authoring geometry.

The function returns -1 for invalid views, geometry, non-finite/unsafe coordinates
or insufficient stride/capacity. It writes indices plus the supplied six-bit
palette and PEL mask. It neither interpolates DAC indices nor mutates the model.

## Supervisor integration

`native/compositor.c` includes `enh_screens.h` and now contains these two calls.
Do not call either hook again in a test wrapper or presenter:

```c
/* sink(), AFTER dispatch: native game assets must already have been captured. */
draw_screens_observe(e);

/* compositor_publish(), AFTER compositor_game_view has populated f->game: */
draw_screens_publish(f, start);
```

Observe every core/game event exactly once. Calling the observer on screen events
is harmless: their handlers already capture them. Keep `draw_screens_register` in
its existing initialization order. The frame publication call initializes every
screen-view field, including `valid`; do not use an unpopulated member.

Use `enh_screens_render` for every `screen_view.valid` publication: title/front
pictures, menu/password/MODE, CARTE, intro, tally, game over, THE END, credits,
blank/loading and creators mode. Ordinary gameplay has `valid=0`, intentionally,
and remains on the gameplay renderer. At high resolution pass the view's own
width/height. Palette fades use the same screen renderer and the publication's
palette; no faithful image is needed to fade it.

For smooth pans, use two consecutive observed publications with matching kind,
epoch, format, active geometry and master/font identity; never extrapolate.
At alpha 0/1 render the complete corresponding endpoint. For an open interval:

* CARTE: interpolate their **published** `pan_x`; Y remains zero. Use the newer
  native surface, but cap each `revealed[p]` to the minimum of the two endpoints
  until the newer endpoint is reached. This avoids promoting an incomplete plane
  pass. The renderer additionally masks source channels beyond the frontier.
* Menu: `pan_x` is the actual linear origin modulo 65536, and `pan_y` is zero at
  publications. First unwrap the difference to [-32768,32768), then split it as
  `dx + 320*dy`, taking dx in [-160,160). Relative to the newer view, supply
  `pan_x = newer.pan_x - (1-alpha)*dx` and
  `pan_y = -(1-alpha)*dy`. This keeps the row-wave displacement separate from
  horizontal motion. For discrete vertical bounce, simply keep pan_y zero.

Indexed samples floor both coordinates. To filter fractional positions, render
at floor/ceil X and Y and blend the resulting palette-expanded RGB images with
fractional weights. Text stays at the newer publication's authored position;
blending scroll samples must not translate that glyph layer. Do not linearly
interpolate the entire menu's wrapped linear origin through a row-wave jump.
For smooth palette fades, interpolate expanded RGB palette entries only between
compatible published endpoints, following the presenter's bounded fade policy;
there is no need to import the Python port's wall-clock exponential chase.

## Retention contract and memory accounting

`compositor_publish` returns a **borrowed** screen view. Capture keeps that
publication alive until the next publication or `compositor_init()`, including
while intervening events replace fonts, images or source assets. Rendering it
immediately needs no retain. Before keeping a copy beyond that lifetime, call:

```c
Pre2Frame copy = *published;
enh_screen_view_retain(&copy.screen_view);
/* Queue/presenter/transition history may now outlive publications and resets. */
/* ... render the retained copy zero or more times ... */
enh_screen_view_release(&copy.screen_view);
```

Each independent owner of a copy takes one reference, even if an image is shared
with another frame. Release once when dropping that copy. A shallow move can
transfer its existing reference without another retain. Release does not clear
the caller's struct; do not render/release it again unless it is assigned another
owned view. Zero-initialized views are safe no-ops. Use the functions on the whole
view, including invalid views, rather than freeing images or individual blocks.

The presenter must retain each of its 32 history entries and every additional
transition endpoint or saved frame; release on eviction, replacement, clear and
destruction. A cross-thread queue must retain on the producer before enqueueing,
then release its queue reference after the consumer has retained/transferred its
own copy. Retaining an already expired unowned pointer is not allowed. Native
world assets still use their existing, separate `enh_level_retain/release` contract.

Images own their blocks; operation nodes own their image and previous node.
Capture releases an image immediately when its target snapshot/source/font/cel is
replaced, and drops each operation batch after publishing it. Retained blocks are
never rewritten. Reference counts are atomic; retain/release and rendering owned
views can occur on consumer threads. Capture/publication/reset stay on their
single producer. The producer owns one reference to the latest borrowed
publication, keeping its roots safe until that borrowing interval ends.

`compositor_init()` releases **all capture-owned references**, including the last
publication, three target snapshots, fonts, cels, operation batch and two semantic
world-page level references. Consumer-owned screen views survive reset without
any epoch lookup or pointer into the new session. After those consumers release
their views and capture resets, all screen heap counters return to zero.

Memory is proportional to the union of images/blocks/batches held by live consumers,
plus fixed working surfaces, three current target snapshots, two semantic world
pages and bounded current source/cel caches. No object retains the session's
publication or operation history. The cel cache has at most 1024 fixed 64-KiB
windows, is dropped on asset/mode replacement and entry into ordinary gameplay,
and is usually much smaller. A source allocation can exceed the batch byte cap
by at most that one operation's fixed maximum image size (512 KiB).

`enh_screen_memory_stats()` reports live and peak heap bytes/blocks, live images
and layers, plus allocation count. Byte totals include reference headers but not
fixed native working buffers or the gameplay renderer's separately owned assets.
`enh_screen_memory_reset_peaks()` starts a new measurement without releasing any
object. Allocation statistics are atomic; peak reset and the observer counters
are capture-thread diagnostics and should be sampled/reset while it is stopped.

## Observer measurements

`enh_screens_lifetime --bench-only --assets assets` records the real gameplay
scene events after simulation frame 10, stopping at frame 75. It measures their
arrival rate during the run, then replays that captured presentation-only event
sequence 256 times with one high-resolution timer around the whole replay. The
replay asserts that the game-memory digest is unchanged. Resource/transition
boundaries are excluded from the microbenchmark; their real handoff work still
runs in the scenario. No timer is added to the production observer.

Before removing the duplicate world painter: 2,773 events, 78,825 events/s,
1,454.6 ns/observer event, estimated 11.466% of measured wall time.
With the fast path on the same sequence in the final full-suite run: 104,081
events/s, 4.1 ns/event, estimated 0.043% wall share. This is approximately 355 times
less observer work. An earlier optimized run measured 4.3 ns/event / 0.042%,
showing the expected small machine-load variation.
The share is an estimate (measured event rate times measured replay cost), not a
sampling-profiler percentage; test callbacks/retention checks are in the measured
run. HUD and handoff events retain their necessary work. The handoff world render
occurs once per boundary, never for ordinary sprite/tile events.
Raw baseline output is `build/screens_shots/observe_before.txt`; the extended
retention test also prints current event rate, replay cost and memory high-water
marks, so machine-dependent timings can be reproduced.

## Verification and review

`python tools/nativetest.py`: **30/30 pass** (the existing 27 plus three screen
programs), with the production hooks active. The parity test retains its
26,584-frame comparison; the constructed test additionally caps a 10,000-operation
burst without publication. The final suite passes 26 constructed checks and
118 immutable-history checks. Constructed cases cover wrapping ring origins,
text-channel exclusion at 320/321/356/468/640/2048, source/compositor destruction
after capture, logical clears versus literal-black sides, per-plane reveal,
finite map rows, credits glyphs, 640x480 and argument rejection.

`enh_screens_frames` checks every non-world CRT publication of eight runs, not
only stable holds. Counts below include repeated displays, loading and partial
plane passes. All entries have **zero mismatches**, and every run has **zero
invalid non-gameplay views**. There are 26,584 checked screen frames in total.
The separate world-frame exclusions are legitimate gameplay, not screen fallbacks.

| Run | Blank/loading | Picture | Menu | CARTE | Credits | Intro | Tally | Game over | World excluded |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 1995 attract | 178 | 2343 | 0 | 419 | 0 | 424 | 0 | 0 | 1892 |
| 1997 attract | 131 | 2344 | 0 | 786 | 131 | 424 | 0 | 0 | 1440 |
| 1995 password | 85 | 1048 | 240 | 0 | 0 | 0 | 0 | 0 | 0 |
| 1995 MODE/options | 85 | 1048 | 240 | 0 | 0 | 0 | 0 | 0 | 0 |
| 1995 ending | 425 | 1411 | 322 | 1608 | 0 | 0 | 0 | 0 | 1471 |
| 1995 castle | 392 | 1613 | 322 | 1608 | 0 | 0 | 717 | 0 | 604 |
| 1995 game over, full hold | 398 | 1211 | 322 | 1608 | 0 | 0 | 0 | 935 | 782 |
| 1995 ending, VGA memory off | 425 | 1411 | 322 | 1608 | 0 | 0 | 0 | 0 | 1471 |

Named picture counts: TITUS 1,240; PRESENT 4,176; front-end picture 4,547;
THE END 170; creators mode 230; CASTLE 266. Another 1,800 picture publications
retain a previously authored picture during resource loading or palette fades.
Late-year credits persist through 131 publications. Classic credits additionally
pass the constructed test; no scripted key-gate run is claimed for that variant.
The password run asserts both wrong-code hold and reset, and MODE asserts that
an expert selection was reached. The game-over script waits through the full
screen instead of tapping past its scrolling artwork.

Mismatch classes: pictures/partial uploads 0; background ring and glyph channels
0; map reveal/scroll/fades 0; native cels and preserved lower rows 0;
clear/copy/tally 0; credits 0; creators plane pairs 0; invalid screens 0.
Raw per-phase counts are in `build/screens_shots/parity.txt` (generated by running
`build/native/enh_screens_frames.exe --assets assets` and capturing stdout).

`enh_screens_lifetime` retains a sliding window of 32 full frame copies through
one continuous hour of attract loops, ending/game-over runs of 240 logical seconds
each, another 600 seconds of attract loops, and a gameplay benchmark. Three pinned
menu/map/high-resolution views additionally survive all resets. Every retained
screen is compared with its original published indices before eviction; 640-wide
menu/map renders are hashed at publication and verified after the delay. A fixed
32-MiB/50,000-block bound is asserted, including peaks; warm-up is not allowed to
hide a larger peak. Final reset plus release must leave zero bytes, blocks, images
and layers. The final run made 99,519 retained screen checks and 15,300 delayed
wide-image checks, all exact. Memory high-water marks (including three pinned
cross-reset views) were:

| Scenario | Publications | Peak heap bytes | Peak blocks |
|---|---:|---:|---:|
| Attract, 3,600 seconds continuously | 252,310 | 6,856,520 | 12,096 |
| Ending, 240 seconds | 16,801 | 4,311,176 | 8,002 |
| Game over, 240 seconds | 16,821 | 5,607,032 | 10,093 |
| Attract, another 600 seconds | 42,051 | 8,233,576 | 14,657 |
| Gameplay benchmark | 2,443 | 4,104,360 | 7,008 |

The maximum was 7.85 MiB, below the fixed 32-MiB/50,000-block test bound. All
four live counters were zero after the final reset and consumer releases. The
second attract run includes all three earlier pinned views; the first hour
therefore has a lower peak despite lasting six times as long.

`python tools/verify.py`: 86,428-byte program image and all MZ fields exact;
relocation set exact, with the pre-existing five full-file relocation-order bytes.
No C changed, so no translated-C differential-test result is implied by this work.
`python tools/hostbuild.py`: pass. The required faithful headless 45-second smoke
and widescreen smoke with `--wide-sim 464` reach the bounded exit **3**, 3,275 ticks,
3,153 publications and 349,944 PCM bytes. Faithful digest is `b32fdf580ab9c3b9`;
wide-simulation digest is `efedd6f11bf45941`. Width 468 is a presentation width;
wide simulation requires a multiple of 16. These host checks exercise the
production capture hooks. This worktree's presenter still needs the supervisor's
screen-rendering and queue/history retain/release integration described above.

`build/screens_shots/contact.bmp` and `manifest.txt` cover sixteen named views at
356, 468 and 640, including loading and the front title. Individual BMPs preserve
full resolution; creators has a native 640x480 image, with review thumbnails in
all three contact-sheet columns. No faithful image is an input to those exports.
