# Native widescreen simulation variant

The widescreen simulation is an opt-in run configuration: `--wide-sim 480` or
`--wide-sim 640`. The accepted range is 320..640 in multiples of 16; 320 keeps
the compatibility mode, including original capacities. Height remains 176 pixels. A run chooses its
geometry before level entry and keeps it for that level. Demos and the attract
mode use the original 320x176 geometry. `rt_set_wide_sim_width(width)` latches
the request before `rt_init`; it does not change an executing level. LevelStart
selects the variant only when DATA byte_2879 (demo playback) is zero. The front-end
intro runs before that selection, and exit/tally/game-over explicitly select the
320 stage. The configuration stays latched for the next level; it is not a live
host presentation control. The native region table in
`native/level_regions.c` supplies the room exceptions and camera limits; the
same rules are exposed to `c/` through the runtime's region-policy callback.

## Region policy

`native/level_regions.c` now has separate simulation and presentation contracts.
The existing `pre2_region_policy` callback keeps the opt-in simulation variant's
limits. `pre2_region_for_view` returns a half-open rectangle `(x0,x1,y0,y1)` in
world pixels, an identity, an evidence string and `proven_extent`. Backing-map
storage remains distinct: `EnhLevelPolicy` excludes the level-09 graphics bank
but cannot globally exclude LEVEL6's lower room along with its tower.

The presentation main extents follow the shipped headers and MapRows:

| Level | Main world rectangle (pixels) | Evidence / exception |
|---|---|---|
| 00h | 0..4096, 0..784 | LEVEL1 header; ScrollViewDown |
| 01h | 0..2048, 0..1664 | LEVEL2 camera limit 108, plus 20 columns |
| 02h | 0..4096, 0..784 | LEVEL3 header |
| 03h | 0..4096, 0..720 | LEVEL4 camera limit 236 |
| 04h | 0..4096, 0..2048 | LEVEL5 header |
| 05h tower | 0..320, 0..2048 | LEVEL6 flags 06h, horizontal follow disabled |
| 05h lower | 560..1056, 0..2048 | cave 7E0F -> 7223 -> 7925; arrival column 35, right limit 46; MapRows |
| 06h | 0..4096, 0..2048 | LEVEL7 header |
| 07h | 0..4096, 0..1376 | LEVEL8 header |
| 08h | 0..4096, 0..1760 | LEVEL9 header, including arrivals at column 236 |
| 09h | 0..320, 0..192 | columns 20..49 are panel graphics sources, not world |
| 0Ah | 0..2560, 0..384 | LEVELB camera limit 140 |
| 0Bh | 0..2560, 0..816 | LEVELC camera limit 140 |
| 0Ch | 0..4096, 0..816 | LEVELD camera limit 236 |
| 0Dh main | 0..4096, 0..336 | reveal blocks' final base at row 21; bonus rooms below |
| 0Eh | 0..320, 0..2768 | LEVELF camera limit 0, vertical travel retained |
| 0Fh | 0..4096, 0..1344 | LEVELG header |

The earlier wide-simulation callback contains conservative 124/140/140 limits
for 01h/03h/0Ch. They are **not** the shipped header values. This presentation
change does not alter that callback or the opt-in simulation's game state.
The original conditional right-scroll fallback is preserved: a published camera
past the main header does not get pulled into another room. Its aperture is an
explicit unresolved extent until that room's outer geometry is established.
Boss min/max X restrict boss motion (`UpdateBossFight`), not the camera; they
are not imported as artificial arena walls.

All **44** shipped cave source/camera/destination/gate records are explicit in
`pre2_region_caves`, with evidence at each entry. The eight nonzero-gate records
prove a stationary authored 320x176 screen: both normal follow and idle-look
scrolling test `ScrollFollowGate`. Their claimed rectangles are disjoint from
the other known fixed-room rectangles in the same level.

The other **36 records remain unproven outer extents**. Their stored rectangles
are **arrival apertures only**. The follow-up investigation proves that every
one of these apertures is too small to be the room's outer extent: the original
camera can leave it while the player stays at the destination, without crossing
a collision wall. None was promoted to `proven_extent=1`.

