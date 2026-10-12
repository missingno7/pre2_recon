"""Name data at an address, or re-express data rows, in the maintained source.

    python tools/define.py ADDR NAME [byte|word|dword]   insert `NAME label KIND` at ADDR
    python tools/define.py ADDR --words N                 rewrite N bytes at ADDR as dw rows
    python tools/define.py ADDR --class "TEXT" [--len N]  replace the `; UNKNOWN` marker of the
                                                          run at ADDR by `; DATA: TEXT`

ADDR as in tools/where.py (code offset, ds:XXXX, lin:XXXXX). The row holding ADDR is
found from a fresh build's listing and split when ADDR falls inside it. Only `db` rows
of plain items, string literals and `N dup(x)` can be split. Rebuild + verify after.
With --len N, a `; DATA:` class line is also inserted N bytes later (splitting there),
so a classification can cover part of a run and leave the rest UNKNOWN.
"""
from common import *
import re, sys
import where, build

def items(body):
    """Split a db operand list into items, honouring quotes."""
    out, cur, q = [], '', False
    for c in body:
        if c == "'": q = not q
        if c == ',' and not q: out.append(cur.strip()); cur = ''
        else: cur += c
    if cur.strip(): out.append(cur.strip())
    return out

def item_bytes(it):
    m = re.match(r'^(\w+)\s+dup\s*\((.*)\)$', it, re.I)
    if m:
        n = m.group(1); return int(n[:-1], 16) if n[-1] in 'hH' else int(n)
    if it.startswith("'"): return len(it) - 2
    return 1

def split_db(line, k):
    """Split db row `line` before byte k: return (left row or None, right row)."""
    m = re.match(r'^(\s*)db\s+(.*?)(\s*;.*)?$', line)
    if not m: raise SystemExit('not a db row: ' + line)
    ind, body, comment = m.group(1), m.group(2), m.group(3) or ''
    left, right, pos = [], [], 0
    for it in items(body):
        n = item_bytes(it)
        if pos + n <= k: left.append(it)
        elif pos >= k: right.append(it)
        else:      # split inside the item
            cut = k - pos
            d = re.match(r'^(\w+)\s+dup\s*\((.*)\)$', it, re.I)
            if d: left.append(f'{cut} dup({d.group(2)})'); right.append(f'{n - cut} dup({d.group(2)})')
            elif it.startswith("'"): left.append(it[:1 + cut] + "'"); right.append("'" + it[1 + cut:])
            else: raise SystemExit('cannot split ' + it)
        pos += n
    mk = lambda xs: f'{ind}db ' + ', '.join(xs)
    return (mk(left) + comment if left else None), mk(right)

def locate(lin):
    build.assemble()
    rows = where.rows()
    best = [r for r in rows if r[0] <= lin]
    if not best: raise SystemExit('address before the first row')
    start, mod, n, text = best[-1]
    return start, ROOT/'src'/mod, n

def split_at(lin):
    """Make ADDR start a row; return (path, 0-based index of that row, lines)."""
    start, path, n = locate(lin)
    lines = path.read_text(encoding='latin-1').splitlines()
    i = n - 1
    if start == lin:
        while i > 0 and re.match(r'^\w+(:| label )', lines[i - 1]) is None and lines[i - 1].startswith(';'): i -= 1
        return path, i, lines
    left, right = split_db(lines[i], lin - start)
    lines[i:i + 1] = ([left] if left else []) + [right]
    return path, i + 1, lines

def main():
    a = sys.argv[1:]
    if len(a) < 2: raise SystemExit(__doc__)
    lin = where.parse(a[0])
    if a[1] == '--words':
        n = int(a[2])
        path, i, lines = split_at(lin)
        path.write_text('\n'.join(lines) + '\n', encoding='latin-1')
        path, j, lines = split_at(lin + n)
        from image import original
        img = original()[1]
        words = [img[lin + 2*k] | img[lin + 2*k + 1] << 8 for k in range(n//2)]
        # rows i..j-1 hold exactly the bytes; they must be plain db rows
        body = lines[i:j]
        if any(not re.match(r'^\s*db\s', l) for l in body if not l.startswith(';')): raise SystemExit('range holds labels or non-db rows')
        rows = ['    dw ' + ', '.join(f'{w:04X}h' if f'{w:X}'[0] not in 'ABCDEF' else f'0{w:04X}h' for w in words[k:k + 8]) for k in range(0, len(words), 8)]
        lines[i:j] = rows
    elif a[1] == '--class':
        text = a[2]
        if '--len' in a:
            n = int(a[a.index('--len') + 1])
            path, j, lines = split_at(lin + n)
            # the bytes after the class stay unclassified; a marker may only head db rows
            if re.match(r'^\s*db\s', lines[j]):
                lines.insert(j, '; UNKNOWN: not yet classified as code or data')
            path.write_text('\n'.join(lines) + '\n', encoding='latin-1')
        path, i, lines = split_at(lin)
        k = i
        while k < len(lines) and lines[k].startswith(';') and not lines[k].startswith('; UNKNOWN'): k += 1
        if k < len(lines) and lines[k].startswith('; UNKNOWN'): lines[k] = f'; DATA: {text}'
        elif i > 0 and lines[i - 1].startswith('; UNKNOWN'): lines[i - 1] = f'; DATA: {text}'
        else: lines.insert(i, f'; DATA: {text}')
    else:
        name = a[1]; kind = a[2] if len(a) > 2 else 'byte'
        path, i, lines = split_at(lin)
        # a split db run below an UNKNOWN marker keeps its marker
        if i > 0 and re.match(r'^\s*db ', lines[i]) and not lines[i - 1].startswith(';') and not re.match(r'^\w+ label', lines[i - 1]):
            run = [l for l in lines[:i] if l.startswith('; ')][-1:]
            marker = run[0] if run and run[0].startswith(('; UNKNOWN', '; DATA:')) else None
            lines[i:i] = [f'{name} label {kind}'] + ([marker] if marker else [])
        else:
            lines[i:i] = [f'{name} label {kind}']
    path.write_text('\n'.join(lines) + '\n', encoding='latin-1')
    import symtab
    for p, ls in symtab.interface(build.main_sources()).items(): p.write_text('\n'.join(ls) + '\n', encoding='latin-1')
    print('ok; run tools/verify.py')

if __name__ == '__main__':
    main()
