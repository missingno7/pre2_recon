"""Original-routine coverage: translation and differential-test execution, per routine.

    PRE2_COVERAGE=1 python tools/difftest.py      record the oracle's executed blocks
    python tools/coverage.py [--all]              report (build/coverage.md; stdout summary)

Routines are the callable entries code discovery finds (call targets, code-pointer table
entries, vectors and other seeds; tools/explore.py). A routine's body is what its entry
reaches without following calls (jumps, fall-through, shared tails: a tail can belong to
several routines). For each routine:
  - C: the C function of the same name in c/*.c ('-' when none: the routine is translated
    inside another function, replaced by a platform service, or not translated; the
    exceptions table below names the known cases),
  - executed: the share of its instructions the oracle ran in the differential tests,
  - missing: the first unexecuted instruction ranges.
"""
from common import *
from image import original
from explore import explore, all_seeds, near_target, table_entries
import json, re, sys

COND = ('j', 'loop')

# Routines without a same-named C function: (status, where/why). Statuses:
#   INLINE        translated inside the named C function (tested through it)
#   PLATFORM      replaced by a platform service or configuration of the native machine
#   ALTERNATIVE   an alternative hardware path the native configuration does not take
#   ERROR_PATH    DOS error handling the native services never trigger
#   DEAD          no caller (tools/hints.py DEAD_CODE evidence)
EXCEPTIONS = {
    'LoadResourceFile': ('INLINE', 'startup.c InitResourceMemory (/D raw file load)'),
    'ReadHuffmanSymbol': ('INLINE', 'sqz.c decode_huffman'),
    'ResetLzwDictionary': ('INLINE', 'sqz.c LZW decoder'),
    'ReadLzwCode': ('INLINE', 'sqz.c LZW decoder (refill)'),
    'LzssNextBitWord': ('INLINE', 'sqz.c LZSS bit reader (lz_refill), file mode'),
    'unused_16F9': ('INLINE', 'second entry of LzssNextBitWord, sqz.c'),
    'IgnoreEffect': ('INLINE', 'sound.c effect dispatch default'),
    'DecodeLzssStream': ('ALTERNATIVE', 'memory mode: resources held in conventional copies, EMS or XMS (system.c PageInResource modes 1-3)'),
    'LoadResourceToEms': ('ALTERNATIVE', 'EMS resource placement (startup.c: STARTUP_UNSUPPORTED when EMS is present)'),
    'LoadResourceToXms': ('ALTERNATIVE', 'XMS resource placement (startup.c: STARTUP_UNSUPPORTED when XMS is present)'),
    'CpuProbeInt06Handler': ('PLATFORM', 'CPU class probe: pre2_platform_cpu (startup.c ProbeCpuClass)'),
    'OplWriteRegister': ('PLATFORM', 'AdLib probe (startup.c: probes are platform configuration)'),
    'DetectAdLib': ('PLATFORM', 'AdLib probe result: pre2_platform_sound'),
    'DetectSoundBlaster': ('PLATFORM', 'probe steps 1-3 are pre2_platform_sound; steps 4-8 translated in startup.c'),
    'WaitCrtRetrace': ('PLATFORM', 'part of the SB IRQ probe'),
    'ToggleIrqProbeHooks': ('PLATFORM', 'SB IRQ probe hooks'),
    'CountSbIrq2': ('PLATFORM', 'SB IRQ probe handler'),
    'CountSbIrq3': ('PLATFORM', 'SB IRQ probe handler'),
    'CountSbIrq5': ('PLATFORM', 'SB IRQ probe handler'),
    'CountSbIrq7': ('PLATFORM', 'SB IRQ probe handler'),
    'CriticalErrorHandler': ('ERROR_PATH', 'INT 24h: DOS critical (disk) errors; the native file service reports none'),
    'WaitRetryPrompt': ('ERROR_PATH', 'the swap-disk / retry prompt of the error handlers'),
    'UnusedProtectionCheck': ('DEAD', 'no caller'),
    'UnusedPrintHexWord': ('DEAD', 'no caller'),
    'UnusedWipeProtectionCode': ('DEAD', 'no caller'),
    'UnusedProtectionDiskIo': ('DEAD', 'no caller'),
    'UnusedProtectionDiskProbe': ('DEAD', 'no caller'),
}