`UpdateCameraScroll` (`MODULE3.ASM`, `loc_565F`/`loc_5662`) has no per-cave clamp
when the gate is zero. The idle-look branch of `UpdatePlayerStateIdle`
(`loc_5D83`) calls the original scrolling leaves. For destination column P,
left-looking reaches `max(0,P-17)`; right-looking reaches `min(P-2,L)`, where
`L=CameraLeftLimit` if that signed limit is at least `P-20`, otherwise 236.
`ScrollViewLeft` checks zero; `ScrollViewRight` checks this conditional limit
(`MODULE2.ASM`, `loc_3457`/`loc_345A`/`loc_3487`). A wall cannot prevent these
camera-only pans. Normal movement may reach still more; the measured pans are
lower bounds on coverage, not complete room bounds.

`regions_sweep` constructs the exact post-arrival camera, destination, gate and
header fields for every record, including LEVEL6's flag clearing. It executes
16 translated fixed-follow steps, then the two translated idle-look scrolling
leaves under the original predicates. No original executable is booted. The
constructor detaches the VGA callbacks and scene sink during edge draws, then
restores all MEM_SIZE bytes, VGA state, ES, access counters and callbacks before
the suspended scripted run resumes; logical time and every machine statistic
must be unchanged. For claimed extents it asserts camera containment and no
intersection with another known fixed room. This is a bounded camera-routine
proof, not an all-input player/collision reachability proof.

The following blockers remain. Empty tile zero has side class 0, ground class 0
and flags 0 in every decoded level; blank strips cannot serve as collision
barriers. A side-class-only flood fill is insufficient because it ignores
landing, ceilings, jump reach and map mutations. The native sweep checks the
three tile-zero classification bytes against every decoded level. Source/destination pairs alone supply no room-ownership labels.

| Key | Missing outer-bound proof |
|---|---|
| A | 00h: global camera envelope includes the two fixed bonus locations 00DC/0065; local movement and mutable-boundary isolation remain unproved. |
| B | 01h: header limit 108 also admits the fixed 006B bonus columns; it does not separate the bonus from the vertical main area. |
| C | 02h: upper and lower destination areas share ordinary follow; no enclosing, mutation-safe barrier has been established. |
| D | 04h: arrivals at rows 0/66/101 share the global 128-row map limit; there is no cave-local vertical clamp. |
| E | 05h: the arrival at column 35 can look left to column 20. Columns 35..65 prove an art band, not an outer camera rectangle. |
| F | 06h: the 00h cave records recur verbatim in a different 128-row map; 00h's shapes cannot be inherited. The fixed bonus locations still need isolation. |
| G | 07h: paired cave routes share ordinary follow; room walls and all mutable openings have not been bounded. |
| H | 08h: the EC right-camera limit is established, but the left separation of the D0/EC destination areas is not. |
| I | 0Dh: LevelPropHeader 9 is conditional on player Y; reveals change floor classes. A universal camera-floor invariant separating the lower bonuses is not proved. |

Every unproven record is listed below. X is the measured stationary idle-look
camera interval in tile columns; Y is the settled camera top in pixels. These
are counterexamples to treating the arrival aperture as an outer extent.
`build/regions_shots/cave_arrivals.tsv` contains all 44 records, including the
eight stationary-screen proofs. Each native entry also carries its blocker.

Only a locked screen (a trigger record with a nonzero follow gate) is a room: the
simulation variant keeps it at 320 and the presenter centres it. A free-follow
arrival scrolls like the main region, so it keeps the requested width in the
simulation and is not a presentation region; its camera stops where the wide
view reaches the arrival's right edge. This is the legacy port's rule too. The
36 free-follow records have no mapped outer walls, so the wide view can show
what lies beside them in the map, as the original's own idle look does at 320.
`--smoke 50 --scenario cave` walks record 0836h and its way back 2E7Eh (the
view stays 464 with `--wide-sim auto --aspect 21:9`).
Two player-relative checks follow the variant's width as well: the approach
spawner's near limit (a screen width; `SpawnChildOnApproach`) and the pickup
projection's left edge (two tiles of slack, as on the right). The ring spawner
(`SpawnChildOnPlayerBox`, offsets -120..+120 at ds:A343h) is left as authored:
its children rise from the ground on screen in the original too.

