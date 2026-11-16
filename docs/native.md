# Native faithful presentation and audio

The milestone: the complete faithful game runs through native rendering and audio interfaces,
with no production dependence on the emulated VGA, CRT scan-out, Sound Blaster, DMA or PIC, and
those interfaces carry what the later enhancements (widescreen, interpolation, smooth camera,
smooth transitions, stereo effects, presentation controls; pre2_port's experience) need. The
reconstruction (src/ oracle, c/ translation, tools/difftest.py) stays the reference throughout.

```
                    input schedule (scan codes at logical times)
                                   |
 native/machine.c  logical time, coroutine, interrupts at service points  (timing boundary)
                                   |
 c/  translated game: simulation effects  ----------------+---- original rasterizer (VGA model)
      |  capture at the original draw/sound sites         |      reference only: parity tests
      v                                                   v
 c/scene.c  scene builder  -> immutable Pre2Scene        reference scan-out (host/present.c)
 c/audio_events  semantic audio commands                 reference mixer PCM (SbIrqHandler)
      |                                                   |
 native/render.c  scene + native assets -> indexed frame -> palette -> XRGB   <- pixel parity
 native/audio.c   commands -> sequencer -> mixer -> PCM 8403 Hz -> output    <- PCM parity
      |
 host/  SDL3: wall clock -> rt_run_until; shows the newest scene; submits PCM (resampling)
```

## Assessment (2026-11-13)

What the reconstruction gives us: every routine translated and differentially verified on the
original memory model; one top-level control flow (ProgramEntry) with nested screen loops and
busy-waits; drawing done by the game's own code into a planar VGA model; music and effects mixed
by the game's own tracker/mixer into DMA buffers, played through a Sound Blaster model.

What stood in the way of native presentation:

1. **Control flow owns the host.** ProgramEntry never returns; screens, fades, transitions and
   the frame loop are nested loops that spin on interrupt-driven memory. The old host serviced
   interrupts from these spins on wall time, so simulation timing depended on the host's clock,
   on the audio device draining its queue (the SB IRQ fired when SDL's stream ran low) and on
   how fast the host ran.
2. **Drawing and simulation are interleaved** in the same routines (the audit:
   build/audit/render_effects.md); some state the game needs lives in draw routines.
3. **VRAM is storage**, not only display: uploaded sprite tiles, the off-screen tile ring, saved
   backdrops, latch-copy sources.
4. **Audio state lives in game memory** and is advanced by the SB interrupt.

Decisions:

- **Keep the original control flow, run it as a coroutine.** Rewriting every nested loop into
  explicit state machines would reopen verified code for no gain the enhancements need: what
  they need is (a) the host deciding when simulation advances, (b) published scenes with
  logical timestamps, (c) explicit transition state. The coroutine yields at the original's own
  service points; the current loop is visible (pre2_phase, and the scene's screen/transition
  state). This is the "host-controlled stepping interface".
- **Logical time, not wall time.** Time advances only by what the original machine spent: waits
  jump to the next event, busy-wait turns, video bus time and period load time (all already
  established by the reconstruction: docs/timing.md, docs/smoke-test.md). A run is a function
  of its input schedule.
- **Capture at the source, not from memory at the end of a tick.** Draw routines record what
  they draw (with the transient data they consume) into a scene builder at the moment they draw;
  their simulation effects run as before, once per original step, in original order.
- **Two rasterizers during the transition, one in production.** The original VGA drawing stays
  compiled as the parity reference (pre2_raster = 1); production runs with pre2_raster = 0 and
  the native renderer, and must not fall back.
- **Audio: semantic commands at their origins; native engine with its own state**, verified
  PCM-exact against the game's mixer at 8403 Hz. Device resampling is a separate stage.

## Implementation sequence

| Stage | Content | State |
|---|---|---|
| 1 | Timing boundary: logical machine, coroutine stepping, deterministic runs; host on wall clock; turbo/headless scripted runs | done (this section) |
| 2 | Audits: render side effects + VRAM storage (build/audit/render_effects.md); audio (build/audit/audio_native.md); pre2_port survey (build/audit/pre2_port_survey.md) | done |
| 3 | Native asset access: pure SQZ decoder (native/sqz.c, 52/52 files equal), asset files | done |
| 3b | Video memory as storage made explicit (c/vram_store.h); production runs without a video memory model (tests/native/nullvga.c) | done |
| 4 | Contracts: scene events (c/scene.h), compositor + published frame (native/compositor.h, native/frame.h), audio commands (c/audio_events.h), parity harness | done |
| 5 | Gameplay capture + native renderer + pixel parity | done ([notes](native-render-rgame.md)); all 16 levels pixel- and view-equal |
| 6 | Screens/transitions capture + native renderer + pixel parity | done ([notes](native-render-rscreens.md)) |
| 7 | Native audio engine + PCM parity + output stage | done; [boundary and tests](../native/audio.md) |
| 8 | Production switch: no video memory model, native compositor, native audio; production frames = reference-checked frames | done |
| 9 | Enhanced renderer: level assets + semantic per-frame world data, any width/camera (docs/native-enhanced.md) | done; equals faithful playfield on 22,842 gameplay frames at 320 |
| 10 | Presenter: faithful / enhanced / widescreen modes, interpolation, smooth camera, smooth transitions (docs/native-presenter.md) | done; presentation never changes the game |
| 11 | True widescreen (opt-in gameplay variant): view geometry parameter, growable pools, region policies (docs/native-widescreen.md; audit build/audit/view_geometry.md) | done; default geometry byte-identical |

## Timing boundary (native/machine.c)

- The game runs on a fiber. Service points: platform_tick_wait (frame wait), platform_poll
  (busy-wait turn), 3DAh reads, DOS loads. The host calls `rt_run_until(T)`; the game runs until
  its logical time reaches T, then yields.
- Events, in logical-time order: PIT channel 0 ticks at the programmed divisor (exact rational
  schedule), keyboard scan codes queued with a logical time, audio block completions, CRT edges
  (retrace start: start-address latch; line 0: display frame) from the CRTC timing registers.
- Logical time advances by: a frame-tick wait jumps to the next event; a busy-wait turn costs
  RT_POLL_TURN_NS (10 us, a 386 loop turn: also the rate at which WaitForButtonPressRelease's
  PollInputState records into the demo log on the late-year screen); video bus time (0.5 us per
  memory access, 1 us per port access, as counted by the model); period load time (30 ms per
  open, 10 us per byte).
- Interrupts are delivered at service points (as before: a timing the hardware can produce).
  The audio block clock is the card's own: a block started from the IRQ of the previous one
  starts when that one ended, even if the IRQ reached the game late.
- The audio device and the display never drive the game. PCM blocks carry the logical time they
  start playing; display frames carry the logical time of their display start.
- Evidence: tests/native/stepping.c runs 40 s of scripted play stepped at 60 Hz, 240 Hz and
  irregular 0.1-80 ms slices: identical memory, VGA state, tick/block/display counts and PCM.
  The smoke test and the four transition scenarios plus the attract demo reach their targets
  (`pre2 --smoke 45 --headless`: 45 s logical in ~1.2 s; 24.30 frames/s, 72.81 Hz ticks, 49.96
  audio blocks/s in gameplay).

## Video memory as storage (c/vram_store.h)

The audit's census of video-memory read-backs that reach game state or control flow:

| Read-back | Production replacement |
|---|---|
| ProbeVideoMemory (A000:2000h) chooses the adapter path | the native machine has a VGA: the probe makes its accesses and succeeds (platform fact, like the SB/CPU probes) |
| ClassifyTileCache: colour compare over the tile cache -> TileClassTable, mask table, MixedTileCount, GridDirtyFlag (the vertical camera's step table depends on GridDirtyFlag) | the uploads also write the explicit tile cache (pre2_tile_cache); the classifier computes the same compare from it |
| LoadLevel's stash: the level segment saved in four planes at 9DC0h while the sprite sheet overwrites it, then read back (16-bit wraps on both sides) | explicit stash buffer (pre2_level_stash) |
| LoadLevel's wrong-segment word A000:2875h -> BACK:2875h | written after the backdrop upload; no later consumer (audit U4); in production it reads 0 like a cleared page |
| DAC read-back (fades, PRESENT morph) | the register file stays modelled (it is logical game-visible state); the published frame copies the palette from it |

Everything else the game reads from video memory feeds only pixels (latch copies, AND/OR
masks). Evidence: tests/native/nullvga.c runs six scenarios (start-up to level 1, warp,
ending, game over, castle, attract demo) with video memory modelled and not modelled and
requires identical memory at every main-loop frame and step change, identical PCM and counters.

## Presentation (c/scene.h, native/compositor.h, native/frame.h)

- The game's original drawing code keeps running as the carrier of its simulation effects
  (render_effects.md: drawn flags, the player's hurt timer, SnowFrameTick, HUD life awards,
  wind RNG, particle disposal, shake) and of its video bus time. Each drawing operation also
  emits one scene event at its original site with semantic parameters.
- The compositor draws each event's visual effect natively from decoded assets into its own
  indexed video store, laid out as the game's drawing assumes (pages, tile ring, backdrop and
  other off-screen surfaces as regions), so page persistence, partial updates and the
  original's address arithmetic carry over. No VGA registers, latches or write modes.
- At each CRT display start (logical time, start address latched at the retrace) the visible
  image is scanned out of the store into an immutable Pre2Frame: DAC indices, palette, size,
  blanking, the semantic gameplay view (camera, sprites with stable identities, dots) and the
  transition state. Drawing a frame changes nothing in the game.
- Production: video memory not modelled (vga_memory_off), compositor frames presented.
  `pre2 --reference` models video memory and scans the VGA model out (diagnostics).
- The register file (CRTC, attribute, DAC) stays modelled in production: the game reads the DAC
  back, the CRT timing comes from the CRTC timing registers, and the scan-out geometry (start,
  pitch, wrap, pan, display end) is taken from it when a frame is published. It is logical
  register state, not an emulated display: nothing is scanned from it.

## Audio (c/audio_events.h, native/audio*.c, native/audio.md)

- Every origin emits a command (27 SFX sites with source kind/record/position, song
  begin/end with the loader padding the mixer can read, output start/toggle/ready, device).
- The native engine owns pristine assets (SAMPLE.SQZ, 12 TRK via native/sqz.c), the faithful
  destructive playback images, the tracker, the integer mixer (with the original's aliasing in
  a private arena) and the play/mix buffer pipeline; it never touches game memory.
- The machine runs its block clock (119 us samples, 168-byte blocks) on logical time; the game's
  SB/DMA/PIC traffic is ignored in production. PCM blocks carry their logical start time;
  SDL resamples them to the device (the output stage).
- With native audio the game's own mixer fields (DATA 0B5Dh..12C0h ranges listed in
  native/audio.md) stop advancing; nothing outside audio reads them (audit section 4), and
  tests/native/audio_parity.c requires every other byte equal at every main-loop frame. A whole-
  memory digest therefore differs from a `--reference` run by exactly those ranges.

## Verification

| Claim | Test | Result |
|---|---|---|
| Simulation independent of host stepping/presentation rate | tests/native/stepping.c | equal (60 Hz, 240 Hz, irregular) |
| Video memory is presentation only | tests/native/nullvga.c | equal in 6 scenarios |
| Native SQZ decoding | tests/native/sqz.c | 52/52 files byte-equal |
| Pixel parity of the compositor at every display frame | tests/native/render_parity.c | 30,825 display frames in 6 scenarios (start-up, titles, front end, map, level 1, warp into 0Ah, ending, game over, castle, attract demo): 0 differ |
| Gameplay over every level | tests/native/game_levels.c | 16/16 levels (bonus levels via the warp table, a death and checkpoint respawn in level 0), ~950 display frames each: pixels and per-page semantic views equal |
| Drawing operations in isolation | tests/native/game_sprites.c, game_operations.c, screens*.c | all 460 sprite cels x mirror x pass modes + edge matrix (4,176 cases); 875 completed gameplay operations; 1,052 screen store checks; ENTER CODE, ending (mode 12h), castle, game-over frames |
| Production publishes the checked frames | tests/native/production_frames.c | no video memory model + native audio vs reference: 30,825 frames identical (pixels, palette, view, transition) |
| Presenting more often changes nothing | tests/native/present_purity.c | 4 scenarios equal with 4x publication + extra publishes |
| Enhanced renderer = faithful at 320 | tests/native/enh_levels.c, enh_scenarios.c | 22,842 gameplay frames equal (iris/reveal/wipe/tally frames are transitions, excluded) |
| Enhanced renderer at 480/640/864 | tests/native/enh_wide.c, enh_effects.c | 1,116 bounded cases |
| Presenter changes nothing | tests/native/presenter_purity.c | 386,040 renders at 60/144/240 Hz with every option: game memory, counters, PCM, publications equal |
| Interpolation and transitions | tests/native/presenter_motion.c, presenter_transitions.c | identities, spawn/despawn, reuse, cuts; transitions monotonic, never ahead, equal at original steps |
| True widescreen default identity | all difftests, all native tests at the default geometry | 5,858/5,858 equal; native suites unchanged |
| True widescreen variant | tests/native/wide_sim.c, wide_pools.c, wide_regions.c, wide_capture.c, wide_geometry.c | 16 levels at 480 and 640, twice each: deterministic, 0 invariant failures; restricted rooms keep the 320 aperture |
| PCM parity of the native audio engine | tests/native/audio_parity.c, audio_songs.c | 20,857 scenario blocks and 175,566 song/control blocks byte- and time-equal |
| Translation unchanged | tools/verify.py, tools/difftest.py | exact; suites equal |

## Limitations and open items

- Interrupts are delivered at service points (unchanged from the reconstruction's host); the
  audio block clock keeps the nominal 19992 us period instead of drifting with that delivery
  delay (a declared reference convention; build/audit/audio_native.md section 7).
- Coverage is the scripted scenarios, 12 s of every level, the constructed operation tests and
  the constructed song runs. Not yet run end to end through the native path: the level 2 and 8
  boss fights to their end, the level 9 script finale, every world-map node, the Ctrl+Alt+E
  and Ctrl+Alt+W keyed screens during play. The differential tests cover their logic; the
  gameplay and sprite handlers they use are the ones verified above.
- In production the register file (CRTC, attribute, DAC) stays modelled: the game reads the DAC
  back and the CRT timing comes from it. The published frame copies palette and geometry from
  it; nothing is scanned from video memory.
- Production still executes the original drawing routines (their simulation effects and bus
  time are part of the original step); their video-memory writes go nowhere. Separating the
  pure pixel loops from those routines further is an optimisation, not needed for fidelity.
- Capture handlers read game memory at the event (sprite tables, strings, map cells) and copy
  assets into native ownership at load events; the gameplay view is filled from the per-page
  state recorded at draw time and selected by the latched start address.
- The 36 free-follow cave/room records have recorded arrival rectangles but no proven outer
  extent (docs/native-widescreen.md lists the per-record blockers); the wide window treats them
  conservatively. Level 0Dh's earthquake staging uses per-phase visible/forbidden rectangles.
- The settings overlay and live presenter rebuild are verified headlessly (model, rendering,
  pause purity); live SDL interaction and audio pause have not been exercised interactively.

## Presentation and gameplay options (host)

`--mode faithful|enhanced|widescreen`, `--aspect 16:9|16:10|21:9|window`, `--interpolate`,
`--smooth-camera [--camera-tau S]`, `--smooth-transitions`,
`--hud left|centre|right|anchored`, `--backdrop black|mirror|stretch`: presentation only.
Settings persist in `pre2_settings.json` (SDL preferences directory, `--settings PATH`;
defaults < JSON < command line) and F10 opens the paused settings overlay
(docs/native-host.md). `--wide-sim WIDTH|auto` selects the true
widescreen gameplay variant for the run (implies the widescreen presentation; `auto` takes the
width from the aspect); demos and the attract mode always run in the original geometry.

## Extension points for the enhancement milestones

What the later enhancements attach to, and what they must not touch (pre2_port's experience:
build/audit/pre2_port_survey.md; its policies are hypotheses, not original contracts).

| Enhancement | Attaches to | Rule |
|---|---|---|
| Frame interpolation | consecutive Pre2Frames whose `game.sim_frame` differ: previous/current FrameGameView, FrameSprite.id (slot + generation), world positions, `camera_cut` | draw at display refresh from the two views; never step the machine to get a frame; spawn/despawn = id absent in one view; no 32-pixel guessing where `gen`/`camera_cut` say it |
| Presentation camera / smooth camera | FrameGameView.camera_x/y (simulation camera), player sprite identity, shake_rows separate | a presenter-owned camera state; the simulation camera is read only |
| Camera shake | FrameGameView.shake_rows | the original applies shake in CopyRingWindowToPage and the sprite pass, and moves PlayerY in UpdateCameraShake (simulation): a presenter may re-apply or smooth the visual offset only |
| Widescreen / variable viewport | a level asset query by `level_epoch` (map cells, decoded tiles, animation phase, backdrop) to draw beyond the 320-pixel aperture | presentation-only widening never changes which objects are simulated; widening the active area (on-screen predicates, pickup projection budgets) is a separate, explicit gameplay policy |
| Present-time transitions | Pre2Frame.transition (kind, phase, step/steps, iris centre/radius) and the frames before/after | the original's discrete progression keeps running in the simulation; a presenter may draw it continuously but must not release gameplay early |
| Stereo SFX / alternative audio backends | AudioCmd (c/audio_events.h): every trigger with source kind, record, world position, frame; the native engine's separated asset / sequencer / mixer / output stages | the faithful mono mix stays the reference; panning is a later output-stage policy using the presenter's listener (camera), not the original 320-pixel screen |
| Presentation controls (aspect, scaling, refresh, HUD placement) | the host and Pre2Frame only | no control may alter the machine, the input schedule or logical time |

Known restrictions to record before widening the view (from the port, to be confirmed against
the oracle when that milestone starts): the gorilla boss level keeps faces/glyphs in off-screen
map columns (map data used as graphics); the tower level and level 0Eh are fixed rooms whose
neighbouring columns are other rooms; caves and bonus rooms with the room flag or level flag
bit 2 are fixed-camera rooms; level 0Dh stages earthquake pillars below the floor; level flag
bit 4 forces vertical scrolling; pickups are animated only inside a 20-slot projection budget;
enemy activation and despawn use their own windows (320x300 despawn bounds).

## Progress record

- 2026-11-13: stage 1 done; audits and the SQZ decoder delegated.
- 2026-11-13: audits integrated; SQZ decoder integrated; video memory read-backs replaced by
  explicit storage (nullvga equal); scene/compositor/frame/audio contracts and parity harness
  written; host presents compositor frames by default. Workers: audio engine, gameplay
  renderer, screens renderer (in parallel worktrees).
- 2026-11-13: stage 7 done (audio worker, reviewed and merged). Production selects the
  command-driven native audio engine; the reference SB path stays selectable. Six scripted
  scenarios: 20,857 PCM blocks and per-frame non-audio state equal. Twelve complete song/order
  loops plus repeated SFX, F3, changes during SFX and /B: 175,566 PCM blocks equal.
  native/audio.md documents the compatibility arena bounds and loader-padding inputs.
- 2026-11-13: stage 6 merged (screens worker, reviewed). render_parity, scenario none: every
  start-up, title, front-end and map step pixel-equal at every display frame (TITUS 161,
  PRESENT 522, front end 355, map scroll-in 837 frames); level entry and gameplay wait on
  stage 5. Screen tests: screens, screens_frames (ENTER CODE), screens_gameover pass.
- 2026-11-13: stage 5 merged (gameplay worker, reviewed); view/transition hooks now receive the
  latched start address (the page on screen, not the page just requested). Stage 8: production
  runs without a video memory model with the native compositor and native audio;
  production_frames proves it publishes the reference-checked frames. All 5,858 differential
  cases equal, tools/verify.py exact, 12/12 native tests pass.
- 2026-11-14: enhancement phase. View-geometry audit (build/audit/view_geometry.md: 55
  decision sites, pools, level facts; port claims checked). Enhanced renderer, presenter and
  true widescreen merged after review. Integration fixes: presenter camera and image width use
  the simulated view width; `--wide-sim auto`; the HUD strip's rectangle copies are kept in the
  wide variant (the HUD stays 320 wide in every geometry).
- 2026-11-14: unified native presentation (the model of rayman_recon's single native renderer;
  pre2_port's solutions checked against the C translation). Every enhanced/widescreen pixel is
  drawn natively; the faithful image is only the 320 reference: native screens at any width
  (docs/native-screens-wide.md: menu ring and CARTE extend, 320-column pictures get black
  sides; 26,584 screen frames equal at 320; capture memory bounded by retain/release), HUD
  from its four fields (lives, score, energy, BONUS) moved whole in wide layouts, native iris /
  curtain / room wipes at any width, backdrop wrap where seamless, region windows instead of
  masked 320 apertures (docs/native-widescreen.md), margin terrain sources in presentation-only
  widescreen. presenter_native: zero faithful pixels over 23,457 publications in four
  enhanced/widescreen configurations, enhanced 320 equal to faithful at every one. JSON
  settings and the F10 overlay with pause purity (docs/native-host.md).
