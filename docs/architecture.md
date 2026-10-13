# From the ASM oracle to a native port

```
PRE2.EXE --byte exact (tools/verify.py)--> src/ semantic ASM oracle
src/ --bounded differential tests (tools/difftest.py, Unicorn)--> c/ translated regions
c/ + platform models (DOS model) --> verified portable game core
c/ + host/ SDL3 services (replacing the platform models) --> native application
```

## Translated C (c/)

The C code is portable C11 that operates on the **original address space**: `pre2_mem`
(c/pre2.h) is the 1 MiB real-mode memory with the program image at load segment 1010h,
DATA at 1A0Fh and the code segment at 1030h, exactly the frames the original runs in.
Game state is read and written at its original segment and 16-bit offset; offsets wrap
at 64 KiB. Offsets come from c/pre2_sym.h, generated from the ASM labels
(tools/cgen.py), so a layout is never duplicated by hand. C owns no game state: no
statics, no shadow copies, no converted records. Original integer behaviour (16-bit
wrap, signedness of compares, stale fields, bugs) is kept explicitly.

Why not a 16-bit DOS hybrid (as overkill_recon used): this program is one 40 KiB code
segment of hand-written assembly with a single DATA segment; translated regions can be
compared one routine at a time against the oracle under Unicorn with the same memory
snapshot, and the identical C then links into the native host. The DOS-model hardware
(below) keeps the platform semantics fixed while the language changes.

Boundaries:
- Routines entered from tests keep an adapter `int c_<label>(Regs *r)` that maps the
  original register contract (registers, CF/ZF results) onto a C function with explicit
  parameters. C-to-C calls are direct; adapters are removed when no test or ASM caller
  needs them.
- ES is the one register the original lets flow across routine boundaries and consumes
  stale (the render-slot fill at loc_01C4, the tally clear, the game-over record copy):
  the C carries it as a register model, `pre2_es` (c/pre2.h), which every translated
  routine leaves as the original leaves ES, including its push/pop pairs and the
  interrupt handlers' save/restore. tools/esflow.py --sets lists the ES-changing routines
  statically; the differential tests compare `pre2_es` with the oracle's ES after every
  case. Other registers are explicit parameters; PRE2_POISON=1 difftest runs start every
  unspecified register as garbage on both sides to show no contract is missing.
- Platform services are C functions with narrow contracts that replace one hardware or
  DOS interaction each (dos_files.c: INT 21h file requests). The same contract is
  implemented twice only where it must be: once for the oracle side (tools/emu.py
  device hooks) and once for C; their observable logs are compared.

## Differential tests (tools/difftest.py, tests/)

Each case constructs state statically (from known data, real asset files, edge values),
runs the original routine in tools/emu.py and the C entry in build/c/pre2core.dll from the
same memory snapshot, then compares the DATA segment, code-segment variables, declared
output buffers, contract registers/flags, the ES model, VGA memory and registers, port
and DOS request logs. No case boots the game. Composed cases chain routines the way the
top level does (a frame whose UpdateLevelState requests a restart, LevelRestart, the next
level's first frame or the front end); waits for a button run a scripted press/release on
both sides (Case.c_setup and the pre2_test_poll hook on the C side, code hooks at the
original's matching instructions). PRE2_COVERAGE=1 records the oracle's executed code
for tools/coverage.py. tests/_vga_kat.py checks the shared VGA model itself against
documented VGA behaviour (the differential tests cannot: both sides use it).
Mutation checks (a deliberately wrong C change must fail) confirm that cases observe what
they claim to.

## Platform models and the SDL3 host (host/)

Hardware the game drives directly (VGA planar memory and registers, DAC palette, PIT,
keyboard controller, Sound Blaster DSP + 8237 DMA + PIC, AdLib) is modelled in C at the
level the game's code observes: register writes and memory side effects. Video
presentation scans the modelled VGA state out to RGBA in an SDL3 texture the way the CRT
and attribute controllers do, from register state only (host/present.c); the BIOS mode set
is part of the VGA model (c/vga_bios.c: registers, default DAC, memory clear); audio presents the PCM blocks the game's own mixer produces; timing
derives simulation ticks from the original PIT programming and handler counters, never
from the presentation rate. host/ replaces these models with SDL3 services behind the
same contracts. The host also gives DOS file loads and VGA accesses the time they took on a period
machine (host_io, bus_pay), so interrupts (keyboard releases among them) arrive during
loads as they did and VGA-bound loops keep their pace against the retrace; a CRT timeline
from the CRTC timing registers drives 3DAh, the start address latch and the moment each
frame is scanned out (crt_advance in host/platform_host.c).