| Level | Source | Camera | Destination | Idle X | Settled Y | Blocker |
|---|---|---|---|---:|---:|---|
| 00h | 2725 | 2440 | 2E4C | 59..74 | 592 | A |
| 00h | 07E5 | 19DC | 20E4 | 211..226 | 400 | A |
| 00h | 0836 | 2672 | 2C7E | 109..124 | 608 | A |
| 00h | 2E7E | 0032 | 0636 | 37..52 | 0 | A |
| 00h | 076E | 1EC7 | 22D1 | 192..207 | 480 | A |
| 00h | 220F | 1BDB | 1FE4 | 211..226 | 432 | A |
| 01h | 0526 | 5767 | 5B6F | 94..108 | 1360 | B |
| 01h | 0875 | 5965 | 5E6F | 94..108 | 1408 | B |
| 01h | 3537 | 4E19 | 5223 | 18..33 | 1216 | B |
| 01h | 0874 | 5767 | 5E6F | 94..108 | 1392 | B |
| 01h | 253C | 3A01 | 410C | 0..10 | 928 | B |
| 01h | 1533 | 0200 | 0703 | 0..1 | 16 | B |
| 02h | 1F0E | 0257 | 045E | 77..92 | 0 | C |
| 02h | 2A38 | 2677 | 2E7F | 110..125 | 608 | C |
| 04h | 6B12 | 651F | 6C27 | 22..37 | 1616 | D |
| 04h | 7BF0 | 00E7 | 07F0 | 223..236 | 0 | D |
| 04h | 5ADB | 42D2 | 43DB | 202..217 | 976 | D |
| 05h | 7E0F | 7223 | 7925 | 20..35 | 1824 | E |
| 06h | 2725 | 2440 | 2E4C | 59..74 | 624 | F |
| 06h | 07E5 | 19DC | 20E4 | 211..226 | 400 | F |
| 06h | 0836 | 2672 | 2C7E | 109..124 | 608 | F |
| 06h | 2E7E | 0032 | 0636 | 37..52 | 0 | F |
| 06h | 076E | 1EC7 | 22D1 | 192..207 | 448 | F |
| 06h | 220F | 1BDB | 1FE4 | 211..226 | 400 | F |
| 07h | 2709 | 4169 | 456F | 94..109 | 1040 | G |
| 07h | 456F | 1A04 | 1D07 | 0..5 | 400 | G |
| 07h | 0E09 | 2B74 | 3178 | 103..118 | 688 | G |
| 07h | 3477 | 030A | 0813 | 2..17 | 48 | G |
| 08h | 55CD | 5CEC | 63F2 | 225..236 | 1472 | H |
| 08h | 07F5 | 0310 | 0918 | 7..22 | 48 | H |
| 08h | 2C59 | 5ED0 | 61D9 | 200..215 | 1488 | H |
| 08h | 55CC | 5EEC | 62F2 | 225..236 | 1504 | H |
| 0Dh | 1305 | 09C1 | 12CC | 187..202 | 144 | I |
| 0Dh | 2433 | 0AC2 | 11CB | 186..201 | 160 | I |
| 0Dh | 2419 | 0A7F | 1189 | 120..135 | 160 | I |
| 0Dh | 2400 | 0914 | 121E | 13..28 | 144 | I |

The lower LEVEL6 art band retains the full MapRows height: arrival row 114 is
not a top clamp. Its cleared flag 02h identifies the encounter even after a
left pan. Its `proven_extent` is now zero as well: the new constructor falsified
the former claim that the art band contained every camera position.

`pre2_region_never_visible` exposes the selected policy's forbidden exterior
ranges and earthquake banks. That policy remains conservative while the above
outer extents are unresolved. It is not a proof that every nonzero map cell is
owned by the current room. The scripted runs check reached camera positions;
they cannot replace the missing movement- and mutation-aware isolation proofs.

### Presentation window

`pre2_present_window` in `present_motion.c` returns the camera and visible world
intersection. When the rectangle is at least as wide/tall as the requested view,
the whole window clamps inside it. At an edge the original simulation aperture
moves off centre. A smaller rectangle is centred with literal RGB black outside
its extent. The presenter renders the full requested width through `enh_render`;
it no longer renders a separate 320-pixel room image and centres that image.
The HUD keeps its own anchoring, centred for narrow rooms. An unsmoothed
320x176 endpoint that originally panned into void retains its exact aperture;
wider windows still rest at the art edge.

The smooth camera uses this same clamp and retains region identity separately from player identity in its seed
tag, so changing region reseeds it. Forced downward scroll still follows published
simulation Y. The helper supports taller playfields; the current host gameplay
output remains 200 rows including the 24-row HUD.

