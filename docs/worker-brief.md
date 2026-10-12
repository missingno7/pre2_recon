# Region worker brief (semantic ASM reconstruction)

You analyse one bounded region of the exact TASM reconstruction of Prehistorik 2
(D:\Prog\pre2_recon) and write a proposal file. You do NOT edit src/, include/, tools/
or docs/; the supervisor applies proposals. Never run the game. Read AGENTS.md first.

## Conventions

- Code offsets (cs:XXXX, frame 0020) are the numbers in `sub_XXXX`/`loc_XXXX` labels.
  DATA labels `byte_/word_/dword_XXXX` carry their ds offset (DS = frame 09FF, the data
  segment for all game code). `cvar_XXXX` = variable in the code segment at cs:XXXX.
  `cs:X` with X >= 9DFC addresses ds:(X - 9DF0) (e.g. cs:byte_2877 is ds:2877).
- `python tools/where.py ADDR --disasm N` maps an address (code offset, ds:XXXX) to the
  source line and disassembles the original. `python tools/graph.py ADDR` lists a
  routine's callers, callees, ports, interrupts and DATA offsets read/written;
  `python tools/graph.py --tree ADDR 2` a call tree. The source files are
  src/MODULE1.ASM (code 0000..23D3 + all DATA), MODULE2 (23D4..454D), MODULE3
  (454E..951F), MODULE4 (9520..9DFB). Search them with grep for labels.
- DS value at an instruction (static dataflow): in Python from D:\Prog\pre2_recon\tools,
  `from segflow import analyse; img, insns, st = analyse(); st[0x1234].get('ds')`
  ('DATA', 'ZERO', 'CODE' or None = unknown). Throwaway scripts go in your scratchpad.
- pre2_port (D:\Games\DOS\dos_recosystem\legacy\pre2_port, a Python port) may give name
  hypotheses: same code offsets (its 1030:XXXX); its data segment 1A13:XXXX may be
  shifted relative to ds offsets (verify!). D:\Prog\pre2_recon\build\research\pre2_port_symbols.md
  summarises it if present. Never adopt a pre2_port claim the instructions do not prove.

## What to produce

For every routine entry in your region (sub_XXXX / unused_XXXX; table entries too):
1. A conservative purpose name (CamelCase verb phrase: `LoadSqzFile`, `WaitVerticalRetrace`,
   `UpdatePlayerJump`). If the purpose is not clear from the instructions, keep the
   placeholder and say why in the report. Hardware/DOS services get purpose-level names
   (SetVideoMode, ReadKeyboardPort) that do not hide side effects that matter.
2. A contract comment block above the label, only what a C translator must preserve:
   ```
   ; Purpose in one or two lines.
   ; In: registers/flags/state the routine consumes (with units/ranges if known).
   ; Out: results (registers, flags such as CF), state written.
   ; Clobbers/preserves: only when unusual or relied on by callers.
   ; Notes: hardware/DOS effects, shared tails, nonlocal exits, interrupt context,
   ;        original quirks (16-bit wraps, signed/unsigned compares, stale fields,
   ;        equality-only limits, wrong-segment accesses, reads past tables).
   ```
   No per-instruction narration. Keep lines under 100 characters.
3. Names for DATA and cvar labels whose meaning your region's code proves (CamelCase
   nouns: `NextFreeSegment`, `SoundBlasterPort`). Data used by several regions: name it
   only if the meaning is clear from your region; the supervisor merges conflicts.
4. Optional: `class` directives for data runs whose kind is proven (tables, buffers,
   strings) and important local labels (loop/exit names) via rename.
5. Translation-critical uncertainties: anything a C translator would have to guess.

## Proposal file format (tools/apply.py)

Write build/proposals/<REGION>.txt (create the directory):
```
rename sub_107B LoadSqzFile
rename word_2875 NextFreeSegment
comment sub_107B
; Opens, reads and decompresses the SQZ resource file named at DS:DX ...
; In: DX = ds offset of the ASCIIZ file name.
; Out: AX = segment of the decompressed data; NextFreeSegment advanced.
end
note loc_1234
; quirk: ...
end
class ds:2F34 256 bit-reversal table (byte i = reverse8(i))
```
`comment LABEL` replaces the comment block above LABEL; use the placeholder label name
(renames are applied first, either name works). Names must be unique in the whole source
(check with grep -i) and valid TASM identifiers (letters, digits, _; not a register or
instruction name; at most 31 characters).

## Report (your final message, at most ~60 lines)

Routines covered, the important state/structures found, platform interactions, original
quirks, translation-critical uncertainties, and anything outside your region you noticed.
