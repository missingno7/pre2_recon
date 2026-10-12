"""Exact acceptance: does the maintained source build to the original program?

1. Original assets and local tools match their pinned hashes; PRE2.SQZ expands to
   PRE2.EXE (the shipped container of the program).
2. A fresh extraction of PRE2.EXE matches metadata/oracle.json.
3. The linked program image equals the original byte for byte; every MZ header field
   (entry, stack, sizes, min/max allocation) and the TLINK signature match.
4. The set of relocations TLINK emits equals the original's. Their order is reported
   as evidence (it depends on assembler record emission), as is full-file identity.
Source text is otherwise free: names, comments, labels and file layout may change.
"""
from common import *
from image import mz, ASSET
from extract import extract, unpack_sqz_exe
from build import assemble, main_sources
import re

def check_hashes(manifest):
    for row in read_json(ROOT/'metadata'/manifest)['files']:
        data = (ROOT/row['path']).read_bytes()
        if len(data) != row['size'] or sha(data) != row['sha256']: raise ValueError('Hash mismatch: ' + row['path'])

MARK = '; UNKNOWN: not yet classified as code or data'

def count_rows(line):
    """Bytes in one `db` row (plain items and `N dup(x)`)."""
    body = line.split(';', 1)[0].strip()[3:]
    n = 0
    for item in body.split(','):
        m = re.match(r'\s*(\w+)\s+dup\s*\(', item, re.I)
        n += int(m.group(1).rstrip('hH'), 16 if m.group(1)[-1] in 'hH' else 10) if m else 1
    return n

def check_sources():
    """Original bytes may not enter through the assembler. Returns (UNKNOWN bytes,
    explicitly encoded instruction rows)."""
    unknown = explicit = 0
    for path in main_sources() + list((ROOT/'include').glob('*.INC')):
        text = path.read_text(encoding='latin-1'); low = text.lower()
        for bad in ('incbin', 'include assets', 'include ../', 'include ..\\'):
            if bad in low: raise ValueError(f'Forbidden directive {bad!r} in {path.name}')
        in_unknown = False
        for n, line in enumerate(text.splitlines(), 1):
            if line.startswith('; UNKNOWN'):
                in_unknown = True; after = n; continue
            if in_unknown and line.startswith('    db '):
                unknown += count_rows(line); continue
            if in_unknown and n == after + 1: raise ValueError(f'{path.name}:{after}: UNKNOWN marker must head plain db rows')
            in_unknown = False
            if re.match(r'^\s+db .*; ENCODED', line): explicit += 1
    return unknown, explicit

def relocation_lcs(a, b):
    lcs = [[0]*(len(b) + 1) for _ in range(len(a) + 1)]
    for i in range(len(a) - 1, -1, -1):
        for j in range(len(b) - 1, -1, -1):
            lcs[i][j] = lcs[i+1][j+1] + 1 if a[i] == b[j] else max(lcs[i+1][j], lcs[i][j+1])
    return lcs[0][0]

def first_differences(expected, actual, limit=5):
    out = [f'length {len(actual)} != {len(expected)}'] if len(actual) != len(expected) else []
    i = 0
    while i < min(len(expected), len(actual)) and len(out) < limit:
        if expected[i] != actual[i]:
            j = i
            while j < min(len(expected), len(actual)) and expected[j] != actual[j]: j += 1
            out.append(f'{i:05X}..{j:05X}: expected {expected[i:j][:8].hex()} got {actual[i:j][:8].hex()}'); i = j
        else: i += 1
    return out

def verify():
    check_hashes('inputs.json'); check_hashes('toolchain-lock.json')
    original = ASSET.read_bytes()
    if unpack_sqz_exe((ROOT/'assets'/'PRE2.SQZ').read_bytes()) != original: raise ValueError('PRE2.SQZ does not expand to PRE2.EXE')
    manifest = extract()
    if manifest != read_json(ROOT/'metadata'/'oracle.json'): raise ValueError('Fresh extraction differs from metadata/oracle.json')
    unknown, explicit = check_sources()
    import symtab
    stale = [q.name for q, ls in symtab.interface(main_sources()).items()
             if q.read_text(encoding='latin-1').splitlines() != ls]
    if stale: raise ValueError('stale module interface (run tools/symtab.py): ' + ', '.join(stale))
    exe = assemble()
    oh, oimg, orel, _ = mz(original)
    bh, bimg, brel, _ = mz(exe)
    diff = first_differences(oimg, bimg)
    if diff: raise AssertionError('Program image mismatch:\n  ' + '\n  '.join(diff))
    fields = [k for k in oh if k != 'checksum' and oh[k] != bh[k]]
    if fields: raise AssertionError('MZ header differs: ' + ', '.join(f'{k} {bh[k]:X} != {oh[k]:X}' for k in fields))
    if exe[0x1C:0x22] != original[0x1C:0x22]: raise AssertionError('TLINK signature differs')
    o = [s*16 + f for s, f in orel]; b = [s*16 + f for s, f in brel]
    extra = sorted(set(b) - set(o)); missing = sorted(set(o) - set(b))
    if extra or missing:
        raise AssertionError('Relocations: extra ' + ' '.join(f'{x:05X}' for x in extra) + '; missing ' + ' '.join(f'{x:05X}' for x in missing))
    print(f'PASS program image: {len(bimg)} bytes exact, entry {bh["cs"]:04X}:{bh["ip"]:04X}, all MZ header fields equal')
    print(f'PASS relocations: {len(b)} linked, the original set; {relocation_lcs(o, b)}/{len(o)} in original order (evidence only)')
    same = exe == original
    print(f'{"PASS" if same else "INFO"} full file: {"identical to PRE2.EXE" if same else f"{sum(x != y for x, y in zip(exe, original))} header bytes differ (relocation table order)"}')
    print(f'INFO source: {unknown} bytes still UNKNOWN db; {explicit} instructions explicitly encoded')

if __name__ == '__main__':
    verify()
