# Prehistorik 2 reconstruction

A readable TASM source of the DOS game Prehistorik 2 (`PRE2.EXE`, Titus 1993, written in
assembly by Eric Zmiro) that assembles to the exact original program, and a C translation
of it that is verified against that source routine by routine, on the way to a native
SDL3 port.

```
PRE2.EXE  <- byte exact ->  src/ (semantic ASM oracle)  <- differential tests ->  c/  ->  host/ (SDL3)
```

```powershell
python tools/verify.py        # exact rebuild: image, header, relocations (~10 s)
python tools/difftest.py      # every differential suite in tests/ (ASM in Unicorn vs C)
python tools/where.py 107B    # source line for a code offset (ds:XXXX for data)
python tools/graph.py 107B    # callers, callees, ports, interrupts, DATA of a routine
python tools/hostbuild.py     # native SDL3 build -> build/host/pre2.exe
build\host\pre2.exe --assets assets             # play
build\host\pre2.exe --assets assets --smoke 45  # bounded integration check (docs/smoke-test.md)
```

Prerequisites: Python 3.10+ with `capstone` and `unicorn` (pip), MSYS2 mingw64 gcc,
the private original assets in `assets/` and the DOS tools in `toolchain/`
(metadata/*.json pins every file by hash).

## Layout

- `src/MODULE1..4.ASM` - the program in link order: four link modules (relocation order
  and alignment evidence, docs/executable.md); MODULE1 also holds the stack and all DATA.
- `include/` - shared constants (record field offsets, sizes) as they are recovered.
- `c/` - the C translation on the original memory model (docs/architecture.md), platform
  models for VGA and DOS files; `c/pre2_sym.h` is generated from the ASM labels.
- `tests/` - differential suites (`tools/difftest.py`).
- `tools/` - build/verify, address lookup, symbol maintenance (`rename.py`, `define.py`,
  `symtab.py`, `apply.py` for worker proposals), static analysis (`explore.py`,
  `segflow.py`, `graph.py`), the bounded execution harness (`emu.py`) and C build.
- `host/` - the native SDL3 platform: interrupts at the game's polling points, VGA
  presentation, Sound Blaster audio, keyboard.
- `docs/` - the oracle boundary (executable.md), architecture, timing model, smoke test,
  worker briefs, next work.
- `metadata/` - pinned inputs, oracle manifest, toolchain and SDL3 locks.

Names, labels and module files are modern reconstructions; original identifiers are
unknown. Original assets and historical tool binaries are private local inputs.