### Earthquake pillars (0Dh)

The phase is per proximity record, not a global earthquake byte. LEVELE records
12/13/14 have sources 133Ah/132Ch/13B0h, sizes 4x6/4x6/4x8, and +8 countdowns
4/4/6. `UpdateProximityTriggers` (ASM `loc_5427` onward) moves the block up one
row every fourth tick, decrements +8, and sets +4 from FFFEh to FFFFh on completion.
`SavePristineTriggerRows` saved the old rows; every step restores its old bottom
row behind the rising copy. The hidden storage therefore survives completion.

`pre2_region_reveal_visibility` now describes the **tile-rectangle silhouette
at each phase** from the immutable initial map and current published map. It
returns the detected phase, the visible moving copy and two never-visible
rectangles: the hidden moving tail and the restored original rows. It reads no
live trigger state. Half-open row ranges below become world pixels by multiplying
by 16; X is 58..62, 44..48, or 176..180 respectively.

| Records / phase p | Visible rising copy | Never-visible moving tail | Never-visible restored rows |
|---|---|---|---|
| 12/13, p=0..4 | [19-p,21) | [21,25-p) | [25-p,25) |
| 14, p=0..6 | [19-p,21) | [21,27-p) | [27-p,27) |

The first two pillars expose 2/3/4/5/6 copied rows in phases 0/1/2/3/4; the
third exposes 2/3/4/5/6/7/8 in phases 0..6. Empty intervals disappear at the
endpoints. Actual tile coverage continues to come from native tile textures;
these rectangles describe source provenance rather than synthesize pixels.

The two forbidden portions have a **constant union**: [21,25) or [21,27).
There is no basis for exposing that bank as the countdown advances: doing so
would reveal the restored original art. `pre2_region_staging` and the existing
seven-rectangle `pre2_region_never_visible` interface keep returning that union,
so renderer consumers remain compatible. Callers inspecting the individual
phases use the new visibility structure. An unrecognised/partially modified map
returns phase -1, no visible-copy claim and the whole bank as forbidden.

`regions_reveal` independently staggers the three trigger starts and reaches
**all 175 joint phase combinations** through the translated reveal routine.
It checks 525 per-pillar silhouettes, moving-copy bytes against their original
shifted source, restored bytes at their original coordinates, +0/+4/+8 state,
read-only map inference and conservative handling of a damaged phase pattern.
No in-progress countdown is paused or rewritten to manufacture a combination.
In 167 of the 175 combinations, row 19 no longer consists entirely of ground
class 1. This falsifies a phase-blind solid-floor argument; it does not by
itself prove that the player can fall through, because the remaining contact
and slope rules still need to be considered.

The row-21 presentation horizon follows the three final bases
`19+6-4 = 19+8-6 = 21`. It is a content mask, **not a proven physical camera
bound**; the 0Dh main policy now has `proven_extent=0`. The original vertical
reference 9 is conditional, and the moving copies change which floor tiles have
ground class 1. An original 320 endpoint can still preserve its overshoot for
parity; a wider presentation keeps the storage hidden. No 149-pixel cap or
unconditional player/camera-floor proof is asserted.

### Follow-up verification

`python tools/nativetest.py` passes **32/32 in this worktree** (the supervisor's
main-checkout 34-suite run is separate). This includes all five region suites,
16-level gameplay/enhanced parity, publication/PCM/counter purity and wide-sim.
The full log is `build/regions_proofs/native_followup.log`.

The extended sweep retains 12,126 pure captures and 3,476 exact 320x200 frames,
covers the same 3,476 reached camera views, and adds all 44 constructed camera
arrivals. Eight claimed fixed extents remain contained and disjoint from the
other fixed rooms; all 36 original free-arrival apertures have counterexamples.
232 sampled views now carry unproven outer extents, including the demoted 0Dh
content horizon (the initial implementation reported 33). This changes proof
metadata rather than render rectangles or game state. No new free-follow outer
extent is claimed. Each remaining record and its blocker is listed above.

The exact verifier still reports 86,428 program-image bytes and all MZ fields
equal; the existing full-file header differences are relocation-table ordering.
`c/`, ASM, assets, metadata, presenters, renderers and `present_motion.c` were
not edited by this follow-up. `git diff --check` passes.

## Margin objects

