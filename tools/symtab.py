"""Recompute every module's public/extrn declarations from the labels in src/.

    python tools/symtab.py          rewrite the interface blocks in place
    python tools/symtab.py --check  fail if they are stale

Each module has two generated blocks between the marker comments below: one at the top
(publics, and data externs inside an empty DATA segment block, modules 2.. only, since
MODULE1 must declare STK, CODE, DATA first to fix the segment order) and one after the
CODE segment's assume line (code externs, which need the CODE segment context).
Run it after adding, removing, renaming or moving a label used across modules.
"""
from common import *
from build import main_sources
import re, sys

BEGIN, END = '; module interface (tools/symtab.py)', '; end module interface'
IDENT = re.compile(r'[A-Za-z_@$?][\w@$?]*')
DECL = re.compile(r'^([A-Za-z_@$?][\w@$?]*)(?::|\s+(label)\s+(\w+)|\s+(db|dw|dd)\b|\s+proc\b)', re.I)
KIND = {'db': 'byte', 'dw': 'word', 'dd': 'dword'}

def strip_blocks(lines):
    out, skip = [], False
    for line in lines:
        if line == BEGIN: skip = True; continue
        if line == END: skip = False; continue
        if not skip: out.append(line)
    return out

def code_text(line):
    """Line without comment and string literals (identifiers only from real operands)."""
    line = re.sub(r"'[^']*'", "''", line)
    return line.split(';', 1)[0]

def scan(lines):
    """(definitions {NAME: (name, segment, kind)}, used identifiers {NAME})."""
    defs, used, seg = {}, set(), None
    for line in lines:
        text = code_text(line)
        m = re.match(r'^\s*(\w+)\s+segment\b', text, re.I)
        if m: seg = m.group(1).upper(); continue
        if re.match(r'^\s*\w+\s+ends\b', text, re.I): seg = None; continue
        if re.match(r'^\s*(public|extrn|assume|end)\b', text, re.I): continue
        d = DECL.match(text)
        if d and seg:
            name = d.group(1)
            kind = d.group(3) if d.group(2) else KIND.get((d.group(4) or '').lower(), 'near')
            if seg != 'CODE' and kind == 'near': kind = 'byte'
            defs[name.upper()] = (name, seg, kind.lower())
            text = text[d.end():]
        used |= {t.upper() for t in IDENT.findall(text)}
    return defs, used

def interface(paths):
    files = {p: strip_blocks(p.read_text(encoding='latin-1').splitlines()) for p in paths}
    scans = {p: scan(lines) for p, lines in files.items()}
    owner = {}
    for p, (defs, _) in scans.items():
        for k, v in defs.items():
            if k in owner: raise ValueError(f'{v[0]} defined in {owner[k][0].name} and {p.name}')
            owner[k] = (p, v)
    out = {}
    for k, p in enumerate(paths):
        lines = files[p]; defs, used = scans[p]
        exported = sorted(v[0] for key, (q, v) in owner.items() if q == p and
                          any(key in scans[r][1] for r in paths if r != p))
        imported = sorted((owner[key][1] for key in used if key in owner and owner[key][0] != p and key not in defs),
                          key=lambda v: v[0].upper())
        top = [BEGIN] + [f'public {x}' for x in exported]
        data_ext = [v for v in imported if v[1] != 'CODE']
        code_ext = [v for v in imported if v[1] == 'CODE']
        if data_ext:
            if k == 0: raise ValueError('MODULE1 cannot import data (segment order)')
            top += ["DATA segment word public 'DATA'"] + [f'extrn {n}:{kind}' for n, s, kind in data_ext] + ['DATA ends']
        top.append(END)
        code = [BEGIN] + [f'extrn {n}:{kind}' for n, s, kind in code_ext] + [END]
        # top block after the leading comment lines (and after STK in the first module)
        i = 0
        while i < len(lines) and lines[i].startswith(';'): i += 1
        if k == 0:
            while i < len(lines) and not re.match(r'^STK\s+ends', lines[i], re.I): i += 1
            i += 1
        new = lines[:i] + top + lines[i:]
        j = next(n for n, l in enumerate(new) if re.match(r'^\s*assume\s+cs:CODE', l, re.I))
        new = new[:j + 1] + code + new[j + 1:]
        out[p] = new
    return out

def main():
    check = '--check' in sys.argv
    stale = []
    for p, lines in interface(main_sources()).items():
        text = '\n'.join(lines) + '\n'
        if p.read_text(encoding='latin-1') != text:
            stale.append(p.name)
            if not check: p.write_text(text, encoding='latin-1')
    if check and stale: raise SystemExit('stale module interface: ' + ', '.join(stale) + ' (run tools/symtab.py)')
    print(('stale: ' if check else 'updated: ') + (', '.join(stale) or 'none'))

if __name__ == '__main__':
    main()
