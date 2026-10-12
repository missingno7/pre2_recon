# Local tools

- TASM.EXE: Turbo Assembler 1.0 (C:/tools/tasm-1.00/files/disk04), run under nmlgcdos/msdos.exe.
- TLINK.EXE: Turbo Link 2.0 from the same disk, run under msdos-i86/msdos.exe. The original
  PRE2.EXE header carries TLINK 2.0's signature (01 00 FB 20 'rj' at 1Ch) and its 512-byte
  header rounding, so the linker is very likely the original one; the assembler is not proven.
- ndisasm.exe (+ msys-2.0.dll, NASM-LICENSE.txt): optional NASM-syntax disassembly.

Runners and tools were copied from overkill_recon/toolchain, where the same binaries are
pinned; all hashes are in metadata/toolchain-lock.json and checked by tools/verify.py.
These are local task dependencies, not claims about the original development tools.
Do not redistribute the historical binaries.