`enh_margin_capture` runs once in the gameplay publication path. It reads bounded
source arenas and appends `FrameSprite`s with `margin_only=1`; faithful pixels,
regular sprites, dots, game memory, counters and audio are untouched. It is off
when `pre2_view.variant` is active, including restricted rooms of `--wide-sim`.

The allowlist is derived from Prehistorik 2, independently of Rayman's types:
**sleeping directional terrain sources** among the 16 records at 9107h, stride 15.
`terrain_directional` returns before movement only while +Eh is zero, target speed
+7 is nonnegative and flags C0h are clear. Falling type 8 and every moving/awakened
source are excluded. Terrain moves before projection, so merely being unprojected
is insufficient evidence of immobility. Non-ridden placement uses the original
Y-2 adjustment. This is a state-qualified allowlist, not permission to draw all
platform types at their inactive source positions.

The **70 float/pickup sources are excluded** by the requested unchanged-activation
criterion. DATA 8F1Dh, stride 7: `BuildEffectDrawList` freezes Y/sway while a source
is omitted, but resumes its bounce after admission and before the 20-record
capacity return. The scripted sweep measured 59 nonzero activation Y steps, all
within the original -3..+3 sway range. Frozen inactive positions therefore do not
prove motionless handoffs. Enabling those extras requires an explicit relaxation
of that criterion; no such relaxation is assumed here.

No inactive enemy entity type is admitted: its spawn/script handoff needs a
separate animation/position proof. The port's 64-record enemy ownership walk is
not used; the original has 12 child records.

Actual projected slot ownership (+9) together with the **published** regular draw
list decides whether a source is already drawn. A projected but sprite-culled
source is still a candidate. Terrain source IDs use 80000046h..80000055h and synthetic
slots F046h..F055h; generation is the authored-source level epoch. Sources are
loaded rather than allocated during play; reset restores the same sources and
publishes a camera cut. Capture has no observation-dependent lifetime counters.
Any collision with a regular generation ID suppresses the extra instead of
publishing duplicate IDs.

The renderer draws the appended sources behind regular sprites and preserves
every pixel overlapping the simulation's original 320x176 window, including the
mask blitter's edge carry. Foreground-neighbour requests from extras also stay
outside that window when interpolation supplies position overrides. Trailing
dots retain their original place after
foreground. The remaining pickup pop-ins are intentional under the strict
allowlist. Moving terrain and enemy activation pop-ins also remain.

## Timing and capture

The variant keeps original update/pass/wait counts and input poll order. Its
explicit timing convention is **semantic sprite and tile-summary work has zero
video-bus cost**. Existing fixed-320 raster operations (ring copies, edge loads,
weather, HUD and transitions) still charge their actual original accesses. Sprite
rasterization and overlay/animated-tile rasterization are replaced by semantic
work in the variant, so their old memory-bus cost is absent (remaining preparation port writes are
still charged). Wider native output adds
no bus cost. Consequently variant logical frame timing can differ from faithful
play; it is deterministic for a fixed run configuration and input schedule. The
default keeps the complete original bus accounting. This is not a claim to model
the time a VGA would spend rasterizing a wider image.

Wide gameplay publishes sprites and dots in their original draw order with
stable slot generations. The fixed 320-pixel faithful page compositor is not a
world renderer for this mode; wide frames use the enhanced renderer and the
region table. Screens, tally and exit stages retain their authored 320-pixel
layout.

## Evidence and limits

The static policy evidence comes from decoded level headers and records. The
implementation must be validated with default differential, native and binary
identity suites, then scripted deterministic runs across all 16 levels at 480
and 640 pixels. Those runs should report pool high-water marks and check pool
extents, table reads, stable identities, player state, and scripted exits.
Enhanced-renderer contact sheets belong in `build/wide_shots/` for review.

The audit identifies unresolved polygons for caves and portions of Level 05,
as well as the exact Level 0D reveal silhouette. Until those are annotated,
room policy remains conservative; a wider output does not establish that every
stored map column is authored gameplay space.

## Storage and order

`c/pools.c` owns the role accessors. Compatibility mode returns the original DATA
records, counts, search order and overflow behavior. Wide mode first uses those
records, then stable DATA offsets in B200h..DB75h, reserved before the first heap
allocation. Heap allocation starts beyond DB80h; the gap DB80h..DB8Fh is a test
canary. Growth never moves a record or invalidates owner/back pointers.

