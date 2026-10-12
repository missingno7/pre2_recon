# Translation worker brief (ASM oracle -> C, differential gate)

You translate one bounded region of the exact ASM reconstruction (D:\Prog\pre2_recon\src)
into C under c/, and prove equality with a differential test suite under tests/. Read
AGENTS.md, docs/architecture.md and c/sqz.c + tests/sqz.py (the reference example) first.
Never run the game. Do not edit src/, include/, tools/ or another region's C files.

## Rules for the C

- Portable C11 on the original memory model (c/pre2.h): all game state lives in
  `pre2_mem` at its original segment:offset. Use the generated names from c/pre2_sym.h
  (`DS_<Label>` = ds offset of a DATA label, `CS_<Label>` = code offset of a code-segment
  label; run `python tools/cgen.py` to regenerate, names keep the ASM case). Access with
  DB/DW/SET_DB/SET_DW (DATA), CB/CW/SET_CB/SET_CW (code segment), rd8/rd16/wr8/wr16
  (any segment). Never hard-code an offset that has a label; add no C statics that hold
  game state; no shadow copies of records.
- Keep original integer behaviour: 16-bit wraps (cast to uint16_t/uint8_t), signed vs
  unsigned compares exactly as the ASM branch (jl/jg signed, jb/ja unsigned), 8-bit
  carries, offset wrap within segments (`(uint16_t)(off + n)`), stale or stray writes,
  out-of-table reads. Preserve bugs. Comment each such quirk briefly.
- Structure: one C function per original routine (named after its ASM label), with
  explicit parameters/returns for its register contract; direct C-to-C calls. For each
  routine the tests enter, export an adapter `EXPORT int c_<Label>(Regs *r)` that maps
  registers (and CF/ZF in r->flags) to and from the function; return 0 on success.
- Hardware: VGA through c/vga.h (vga_out/vga_in/vga_write/vga_read), DOS files through
  c/dos_files.h. Other hardware (PIT, PIC, SB DSP/DMA, keyboard, AdLib) is not modelled
  yet: if your region needs it, stop at that boundary and report.
- Build: `python tools/cbuild.py` (gcc -Wall -Wextra -Werror). Keep it warning free.

## Rules for the tests

- tests/<suite>.py defines CASES = [Case(...)] (tools/difftest.py): `setup(m)` writes the
  initial state through the harness (m.ww/m.wb/m.write; m.code('Label') / m.data('Label')
  give offsets of labels), `regs` the input registers, `out_regs` the contract registers
  and flags ('CF', 'ZF') to compare, `ranges` extra memory to compare (DATA and the code
  segment are always compared). Run `python tools/difftest.py <suite>`.
- Construct state from the contract: realistic values (from the shipped data files via
  the SQZ loader if needed), boundary values (0, max, sign boundary, wrap), every branch
  you can identify statically, and controlled random cases (fixed seeds) for arithmetic.
  A few strong cases beat many weak ones.
- Mutation check: deliberately break 3 different things in your C (one at a time), confirm
  the suite fails, restore. Report which mutants were caught.
- Every unexplained difference must be resolved, never masked. If the oracle stops
  (interrupt, budget), your setup is incomplete: fix the state, do not drop the case.

## Report (final message, at most ~50 lines)

Files written, routines translated, cases and what they cover, mutants caught, quirks
preserved, and any boundary you stopped at (hardware or region edge).
