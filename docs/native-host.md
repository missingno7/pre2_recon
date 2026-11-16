# Native host settings

Build with `python tools/hostbuild.py`. The SDL3 host runs the translated
program on a simulation worker; rendering and the settings UI consume published
frames. `--assets assets --smoke 45 --headless --year 1995` remains the bounded
integration run (exit **3** means it reached its logical limit).

The product is `build/host/pre2_sdl3.exe` plus `SDL3.dll`, a window-subsystem
program: copied into a game folder it starts with no arguments. Without
`--assets DIR` the game data is the first of the executable's directory, its
`assets/`, the working directory and `./assets` that holds `SPRITES.SQZ`; if
none does, a message box says so. Started from a console it still prints its
reports there.

Preferences live in `pre2_settings.json` under
`SDL_GetPrefPath("pre2-reconstruction", "Prehistorik2")`. On Windows this is
normally `%APPDATA%\pre2-reconstruction\Prehistorik2\pre2_settings.json`.
The host prints the resolved filename at startup. `--settings PATH` chooses
another file; its parent directory must already exist. Preferences never go
into the game's assets or original configuration.
If SDL cannot create/access its preferences directory, the host warns and
continues with run-only defaults; `--settings` can select a writable location.

Precedence is **defaults < JSON < command line**. Loading a missing file uses
defaults. A malformed document or unsupported version uses defaults with a
warning; a valid document with an invalid known value uses the default for
that key while keeping valid siblings. Unknown keys are ignored, including
compound values. They are not retained on save. Files are limited to 64 KiB
and nesting to 32 levels. Version 1 is written; a partial document may omit
`version`. JSON types are checked (for example, `mute` requires a Boolean).

The overlay saves on every change using a same-directory `.tmp` file followed
by atomic replacement (`MoveFileEx` on Windows, `rename` elsewhere). Failed
writes report a warning and leave the previous file intact. Command-line
overrides remain run-only: only the particular field changed in the overlay
is copied into the saved record. There is no automatic save at exit.

| JSON key | Values | Default |
|---|---|---|
| `version` | `1` | `1` |
| `mode` | `"faithful"`, `"enhanced"`, `"widescreen"` | `"faithful"` |
| `aspect` | `"16:9"`, `"16:10"`, `"21:9"`, `"window"` | `"16:9"` |
| `interpolate` | Boolean | `false` |
| `smooth_camera` | Boolean; presenter implies interpolation | `false` |
| `camera_tau` | seconds, greater than zero through 10 | `0.12` |
| `smooth_transitions` | Boolean | `false` |
| `hud` | `"anchored"`, `"left"`, `"centre"`, `"right"` | `"anchored"` |
| `backdrop` | `"black"`, `"mirror"`, `"stretch"` | `"black"` |
| `wide_sim` | `"off"`, `"auto"`, or integer 320..640, multiple of 16; `0` also means off | `"off"` |
| `fullscreen` | Boolean, borderless desktop fullscreen | `false` |
| `window_scale` | integer 1..6 (window size 320x200 times scale) | `3` |
| `vsync` | Boolean | `true` |
| `frame_cap` | integer 0..1000, 0 unlimited; menu offers 0/30/60/120/144/240 | `0` |
| `integer_scaling` | Boolean; integral enlargement where it fits | `false` |
| `pixel_aspect` | `"square"` retains the host's original 320:200 (16:10) shape; `"crt"` stretches height by 6/5 for 4:3 | `"square"` |
| `volume` | integer percent 0..100 | `100` |
| `mute` | Boolean | `false` |
| `performance` | Boolean | `false` |

CLI names are the keys with underscores changed to hyphens, for example
`--window-scale 4 --vsync off --hud anchored --pixel-aspect crt`.
Booleans accept `true`/`false`, `on`/`off`, or `1`/`0`; without a value they
enable the option, preserving the existing `--interpolate`, `--smooth-camera`
and `--smooth-transitions` flags. Other existing diagnostic/run flags remain
available (`--assets`, `--smoke`, `--scenario`, `--shots`, `--year`, `--turbo`,
`--headless`, `--reference`, `--fast-load`, `--fast-vga`, `--demo-in/out`).
Those flags describe a test/run rather than user preferences and are not saved.
`--reference` always uses default presenter options, including after a menu edit.
Input rebinding, mouse and gamepad input are not implemented by the host.