| Role | Initial wide capacity | Explicit maximum |
|---|---:|---:|
| Children | 24 | 128 |
| Bursts | 64 | 256 |
| Pickup/source projections | 70 | 70 authored sources |
| Debris sprites | 32 | 128 |
| One-pass points | 40 | 256 |
| Terrain projections | 16 | 16 authored sources |

Allocators double up to the declared maximum. Beyond it they fail safely; point
debris cannot walk into SideTileClassTable. Terrain has no decrement-from-zero
terminator in the variant. Compatibility preserves both original overflows. At the wide debris-sprite
maximum, allocation reports FFFFh and the caller skips the stale-slot lifetime
adjustment; compatibility keeps that original stale-pointer quirk.
Source projections visit every eligible source in original source order, without
the 20-record freeze. Contact order is ascending children, then the ascending
burst/projection union. Sprite drawing descends the same logical role sequence
as the original, with each extension adjacent to its role. The weapon pool stays
at four records; longer visibility can increase projectile residency.

Capture reserves 1024 sprites (the bounded logical sprite maximum is 627), fixed
identity ranges per role, and 1024 dots (the original maximum 511 snow dots plus
256 point records and 20 fireflies is 787; constructed 256-flake states reach 788). IDs use a 2048-slot generation stride
in the variant; default IDs retain their 128-slot stride. Point/firefly positions
are captured before point disposal. Wide dots also carry source generations and
per-piece 64-bit IDs: point reuse bumps its generation, fireflies retain theirs
until a free sentinel, and snow pieces retain their source identity within the
level epoch; weather retains its original 320-wide address
field and RNG rather than creating extra shared-RNG sources. The faithful page
compositor is bypassed for wide world events, while the fixed HUD remains safe.

## Integration limitations

The presentation window and margin-object pass described above replace the old
restricted-aperture centring policy for ordinary world rendering. The opt-in
wide simulation still uses its existing region-policy callback and conservative
cave widths. Other workers own screen/HUD/transition native rendering; this change
adds no faithful-image fallback and does not complete those paths. Free-follow
cave extents and complete earthquake silhouettes remain content-policy limits.

## Validation coverage

`tools/difftest.py`: all 5858 compatibility cases equal. `tools/nativetest.py`:
all original 18 suites pass unchanged; with the five new wide suites, 23/23 pass. `tools/verify.py`: 86428-byte program image
exact, original relocation set; its existing full-file note is five header bytes
from relocation ordering. The native SDL host also builds with repository flags.

The new `wide_sim` runs 16 levels at 480 and 640, twice each, using scripted walk,
jump and attack input. It requires matching captured-state and full-address-space
hashes, checks every displayed gameplay frame for duplicate sprite/dot IDs,
geometry/death state and reserved pool extents, and checks the heap-gap canary.
The observed occupancy high waters were children 10, bursts 1, source projections
20, debris sprites 3, point effects 15, terrain projections 1. Active child sprites
appeared beyond x320 in levels 0..4,6,8 (6626 captured frames). These modest peaks
are coverage facts, not a bound on other gameplay paths.

`wide_pools` forces growth and exhaustion: child slot 25, burst slot 65, debris
slot 33, all 256 point records, all 70 source projections in source order, and
terrain counts exactly 7 and 16. `wide_capture` checks extension identity stability
after earlier-pool growth and point/firefly/snow generations. `wide_geometry`
checks A01 and F01 endpoints, original demo geometry, explicit camera edges,
column-236 cave mapping, and exit contacts in an extension projection over all
16 level remaps plus the warp contact. `wide_regions` checks tower/lower bands,
graphics exclusion, level F and cave apertures.

The contact sheet `build/wide_shots/wide_sim.bmp` is a 4x2 grid. From left to
right, its top row is levels 01h at 480/640 then 02h at 480/640; its bottom row
is 06h at 480/640 then 08h at 480/640. Empty initial CRT pages are excluded, and
frames with active enemies in the wider margins are preferred.

These are bounded scripted runs and constructed contact tests, not complete
route/exit playthroughs or all boss endings. Extent assertions and a canary do
not constitute a general read-access sanitizer: table-read instrumentation remains
an acceptance item. The new loops have explicit source/record bounds, but no
claim is made that every original table lookup was dynamically monitored.

## Regions validation (initial implementation, 2026-11-14)

