"""Rename a symbol everywhere in src/ and include/, then refresh module interfaces.

    python tools/rename.py OLD NEW [OLD2 NEW2 ...]

Matches whole identifiers outside comments and string literals, case-insensitively
(TASM symbols are case-insensitive). Refuses a NEW name that already exists.
Run tools/verify.py afterwards.
"""
from common import *
from build import main_sources
import re, sys, symtab

def split_code(line):
    """(code part, comment part) honouring quotes."""
    q = None
    for i, c in enumerate(line):
        if q:
            if c == q: q = None
        elif c in "'\"": q = c
        elif c == ';': return line[:i], line[i:]
    return line, ''

def rename_all(pairs):
    paths = main_sources() + sorted((ROOT/'include').glob('*.INC'))
    texts = {p: p.read_text(encoding='latin-1') for p in paths}
    every = set()
    for t in texts.values():      # identifiers in code only (comments may mention future names)
        for line in t.splitlines(): every |= {x.upper() for x in symtab.IDENT.findall(symtab.code_text(line))}
    for old, new in pairs:
        if not re.fullmatch(r'[A-Za-z_@?][\w@?$]*', new): raise SystemExit(f'bad name {new}')
        if new.upper() in every and new.upper() != old.upper(): raise SystemExit(f'{new} already exists')
        if old.upper() not in every: raise SystemExit(f'{old} not found')
    table = {old.upper(): new for old, new in pairs}
    pat = re.compile(r'(?<![\w@?$])([A-Za-z_@?][\w@?$]*)')
    for p, t in texts.items():
        out = []
        for line in t.split('\n'):
            code, comment = split_code(line)
            parts = re.split(r"('[^']*')", code)      # keep string literals
            code = ''.join(s if s.startswith("'") else pat.sub(lambda m: table.get(m.group(1).upper(), m.group(1)), s) for s in parts)
            out.append(code + comment)
        new = '\n'.join(out)
        if new != t: p.write_text(new, encoding='latin-1')
    for p, lines in symtab.interface(main_sources()).items():
        p.write_text('\n'.join(lines) + '\n', encoding='latin-1')

if __name__ == '__main__':
    a = sys.argv[1:]
    if not a or len(a) % 2: raise SystemExit(__doc__)
    rename_all(list(zip(a[::2], a[1::2])))
    print('renamed', len(a)//2)
