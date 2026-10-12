"""Apply a worker proposal file to src/, then refresh interfaces. Run verify.py after.

    python tools/apply.py PROPOSAL.txt [--dry]

Proposal format (one directive per line; blank lines and lines starting with # ignored):

    rename OLD NEW                    rename a symbol everywhere (tools/rename.py rules)
    comment LABEL                     replace the comment block directly above LABEL's line
    ; text...                         (the block: following lines starting with ';')
    end
    note LABEL                        append ';' comment lines directly after LABEL's line
    ; text...
    end
    define ADDR NAME [byte|word|dword]   name data at an address (tools/define.py)
    class ADDR LEN text...            classify LEN bytes at ADDR as `; DATA: text`

Labels are matched case-insensitively against `NAME:` / `NAME label` / `NAME db|dw|dd`
definitions. Directives apply in order: renames first (all at once), then the rest, so
comment/note may use either the old or the new name.
"""
from common import *
from build import main_sources
import re, sys, subprocess

def parse(text):
    ops, lines, i = [], text.splitlines(), 0
    while i < len(lines):
        l = lines[i].rstrip(); i += 1
        if not l.strip() or l.startswith('#'): continue
        w = l.split()
        if w[0] in ('comment', 'note'):
            block = []
            while i < len(lines) and lines[i].strip() != 'end':
                b = lines[i].rstrip(); i += 1
                if not b.startswith(';'): raise SystemExit(f'{w[0]} {w[1]}: block lines must start with ";": {b!r}')
                block.append(b)
            i += 1
            ops.append((w[0], w[1], block))
        elif w[0] == 'rename' and len(w) == 3: ops.append(('rename', w[1], w[2]))
        elif w[0] == 'define' and len(w) in (3, 4): ops.append(('define', w[1], w[2], w[3] if len(w) == 4 else 'byte'))
        elif w[0] == 'class' and len(w) >= 4: ops.append(('class', w[1], int(w[2], 0), ' '.join(w[3:])))
        else: raise SystemExit(f'bad directive: {l!r}')
    return ops

def find_label(name):
    pat = re.compile(r'^' + re.escape(name) + r'(?::|\s+label\s|\s+(?:db|dw|dd)\s)', re.I)
    for p in main_sources():
        lines = p.read_text(encoding='latin-1').splitlines()
        for n, l in enumerate(lines):
            if pat.match(l): return p, lines, n
    raise SystemExit(f'label {name} not found')

PLACEHOLDER = re.compile(r'^(sub|loc|unused|cvar|tbl|byte|word|dword|dtbl)_([0-9A-F]{4})$', re.I)

def by_address(name):
    """A placeholder label renamed by an earlier proposal: find today's label at its address."""
    m = PLACEHOLDER.match(name)
    if not m: raise SystemExit(f'label {name} not found')
    kind, off = m.group(1).lower(), int(m.group(2), 16)
    lin = off + (0x9FF0 if kind in ('byte', 'word', 'dword', 'dtbl') else 0x200)
    import where
    hits = [n for n, l in where.labels().items() if l == lin]
    if not hits: raise SystemExit(f'label {name} not found (no label at its address)')
    return hits[0]

def main():
    a = sys.argv[1:]
    ops = parse(Path(a[0]).read_text(encoding='latin-1'))
    dry = '--dry' in a
    renames = [(o[1], o[2]) for o in ops if o[0] == 'rename']
    alias = {new.upper(): old for old, new in renames} | {old.upper(): new for old, new in renames}
    if dry:
        for o in ops: print(o[:3])
        return
    if renames:
        import rename, symtab
        every = set()
        for p in main_sources():
            for line in p.read_text(encoding='latin-1').splitlines(): every |= {x.upper() for x in symtab.IDENT.findall(symtab.code_text(line))}
        skipped = [(o, n) for o, n in renames if o.upper() not in every and n.upper() in every]
        for o, n in skipped: print(f'skip rename {o} -> {n}: already applied')
        taken = [(o, n) for o, n in renames if o.upper() not in every and n.upper() not in every]
        for o, n in taken: print(f'CONFLICT rename {o} -> {n}: {o} was already renamed by another proposal')
        clash = [(o, n) for o, n in renames if o.upper() in every and n.upper() in every and o.upper() != n.upper()]
        for o, n in clash: print(f'CONFLICT rename {o} -> {n}: {n} already names another label')
        renames = [r for r in renames if r not in skipped and r not in taken and r not in clash]
    if renames:
        rename.rename_all(renames)
    for o in ops:
        if o[0] in ('comment', 'note'):
            name = o[1]
            try: p, lines, n = find_label(name)
            except SystemExit:
                try: p, lines, n = find_label(alias.get(name.upper(), name))
                except SystemExit:
                    p, lines, n = find_label(by_address(name))
            if o[0] == 'comment':
                k = n
                while k > 0 and lines[k - 1].startswith(';') and not lines[k - 1].startswith(('; UNKNOWN', '; DATA:', '; module interface', '; end module')): k -= 1
                lines[k:n] = o[2]
            else:
                lines[n + 1:n + 1] = o[2]
            p.write_text('\n'.join(lines) + '\n', encoding='latin-1')
        elif o[0] == 'define':
            subprocess.run([sys.executable, str(ROOT/'tools'/'define.py'), o[1], o[2], o[3]], check=True)
        elif o[0] == 'class':
            subprocess.run([sys.executable, str(ROOT/'tools'/'define.py'), o[1], '--class', o[3], '--len', str(o[2])], check=True)
    import symtab
    for p, ls in symtab.interface(main_sources()).items(): p.write_text('\n'.join(ls) + '\n', encoding='latin-1')
    print(f'applied {len(ops)} directives; run tools/verify.py')

if __name__ == '__main__':
    main()