All 27 existing native suites and the five new region suites passed. The full
`python tools/nativetest.py` invocation reported 31/31 (four new suites existed
when it started); `regions_margin` passed separately. Final affected-suite runs
covered `presenter_motion`, `regions_margin`, `regions_sweep`, and `regions_window`.
The final sweep also asserts that the constructed shipped-terrain contact really
adds pixels. The final window tests also cover original 320 overshoots, strict
staging exclusion at wider widths, horizontal-only locks and reseeding under
colliding player/region identity hashes. `regions_reveal` checks all 17 phases
of the translated routine.

* Region checks: 322,560 map cells, all 44 cave records matched to shipped data,
  503,736 camera/size cases, and 3,476 reachable scripted camera views covered.
  33 views use explicitly unresolved outer extents. These runs are not complete
  cave routes, all boss endings, or proof of every free-follow room polygon.
* Capture checks: 12,126 full-memory/counter/regular-view purity checks and 3,476
  exact 320x200 frames. On/off replay also compares every machine counter, PCM,
  faithful pixels/palettes/transitions and regular semantic views in ordinary,
  warp, game-over and wide-sim schedules.
* Allowlist evidence: 8,372 frozen sleeping-terrain samples and one unchanged
  activation in the level scripts, plus a constructed admission using the actual
  terrain projector. Floats had 152,690 omitted-source samples but 59 Y-changing
  activations; they are rejected. Enemy types remain unqualified.
* `python tools/verify.py`: 86,428 program-image bytes exact, all MZ fields equal;
  the existing five full-file header-byte differences are relocation ordering.
* `python tools/hostbuild.py`: pass. Headless 45-second year-1995 smokes pass with
  exit 3 (the harness time limit) in faithful, enhanced, widescreen 21:9 with all
  smoothing, and widescreen with `--wide-sim 640`. The three presentation-only
  runs retain digest `b32fdf580ab9c3b9`, 3,275 timer ticks, 3,153 publications and
  349,944 PCM bytes. The existing simulation variant has digest
  `469af867632dfdf5`; its capture is disabled.
* `git diff --check`: pass. No changes to ASM, `c/`, assets, host, or other workers'
  source files. No differential rerun was needed because `c/` was not changed.

`build/regions_shots/contact.bmp` is a 4x6 sheet, with `manifest.txt` identifying
all cells. It includes every level, a constructed locked cave, both edges of the
lower tower band, four constructed earthquake phases, and a shipped sleeping
terrain source viewed in the margin. Constructions are labelled as such, rather
than represented as played routes. Full backing-map BMPs are inspection data,
not declarations that every stored cell is playable. `sweep.log` and the four
`host_*.log` files contain current measurements; `rejected_float_trial.log`
records the rejected permissive float experiment.

The pop-in metric counts a source newly entering the regular draw list that was
already visible by its anchor in the previous wider window; it is not a count of
all pixel discontinuities. In particular it does not count unqualified enemies.
The strict allowlist improves one observed level-0Fh terrain handoff at 468/640;
most source pop-ins are pickups and intentionally remain:

| Level | 356 before -> after | 468 before -> after | 640 before -> after |
|---|---:|---:|---:|
| 00h | 0 -> 0 | 2 -> 2 | 2 -> 2 |
| 01h | 3 -> 3 | 7 -> 7 | 7 -> 7 |
| 02h | 3 -> 3 | 4 -> 4 | 4 -> 4 |
| 03h | 4 -> 4 | 4 -> 4 | 4 -> 4 |
| 04h | 0 -> 0 | 0 -> 0 | 0 -> 0 |
| 05h | 0 -> 0 | 0 -> 0 | 0 -> 0 |
| 06h | 4 -> 4 | 5 -> 5 | 5 -> 5 |
| 07h | 0 -> 0 | 1 -> 1 | 1 -> 1 |
| 08h | 0 -> 0 | 0 -> 0 | 0 -> 0 |
| 09h | 0 -> 0 | 0 -> 0 | 0 -> 0 |
| 0ah | 4 -> 4 | 5 -> 5 | 5 -> 5 |
| 0bh | 5 -> 5 | 7 -> 7 | 7 -> 7 |
| 0ch | 0 -> 0 | 1 -> 1 | 1 -> 1 |
| 0dh | 4 -> 4 | 6 -> 6 | 6 -> 6 |
| 0eh | 0 -> 0 | 0 -> 0 | 0 -> 0 |
| 0fh | 0 -> 0 | 2 -> 1 | 2 -> 1 |
