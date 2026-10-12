"""TASM 1.0 listing reader shared by gen.py and where.py.

TASM numbers listing rows, not source lines: byte continuations and wrapped long source
lines get rows of their own. Rows are therefore matched to the source by their text
column, joining wrapped rows until they spell the source line.
"""
import re

def parse(text, src_lines, name='?'):
    """Return [offset or None] per source line (index 0 = line 1)."""
    rows, col = [], None
    first = src_lines[0].strip()
    for line in text.splitlines():
        if not re.match(r'^\s*\d+ ', line): continue
        if col is None: col = line.index(first)
        if len(line) <= col: continue
        off = re.match(r'^\s*\d+\s+([0-9A-F]{4})\s', line)
        rows.append((int(off.group(1), 16) if off else None, line[col:]))
    out, r = [], 0
    for n, want in enumerate(src_lines, 1):
        want = want.strip()
        if not want: out.append(None); continue      # blank lines get no listing row
        if r >= len(rows): raise RuntimeError(f'{name}: listing ends before source line {n}')
        off, txt = rows[r]; r += 1
        txt = txt.strip()
        while txt != want and want.startswith(txt) and r < len(rows):
            txt += ' ' + rows[r][1].strip(); r += 1
            if want.replace(' ', '') == txt.replace(' ', ''): txt = want
        if txt != want: raise RuntimeError(f'{name} line {n}: listing {txt!r} != source {want!r}')
        out.append(off)
    if r != len(rows): raise RuntimeError(f'{name}: {len(rows) - r} listing rows left over')
    return out
