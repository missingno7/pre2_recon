"""Build the translated C regions (c/*.c) into a test DLL for differential tests.

    python tools/cbuild.py        -> build/c/pre2core.dll

Uses the MSYS2 mingw64 gcc (x64). The same sources are later linked into the native
SDL3 host. Regenerates c/pre2_sym.h first.
"""
from common import *
import cgen, subprocess, shutil, os

GCC = Path(os.environ.get('PRE2_GCC', r'C:\msys64\mingw64\bin\gcc.exe'))

def build(out=ROOT/'build'/'c'/'pre2core.dll', extra=()):
    cgen.generate()
    out.parent.mkdir(parents=True, exist_ok=True)
    skip = {x.strip() for x in os.environ.get('PRE2_C_EXCLUDE', '').split(',') if x.strip()}
    sources = sorted(str(p) for p in (ROOT/'c').glob('*.c') if p.name not in skip)   # skip: files still in progress
    cmd = [str(GCC), '-shared', '-O2', '-std=c11', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
           '-fwrapv', '-o', str(out), *extra, *sources]
    env = dict(os.environ, PATH=str(GCC.parent) + os.pathsep + os.environ.get('PATH', ''))
    p = subprocess.run(cmd, capture_output=True, text=True, env=env)
    if p.returncode: raise RuntimeError(p.stdout + p.stderr)
    return out

if __name__ == '__main__':
    print(build())
