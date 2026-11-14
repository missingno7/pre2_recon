# Native screens at arbitrary width

Implemented in the `screens` worktree. `FrameScreenView` is additive to `Pre2Frame`
(as `screen_view`; `screen` remains the existing diagnostic phase string).
The screen renderer accepts 320..2048 columns at 200 rows. The 640-column creators
picture keeps its native 640x480 geometry; 640x350 geometry is also retained.
No ASM, translated C, gameplay renderer, compositor core, presenter, host or assets
were changed. The shared compositor/presenter wiring below is the supervisor's
integration step; this worktree's host still uses its existing screen fallback.

## Model and source evidence

`draw_screens.c` captures each 400..409 operation at its existing native handler.
Pictures retain decoded indexed sources before the loader reuses its buffers.
Credits retain 8x12 indexed glyphs, including spaces. Menu text retains its shifted
24x12 glyph layers, previous-position channel clears, character and font banks.
Clear/copy layers describe their source/destination rectangles, pitch, channel
mask and first-pixel row wrap. Callback edge records retain both old/new X/Y.
World column records retain each actual plane pass and its indexed column source.

The model materializes those operations into **independent native authoring
surfaces**, with immutable 512-pixel blocks shared between publications. These are
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
from 100.. events. `draw_screens_observe` paints these into the independent screen
target from existing native-owned `game_hud`, cels, tiles and foreground assets.
This preserves partial uploads, sprite byte fringes and stale HUD rows without
importing gameplay pixels. Their full-world projection remains the gameplay
renderer?s responsibility; a valid world view makes the screen view invalid.

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

The ownership restriction deliberately leaves `compositor.{h,c}` and presenters
untouched. Add these two calls in the shared compositor, using `enh_screens.h`:

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

Images/recipes are immutable **session-owned** objects. Destroy queued presenter
frames before `compositor_init`; snapshots do not survive that reset. As in the
existing session asset convention, no caller frees a borrowed image. This initial
capture arena retains history until that reset: long unattended menu sessions
therefore accumulate capture history. Reference-counted pruning is an integration
follow-up before treating this capture implementation as a bounded-memory host
service; it does not affect pixel coverage or the pure renderer API.

## Verification and review

`python tools/nativetest.py`: **29/29 pass** (the existing 27 plus the two screen
programs). The final screen-only rerun passes with 25 constructed checks and
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

`python tools/verify.py`: 86,428-byte program image and all MZ fields exact;
relocation set exact, with the pre-existing five full-file relocation-order bytes.
No C changed, so no translated-C differential-test result is implied by this work.
`python tools/hostbuild.py`: pass. The required faithful headless 45-second smoke
and widescreen smoke with `--wide-sim 464` reach the bounded exit **3**, 3,275 ticks,
3,153 publications and 349,944 PCM bytes. Faithful digest is `b32fdf580ab9c3b9`;
wide-simulation digest is `efedd6f11bf45941`. Width 468 is a presentation width;
wide simulation requires a multiple of 16. These host checks are regressions of
the current host, not a claim that the pending screen hooks are already wired.

`build/screens_shots/contact.bmp` and `manifest.txt` cover sixteen named views at
356, 468 and 640, including loading and the front title. Individual BMPs preserve
full resolution; creators has a native 640x480 image, with review thumbnails in
all three contact-sheet columns. No faithful image is an input to those exports.
