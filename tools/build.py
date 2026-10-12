"""Assemble src/ with TASM 1.0 and link with TLINK 2.0 (both under local MS-DOS Players).

Every file in src/sources.txt is assembled, then TLINK links the objects in that order
into build/asm/PRE2.EXE. Listings (.LST) and the map land in build/asm for address
lookup (tools/where.py). No bytes from the original assets enter the build.
"""
from common import *
from dos import dos
from image import mz
import re, shutil

def main_sources():
    lines = (ROOT/'src'/'sources.txt').read_text().splitlines()
    return [ROOT/'src'/l.strip() for l in lines if l.strip() and not l.startswith('#')]

class BuildLock:
    """Cross-process lock for a build directory (several tools and workers build at once)."""
    def __init__(self, build): self.path = build.parent/(build.name + '.lock')
    def __enter__(self):
        import os, time
        self.path.parent.mkdir(parents=True, exist_ok=True)
        for _ in range(3000):
            try:
                os.close(os.open(self.path, os.O_CREAT | os.O_EXCL)); return self
            except FileExistsError:
                try:
                    if time.time() - self.path.stat().st_mtime > 600: self.path.unlink()   # stale
                except OSError: pass
                time.sleep(0.1)
        raise RuntimeError(f'build lock {self.path} held too long')
    def __exit__(self, *exc):
        try: self.path.unlink()
        except OSError: pass

def assemble_and_link(paths, build, exe):
    """Assemble `paths` in `build`, link them into `exe`; return the linked EXE bytes."""
    with BuildLock(build):
        return _assemble_and_link(paths, build, exe)

def _assemble_and_link(paths, build, exe):
    build.mkdir(parents=True, exist_ok=True)
    for inc in (ROOT/'include').glob('*.INC'): shutil.copyfile(inc, build/inc.name)
    for path in paths:
        shutil.copyfile(path, build/path.name)
        obj = build/(path.stem + '.OBJ'); obj.unlink(missing_ok=True)
        log = dos('TASM.EXE', [f'{path.name},{path.stem}.OBJ,{path.stem}.LST'], build)
        (build/(path.stem + '.log')).write_text(log)
        if not obj.exists(): raise ValueError(f'TASM failed for {path.name}; see {build/(path.stem + ".log")}')
    stem = exe[:-4]; (build/exe).unlink(missing_ok=True)
    (build/'LINK.RSP').write_text('+\n'.join(p.stem + '.OBJ' for p in paths) + f'\n{exe}\n{stem}.MAP\n')
    log = dos('TLINK.EXE', ['/s', '@LINK.RSP'], build)
    (build/(stem + '.log')).write_text(log)
    if re.search(r'^(Error|Fatal)', log, re.M) or not (build/exe).exists(): raise ValueError('TLINK failed:\n' + log)
    return (build/exe).read_bytes()

def assemble():
    exe = assemble_and_link(main_sources(), ROOT/'build'/'asm', 'PRE2.EXE')
    header, image, relocations, _ = mz(exe)
    (ROOT/'build'/'program.bin').write_bytes(image)
    return exe

if __name__ == '__main__':
    print('Linked', len(assemble()), 'bytes')
