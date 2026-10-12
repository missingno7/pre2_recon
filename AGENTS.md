# Reconstruction rules

## Goal

A byte-exact, readable TASM source of Prehistorik 2's program (PRE2.EXE) that is the
permanent executable specification of the original, and from which a C translation and
then a native SDL3 port are derived with as little guessing as possible:

```
PRE2.EXE  <- byte exact (tools/verify.py) ->  semantic ASM oracle  <- bounded differential tests ->  C  ->  SDL3 host
```

Priorities: 1. exact binary match (absolute); 2. semantic clarity for translation;
3. separation of game logic and platform code where the program really has it;
4. resemblance to a historical source (secondary).

## Execution policy

Static analysis is the default. Never boot or play the original game to observe it.
Run original code only to settle a specific question static work cannot, and then only
the smallest routine or region, entered at a known boundary with constructed state.

## The loop

1. Choose the next target by asking: what unresolved part would force a future C
   translator to guess? (docs/next-work.md)
2. Investigate one region: `python tools/where.py ADDR --disasm N` (source line and
   original bytes), callers/callees (grep labels), DS state (`tools/segflow.py`).
3. Improve the source, in order of value: classify UNKNOWN bytes; global state, records,
   field offsets, tables, dispatch, buffers, state machines; routine contracts (inputs,
   outputs, registers/flags callers consume, side effects, shared tails, nonlocal exits,
   interrupt state); platform vs game classification; local labels last.
4. `python tools/verify.py` (about 10 s). If it fails, fix or revert before continuing.

## Fixed constraints

- Never modify assets/ or metadata/inputs.json; never modify neighbouring projects.
- No INCBIN or asset includes. `; UNKNOWN` heads plain db rows of unclassified bytes
  only; `; DATA: class` heads classified data. Proven code is written as instructions.
  `; ENCODED:` marks an instruction TASM cannot spell with the original encoding.
- Names are modern, evidence-supported reconstructions. Placeholder names
  (`sub_XXXX`, `loc_XXXX`, `unused_XXXX`, `byte_/word_/dword_XXXX` = ds offset,
  `cvar_XXXX` = code-segment variable, `tbl_/dtbl_XXXX` = code pointer tables) carry the
  address; replace them with `python tools/rename.py OLD NEW` (updates every module and
  the public/extrn blocks). `python tools/define.py ADDR NAME [kind]` names data at an
  address; `--class` classifies a run. Module interfaces are generated (tools/symtab.py).
- pre2_port (legacy Python port) is a source of hypotheses only; confirm against the
  original instructions before naming or documenting a contract.
- Preserve original quirks and bugs; document them at their sites (words: quirk, stale,
  unsigned, wraps, equality, reads past, wrong segment).
- Comments explain purpose, non-obvious intent, hardware/DOS behaviour, unusual control
  flow, state meaning and real uncertainty. No per-instruction restatement or addresses.
- Every tool must have a current consumer. One source of truth: the source; derive the
  rest (listings, grep, segflow) on demand.
- Workers investigate bounded regions with disjoint ownership; the supervisor integrates
  and runs the exact verifier after each batch.
