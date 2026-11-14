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

The backing level map is 256 columns of storage, not a 4096-pixel playable
world. Continuous regions retain the level's authored right-camera limit. When
the aperture grows, the limit moves left by the added tile columns so the
playable world-right edge stays fixed. This is conservative because the level
headers and cave records do not describe complete safe polygons for every
disconnected room.

Known restrictions keep an effective 320-pixel aperture, centred by the host
inside the wider output:

* Level 05 tower and lower encounter remain authored as 320-pixel bands. The
  tower uses columns 0..19; the lower encounter retains its moving aperture
  starting at camera column 35 and its authored right limit 46.
* Level 09 keeps columns 0..19 as world and columns 20..49 as graphics storage.
* Level 0E keeps its 20-column horizontal band while preserving vertical travel.
* A cave with a nonzero follow gate is locked to a 320-pixel aperture. An
  unannotated arrival also holds a 320-pixel aperture around its mapped target
  until its room bounds are established.

These choices follow the decoded headers, cave records, and map facts in
`build/audit/view_geometry.md` (sections 3 and 4). They do not claim the
remaining cave and room shapes have all been proven safe. In particular, Level
0D's staged reveal requires phase-aware masks and is not covered by a global
vertical clamp. Its 149-pixel port workaround is not an original camera limit.

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

The presenter worker owns host renderer selection, effective-aperture centering,
and the rest of `native/enh_render.c`. This work only changes its level-policy
function. Its existing foreground exclusion and tile-paint domain still assume
320 pixels. Main gameplay capture publishes the full simulation width, but the
presenter must consume the region's effective aperture when choosing its output
camera and clip. Dynamic cave/level05 lower-region render masks need that shared
policy integration; the simulation table alone does not change the renderer's
per-frame clipping. Level0D phase silhouettes remain unannotated.

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
