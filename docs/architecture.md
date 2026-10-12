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
- Platform services are C functions with narrow contracts that replace one hardware or
  DOS interaction each (dos_files.c: INT 21h file requests). The same contract is
  implemented twice only where it must be: once for the oracle side (tools/emu.py
  device hooks) and once for C; their observable logs are compared.

## Differential tests (tools/difftest.py, tests/)

Each case constructs state statically (from known data, real asset files, edge values),
runs the original routine in tools/emu.py and the C entry in build/c/pre2core.dll from the
same memory snapshot, then compares the DATA segment, code-segment variables, declared
output buffers, contract registers/flags and DOS request logs. No case boots the game.
Mutation checks (a deliberately wrong C change must fail) confirm that cases observe what
they claim to.

## Platform models and the SDL3 host (host/)

Hardware the game drives directly (VGA planar memory and registers, DAC palette, PIT,
keyboard controller, Sound Blaster DSP + 8237 DMA + PIC, AdLib) is modelled in C at the
level the game's code observes: register writes and memory side effects. Video
presentation converts the modelled VGA state (planes, start address, DAC) to RGBA in an
SDL3 texture; audio presents the PCM blocks the game's own mixer produces; timing
derives simulation ticks from the original PIT programming and handler counters, never
from the presentation rate. host/ replaces these models with SDL3 services behind the
same contracts.
