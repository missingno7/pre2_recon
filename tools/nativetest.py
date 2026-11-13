"""Native tests: C programs that check the native path against the reference path.

    python tools/nativetest.py            build and run every tests/native/*.c
    python tools/nativetest.py NAME ...   only tests/native/NAME.c

Each tests/native/NAME.c is one program with its own main(), linked with the translated game
(c/*.c), the native runtime (native/*.c) and the embedded program image (as the host has it),
without SDL. It is run from the repository root as `NAME.exe --assets assets` and passes when
it exits with status 0. What these tests compare (docs/native.md, "Verification"): native
assets against what the game's own loaders leave in memory, native scenes rendered against the
reference VGA scan-out at the same publication point, native PCM against the game's mixer,
and simulation state with drawing on and off. The ASM oracle stays with tools/difftest.py.
"""
from common import *
import cbuild, hostbuild, subprocess, sys, os, time

def compile_test(src, out):
    imgc = ROOT/'build'/'host'/'image_data.c'
    if not imgc.exists():                  # tools/hostbuild.py refreshes it with every host build
        imgc.parent.mkdir(parents=True, exist_ok=True)
        hostbuild.embed(imgc)
    sources = [str(p) for p in sorted((ROOT/'c').glob('*.c'))] + \
              [str(p) for p in sorted((ROOT/'native').glob('*.c'))] + [str(src), str(imgc)]
    cmd = [str(cbuild.GCC), '-O2', '-g', '-std=c11', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
           '-fwrapv', f'-I{ROOT}', '-o', str(out), *sources]
    env = dict(os.environ, PATH=str(cbuild.GCC.parent) + os.pathsep + os.environ.get('PATH', ''))
    p = subprocess.run(cmd, capture_output=True, text=True, env=env)
    if p.returncode: raise SystemExit(f'build {src.name}:\n' + p.stdout + p.stderr)

def main():
    import cgen
    cgen.generate()
    names = sys.argv[1:]
    srcs = sorted((ROOT/'tests'/'native').glob('*.c'))
    if names: srcs = [s for s in srcs if s.stem in names]
    out = ROOT/'build'/'native'; out.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, PATH=str(cbuild.GCC.parent) + os.pathsep + os.environ.get('PATH', ''))
    failed = []
    for s in srcs:
        exe = out/(s.stem + '.exe')
        try: compile_test(s, exe)
        except SystemExit as e:
            lines = str(e).splitlines()[:6]
            print(f'{s.stem}: FAIL (build)')
            for line in lines: print('   ' + line)
            failed.append(s.stem)
            continue
        t = time.time()
        p = subprocess.run([str(exe), '--assets', str(ROOT/'assets')], cwd=ROOT, capture_output=True, text=True, env=env)
        tail = (p.stdout + p.stderr).strip().splitlines()[-12:]
        print(f'{s.stem}: {"PASS" if p.returncode == 0 else "FAIL (%d)" % p.returncode} ({time.time() - t:.1f} s)')
        for line in tail: print('   ' + line)
        if p.returncode: failed.append(s.stem)
    print(f'{len(srcs) - len(failed)}/{len(srcs)} native tests pass' + (f'; failed: {", ".join(failed)}' if failed else ''))
    sys.exit(1 if failed else 0)

if __name__ == '__main__':
    main()
