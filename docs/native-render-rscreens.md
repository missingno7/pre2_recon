# rscreens native renderer

Implemented in this worktree, 2026-11-13. Ownership is limited to the requested screen
files; `native/draw_game.c` remains the empty baseline family here. No ASM, assets,
gameplay sources or audio functions were changed.

## Coverage and capture

`c/scene_screens.h` documents kinds 400..409. Native handlers draw indexed pixels into
`comp_planar` / `comp_chain4`; they never read VGA planes, registers, latches or write
modes. Picture uploads capture their source bytes before asset reuse, with one event
per original plane pass. Credits glyph operations capture their 48-byte glyph into
native scratch. Callback background uploads precede the map buffer's replacement by
the eight font banks. The completed banks and stamped world map have native owned
64-KiB snapshots and explicit capture epochs, invalidated at compositor registration
and replaced on every corresponding setup. No cache retains a game-memory pointer.

Covered: TITUS; PRESENT image, replacement logo and palette morph; UNION/MENU and
front-end callbacks; MODE and ENTER CODE; credits and decimal/text printing; world
map marker and rolling columns; planar pictures; intro backdrop save/restore;
game-over clear, misplaced picture and scrolling strip; CASTLE; THEEND; mode 12h
plane-pair intro; display clears. Palette setters, fades and blank/unblank retain
their original logical register traffic, already consumed by scanout/publication.
`SetLevelVideoMode` has no additional pixel writes beyond the core BIOS clear; page
state and palettes still run unchanged. `startup.c` has no pixel operation to mirror.

## Quirks preserved

- All addresses, source offsets and forward overlapping copies wrap at 16 bits.
- World columns mask the destination to 1FFFh at every row, retain the original
  inter-plane destination arithmetic, and leave the source unmasked.
- Callback edge copies are ordered column then rows; unsigned low-byte row counts,
  41-byte row-copy widths and zero-count LOOP wrap are preserved.
- Text erases planes 2/3 at the previous start and draws the shifted bank at the
  current start; only each row's first address is masked (its next two bytes can
  cross the ring boundary). Spaces use glyph 2Bh; below-'0' terminators and the two
  fonts' different character remaps remain distinct.
- Credits spaces clear all planes. Decimal leading spaces retain their cell writes.
  The startup font's overwritten word is captured as it actually exists in RAM.
- Intro saves 8000 bytes at 3F40h and restores exactly those bytes to each draw page.
- Game-over clears 3F40h but uploads at 5E80h; the untouched lower 24 display rows
  persist. The strip copies 7040 bytes starting at 3F40h + 40*FineScroll.
- Planar uploads publish per plane. Combining all planes early caused one extra
  attract mismatch; completed plane-pass events preserve the original CRT ordering.
- Palette fade/morph termination, overshoot and continuous 82-component retrace
  counter are unchanged. Palette changes require no native pixel rewrite.

## Transition hook

`void draw_screens_transition(FrameTransition *t)` is declared in the screen header.
It only writes when a screen transition is active. **The integrator must chain it
from `compositor_transition`; this family does not install or replace that hook.**
Kinds: 4 palette fade (phase 0 closing / 2 opening; pass progress, nominal 16/32
passes), 5 map scroll (cx/cy = original pixel scroll position). World reveal has
640 steps; callback scroll is cyclic and uses steps=0. Mode changes and completed
fades clear the active state. The logical DAC remains the exact palette source.

## Validation

- `python tools/difftest.py screens credits system render`: 669/669 equal.
- `python tools/verify.py`: 86428-byte program image and all MZ header fields exact;
  relocation set exact. Existing full-file relocation-order difference: 5 header bytes.
- `python tools/nativetest.py stepping nullvga sqz`: 3/3 pass (all six null-VGA
  scenarios; all 52 SQZ/TRK files).
- `python tools/nativetest.py screens`: 1052 full-store checks, zero differences.
  Includes 200-frame MODE and ENTER CODE constructions through both wave signs,
  entry/error/reset/cancel, a different stage marker and full world scroll, credits,
  16-bit text/clear/backdrop wrap, TITUS/PRESENT/CASTLE/THEEND, keyed mode 12h.
- `screens_frames`: CRT plus full-store checks for 1995/1997 startup/attract, 240
  ENTER CODE frames, ending and castle. No differences outside current sprite
  rectangles on either page. Picture checks also run inside the coarse
  `MainFrameLoop` diagnostic phase, so THEEND and mode 12h uploads/fades are covered
  (115 mode 12h CRT frames in the ending scenario).
- `screens_gameover`: 210 constructed original game-over frames with real decoded
  sprites; zero differences outside current sprite rectangles on either page.

## Integration dependencies / remaining parity output

`python tools/nativetest.py render_parity` still exits 1 with the baseline gameplay
family. `render_parity --store` shows TITUS, PRESENT, ordinary front end and world
map pixel- and store-equal in all six scenarios. CASTLE's button hold is equal;
the separate CRT checks cover its picture and the ending pictures under nested phases.

- Attract `RunFrontEnd()`: 411 differing visible frames, first frame 2416 (685
  pixels); 414 store-differing frames, first address 14C8h. All remaining store
  differences are inside the current intro sprite rectangles, independently
  checked at each CRT display by `screens_frames`. Waits for DrawSpriteObjects.
- Game-over sprites likewise wait for DrawSpriteObjects. Their background, displaced
  image/cache and rolling strip pass the independent 210-frame whole-store exclusion
  check. UpdateGameOverSprites remains the original simulation, including ordering.
- `SetLevelVideoMode()` is also the inherited phase name during LevelStart:
  20 differing visible frames per level entry (first frame 2210 in the ordinary
  script, 3365 in attract; 2816 pixels); first store difference 4F0Ah is the stale-ES
  slot fill in `c/game.c`. Ring uploads, HUD and level-entry rendering belong to rgame.
- The gameover scenario has 5 differing `FadeOutPaletteToBlack()` frames, first
  frame 2711 (7360 pixels), store starts at 1B80h: persistent lower display/HUD rows
  not mirrored by the empty gameplay family. They are outside the strip this worker
  owns and need rgame's preceding HUD/page writes.
- MainFrameLoop differences are expected until rgame is integrated. No remaining
  independent screen-pixel mismatch was found. Rerun the full parity suite after
  integrating rgame; these results do not claim its missing operations pass.