F10 opens/closes the modal menu (F1 is left to the game). Escape closes; Up/Down selects an
editable row, skipping info rows; Left/Right changes it; Enter advances its
value; Tab/Shift+Tab changes tabs. Tabs are Display (including performance),
Enhancements, Audio, and Experimental. A compact authored 3x5 bitmap font is
rasterized at an integral scale into the actual drawable XRGB output. The
minimum window is 320x200. The frozen game picture is dimmed behind the panel;
resizing scales that picture without taking a new simulation frame.

Presentation edits rebuild the presenter and seed it with the last 32 retained
publications, and preview the same frozen logical target behind the menu.
Window, vsync, scaling and gain changes apply immediately. Gain/mute operate on SDL's output stream; the
machine still generates exactly the same mono PCM. A frame cap limits presents,
independently of the simulation; vsync can impose a lower effective cap.

The game image is always scaled by the GPU. The menu and the performance line
are drawn at the drawable's resolution into an overlay texture the renderer
blends on top (the earlier CPU composition of the whole window cost 4 ms at
1080p and 15 ms at 4K per frame whenever the performance line was on). The
image is sampled at the display's own time on the logical clock, not at the
bound the simulation worker last finished. The performance line adds DRAW
(composing the image) and SHOW (upload, swap and the wait for sync) in ms.

The audio output stage (`sdl_host_audio`) does not change the machine's PCM. It
centres it (a 7 Hz DC blocker to signed 16-bit: the original rests at 00h, so
every gap SDL fills with silence was a step), keeps a 60 ms cushion ahead of
the device (refilled after an underrun), follows the queue depth with a rate
ratio within 0.5 %, and drops a queue beyond 250 ms. The clicks inside the
original mix (unramped notes and effects, unfiltered 8403 Hz stepping) remain;
removing those needs the separate high-quality engine.
The performance display reports presented FPS, delivered original PIT timer
ticks per wall second (TPS; zero while paused), and logical time minus the current
wall target with pauses removed, in milliseconds. It is not a fixed-rate
game-update counter.

Experimental true widescreen is labelled **changes gameplay**. Editing it
only changes the preference for the **next host boot**; restart the host to
apply it. It never calls the machine's width setter mid-level or mid-session.
At boot it implies widescreen presentation. Auto derives a width from the run's
aspect (window aspect uses 16:9 at boot), rounded to 16 and clamped 320..640.
Demos/attract continue to use the machine's original geometry. See
[native-widescreen.md](native-widescreen.md) for the existing variant's scope.

The main thread requests pause under the bridge lock and waits for worker
acknowledgement at its next stepping boundary. A previously admitted slice can
finish; the worker then makes **no** machine calls until resume (except quit
when the host is closing). Audio-device playback pauses after acknowledgement,
retaining its queued samples. The UI freezes the last presented image. Menu
input is consumed before PC scan-code generation, including repeats and releases;
no mouse events are sent to the game. A game key released while paused gets its
deferred break on resume; UI-only keys never acquire game makes or breaks.
`HostClock` freezes the last admitted logical bound, discards paused wall time,
and resumes from that bound without a catch-up jump. It is serialized by the
same bridge lock as worker admission. Rendering never reads a running machine.

Verification:

* `python tools/nativetest.py settings_json overlay_model overlay_pause` checks
  JSON round trips, strict malformed/partial/type fallback, unknown fields,
  warnings, saved-versus-run precedence and replacement; every menu item changing
  exactly one preference; navigation and deterministic clipped rasterization.
* `overlay_pause` runs the translated program with virtual wall time in ordinary,
  warp and attract scripts for 45 logical seconds. Two 5-second modal pauses at
  logical targets 5 and 38 seconds exercise the same host clock, menu navigation
  and drawing. Time and counters remain unchanged throughout the pause. The
  no-pause and paused runs must have identical memory samples, final memory,
  every machine counter, PCM contents/timestamps/rates, and frame publication
  contents (the compositor's process-global serial is compared relative to each
  run's first publication). This is headless pause-mapping coverage, not a live
  SDL device test.
* `overlay_model` writes review BMPs to `build/overlay_shots/` at 320x200,
  640x400, 960x600, 1280x720, 1920x1080 and a deliberately clipped 257x151.