def status(name, c, n, ran):
    if name in EXCEPTIONS: return EXCEPTIONS[name][0]
    if c == '-': return 'UNRESOLVED'
    return 'TESTED' if ran == n else ('PARTIAL' if ran else 'UNTESTED')

def labels():
    out = {}
    for m in re.finditer(r'#define CS_(\w+) 0x([0-9A-F]{4})u', (ROOT/'c'/'pre2_sym.h').read_text()):
        out.setdefault(int(m.group(2), 16), m.group(1))
    return out

def c_functions():
    names = set()
    for p in (ROOT/'c').glob('*.c'):
        for m in re.finditer(r'^[A-Za-z_][\w \*]*?\b(\w+)\(([^;{()]*)\)\s*\{', p.read_text(encoding='utf-8'), re.M):
            names.add(m.group(1))
    return names

def body(insns, entry):
    seen, work = set(), [entry]
    while work:
        off = work.pop()
        while off in insns and off not in seen:
            seen.add(off); ins = insns[off]; m = ins.mnemonic
            if m in ('ret', 'retf', 'iret', 'hlt'): break
            if m == 'jmp':
                t = near_target(ins)
                if t is not None: work.append(t)
                break
            if m.startswith(COND):
                t = near_target(ins)
                if t is not None: work.append(t)
            off += ins.size
    return seen

def main():
    h, img, rel = original()
    insns, xrefs, indirect, calls = explore(img, all_seeds(img))
    from hints import CODE_SEEDS, CODE_TABLES, DEAD_CODE
    entries = set(calls) | set(CODE_SEEDS) | set(DEAD_CODE)
    for seg, off, n, _ in CODE_TABLES: entries |= set(table_entries(img, seg, off, n))
    names, cfun = labels(), c_functions()
    cov = json.loads((ROOT/'build'/'coverage.json').read_text()) if (ROOT/'build'/'coverage.json').exists() else None
    executed = set()
    if cov:
        for a, n in cov['blocks']:
            o = a
            while o < a + n and o in insns: executed.add(o); o += insns[o].size
    rows = []
    for e in sorted(entries):
        b = body(insns, e)
        if not b: continue
        name = names.get(e, f'cs_{e:04X}')
        run = len(b & executed)
        miss, start = [], None
        for o in sorted(b):
            if o not in executed:
                if start is None: start = o
                last = o
            elif start is not None: miss.append((start, last)); start = None
        if start is not None: miss.append((start, last))
        c = name if name in cfun else '-'
        rows.append((e, name, c, len(b), run, miss))
    total = sum(r[3] for r in rows); ran = sum(r[4] for r in rows)
    translated = sum(1 for r in rows if r[2] != '-')
    full = sum(1 for r in rows if r[4] == r[3]); none = sum(1 for r in rows if r[4] == 0)
    counts = {}
    for e, name, c, n, r, miss in rows: counts[status(name, c, n, r)] = counts.get(status(name, c, n, r), 0) + 1
    lines = ['# Original routine coverage (generated by tools/coverage.py)', '',
             f'{len(rows)} routines; {translated} with a same-named C function; oracle execution in the '
             f'differential tests ({", ".join(cov["suites"]) if cov else "no coverage data"}): '
             f'{ran}/{total} routine instructions ({100*ran/max(total,1):.1f}%), {full} routines fully, {none} not at all.', '',
             'Status counts: ' + ', '.join(f'{k} {v}' for k, v in sorted(counts.items())), '',
             '| cs | routine | status | C | instructions | executed | not executed (first ranges) |', '|---|---|---|---|---|---|---|']
    for e, name, c, n, r, miss in rows:
        note = EXCEPTIONS.get(name, ('', ''))[1]
        ms = ' '.join(f'{a:04X}' if a == b else f'{a:04X}-{b:04X}' for a, b in miss[:4]) + (' ...' if len(miss) > 4 else '')
        lines.append(f'| {e:04X} | {name} | {status(name, c, n, r)} | {c if c != "-" else note} | {n} | {100*r/n:.0f}% | {ms} |')
    (ROOT/'build'/'coverage.md').write_text('\n'.join(lines) + '\n')
    print(lines[2])
    if '--all' in sys.argv:
        for l in lines[4:]: print(l)
    print('report: build/coverage.md')

if __name__ == '__main__':
    main()
