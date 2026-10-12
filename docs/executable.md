# The executable and the oracle boundary

Numbers are hexadecimal. `segment:offset` pairs are relative to the DOS load segment, as
in the MZ header. "Code offsets" are relative to frame 0020 (the `cs:` offsets in the
source); "ds offsets" are relative to frame 09FF (DS for all game code).

## Files

`metadata/inputs.json` pins the original files. The game ships its program as
`PRE2.SQZ`, an LZW container (header: size bits 16..23, 10h, size bits 0..15; 9..12-bit
MSB-first codes, 101h = clear, 100h = end). `PRE2.EXE` is its expansion;
`tools/extract.py:unpack_sqz_exe` reproduces it byte for byte and `tools/verify.py` checks
that on every run. Which loader expands `PRE2.SQZ` in the shipped game is outside this
reconstruction.

`PRE2.EXE` is a plain MZ executable: 87452 bytes, 1024-byte header, 134 relocations,
no packer, no overlay, no appended data. The header bytes 1C..21 are `01 00 FB 20 72 6A`,
the signature TLINK 2.0 writes (`FB`, version 20h, `rj`), and the header is rounded to
512 bytes as TLINK 2.0 does. The rebuilt EXE reproduces every header field.

| Field | Value |
|---|---|
| load module | 86428 bytes (= 151A paragraphs - 4 bytes) |
| entry | 0020:0008 |
| stack | 0000:0200 (the first 200h bytes of the image, zero) |
| min_extra / max_extra | 2 / FFFF paragraphs |

## Address frames

| Frame | Linear | Content |
|---|---|---|
| 0000 | 00000..001FF | stack segment `STK` (zero bytes) |
| 0020 | 00200..09FFB | the only code segment `CODE` (9DFC bytes) |
| 09FF | 09FFC..151AB (+10h) | `DATA`, word aligned right after CODE: its first byte is ds:000C |

`DATA` ends at ds:B1BC including 16 uninitialized bytes beyond the file (MZ min_extra 2;
the entry code computes the first free paragraph from offset B1BCh and `seg DATA + 1`,
which is the one relocation with value 0A00). Labels in DATA are segment relative in the
source; TLINK adds the 0Ch frame offset, so `mov ax, word_2875` assembles to `A1 75 28`.

All 134 relocation sites hold either 09FF (127, `seg DATA`), 0A00 (1, `seg DATA + 1`) or
0020 (6: two far code pointers at cs:027A/027E and four in DATA at ds:0E4D..). Code also
reads DATA through CS: `cs:X` with X >= 9DFC addresses ds:(X - 9DF0), e.g. the timer
handler's `cmp cs:byte_2877, 0`. TASM resolves `cs:DataLabel` against the CS frame, so the
source writes these symbolically.

## Link modules

TLINK emits relocations per object module, in module order. The original table runs
`0027F..022D0, 0AE41..0AE55` (module 1: code then its data part) and then `025D5..09D79`
(the rest of the code, ascending except one rotation, below). So at least two modules,
the second starting at or after code offset 20D0. The single 00h byte at 23D3 is the word
alignment TLINK inserts before a word-aligned module part, which puts the start of module
2 at 23D4 (its first instruction is the first relocation of the second run). The same
lone 00h pad occurs at 454D and 951F; TASM's `even` pads with 90h, so these are taken as
two more module starts (MODULE3 at 454E, MODULE4 at 9520). DATA is held by MODULE1; any
split of DATA after ds:0E57 links identically and is not determined.

Relocation order: the rebuild matches the original order except that the site at 2E5E
(`mov dx, seg DATA`, linear 0305F) precedes 2DDB and 2E03 in the original. TASM 1.0 emits
fixups per LEDATA record; the rotation is not reproduced by a forward/backward reference
change of `seg DATA` in module 2 (tested) and probably reflects a different segment-block
layout of the lost source. Acceptance is the relocation *set*; the order is reported.
Consequently the rebuilt PRE2.EXE differs from the original in 5 header bytes only.

## Toolchain

TASM 1.0 + TLINK 2.0 (toolchain/README.md). TASM 1.0 is single pass: a forward `jmp`
becomes `EB xx 90` unless written `short`, and `jmp near ptr` to an in-range backward
label still assembles short, so such encodings are written explicitly (`; ENCODED`).
`mov reg, DATA` with DATA not yet declared gets a NOP; the source uses `seg DATA`.
The program is hand-written assembly (its own credits: "100% MACHINE LANGUAGE by ERIC
ZMIRO 1992"), consistent with the Borland tools; the original assembler is not proven.

## What the reconstruction covers

The program image immediately before 0020:0008 executes, its entry point, header and
relocations. The game loads no executable code at run time; it overwrites some of its
own code (self-erasing protection check at 19C7, the blitter area 26FA.. on the fatal VGA
path, the far call operand at 1AFB), which the source documents at the sites.
