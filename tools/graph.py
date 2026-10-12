"""Routine inventory and call graph from the original code (static).

    python tools/graph.py                 one row per routine entry
    python tools/graph.py ADDR            details for the routine at a code offset
    python tools/graph.py --tree ADDR [D] call tree from ADDR to depth D (default 3)

A routine is a call/table/vector entry; its body is what is reachable from it without
following calls (shared tails appear in several routines). Columns: entry, size of the
body, callers, callees, I/O ports, software interrupts, DATA offsets read/written, and the
source label (from the last build). Investigation aid for choosing regions, not a claim
about original procedure boundaries.
"""
from common import *
from image import *
from explore import explore, all_seeds, near_target, X86, relocated
import re, sys

def routines():
    h, img, rel = original()
    insns, xrefs, indirect, calls = explore(img, all_seeds(img))
    from hints import CODE_SEEDS, DEAD_CODE, CODE_TABLES
    from segflow import table_call_sites, summaries, segflow
    sites = table_call_sites(img, insns, CODE_TABLES)
    entries = set(calls) | set(CODE_SEEDS) | set(DEAD_CODE) | {t for ts in sites.values() for t in ts}
    summaries(img, insns, sorted(calls))
    states = segflow(img, insns, list(CODE_SEEDS) + list(DEAD_CODE), sites)
    callers = {}
    for off, ins in insns.items():
        if ins.mnemonic == 'call':
            t = near_target(ins)
            if t is not None: callers.setdefault(t, set()).add(off)
    for s, ts in sites.items():
        for t in ts: callers.setdefault(t, set()).add(s)
    out = {}
    for e in sorted(entries):
        body, work = set(), [e]
        while work:
            o = work.pop()
            if o in body or o not in insns: continue
            if o != e and o in entries and o not in body and insns.get(o - 1) is None and False: continue
            body.add(o)
            ins = insns[o]; m = ins.mnemonic
            if m in ('ret', 'retf', 'iret', 'hlt'): continue
            if m == 'jmp' or m == 'ljmp':
                t = near_target(ins) if m == 'jmp' else None
                if t is not None: work.append(t)
                continue
            if m.startswith('j') or m.startswith('loop'):
                t = near_target(ins)
                if t is not None: work.append(t)
            work.append(o + ins.size)
        callees, ports, ints, reads, writes = set(), set(), set(), set(), set()
        for o in body:
            ins = insns[o]; m = ins.mnemonic
            if m == 'call':
                t = near_target(ins)
                callees.add(t if t is not None else 'indirect')
                for t2 in sites.get(o, ()): callees.add(t2)
            if m in ('in', 'out', 'insb', 'outsb', 'insw', 'outsw'):
                imm = [x.imm for x in ins.operands if x.type == X86.X86_OP_IMM]
                ports.add(f'{imm[0]:02X}' if imm else 'dx')
            if m == 'int': ints.add(f'{ins.operands[0].imm:02X}')
            st = states.get(o)
            for k, op in enumerate(ins.operands):
                if op.type != X86.X86_OP_MEM or op.mem.base == X86.X86_REG_BP: continue
                seg = ins.reg_name(op.mem.segment) if op.mem.segment else 'ds'
                if seg in ('ds', 'es') and st is not None and st.get(seg) == 'DATA' and (op.mem.base == 0 and op.mem.index == 0 or ins.disp_size == 2):
                    d = op.mem.disp & 0xFFFF
                    (writes if k == 0 and m not in ('cmp', 'test', 'push') else reads).add(d)
        out[e] = dict(body=body, size=sum(insns[o].size for o in body), callers=callers.get(e, set()),
                      callees=callees, ports=ports, ints=ints, reads=reads, writes=writes)
    return insns, out

def label_names():
    names = {}
    for p in (ROOT/'build'/'asm').glob('*.LST'):
        pass
    for p in sorted((ROOT/'src').glob('*.ASM')):
        for line in p.read_text(encoding='latin-1').splitlines():
            m = re.match(r'^([A-Za-z_]\w*):', line)
            if m: names.setdefault(m.group(1), p.name)
    return names

def fmt(xs, n=6):
    xs = sorted(xs, key=lambda x: (isinstance(x, str), x))
    s = ' '.join(x if isinstance(x, str) else f'{x:04X}' for x in xs[:n])
    return s + (' ...' if len(xs) > n else '')

def main():
    insns, rs = routines()
    a = sys.argv[1:]
    if a and a[0] == '--tree':
        root = int(a[1], 16); depth = int(a[2]) if len(a) > 2 else 3
        def walk(e, d, seen):
            r = rs.get(e)
            print('  '*d + f'{e:04X}' + (f'  ports {fmt(r["ports"])}' if r and r['ports'] else '') + (f'  int {fmt(r["ints"])}' if r and r['ints'] else ''))
            if r is None or d >= depth or e in seen: return
            for c in sorted(x for x in r['callees'] if not isinstance(x, str)): walk(c, d + 1, seen | {e})
        walk(root, 0, set()); return
    if a:
        e = int(a[0], 16); r = rs[e]
        for k in ('size', 'callers', 'callees', 'ports', 'ints', 'reads', 'writes'):
            v = r[k]; print(f'{k:8} {v if isinstance(v, int) else fmt(v, 40)}')
        return
    print('entry size  #in #out ports           ints   DATA r/w')
    for e, r in rs.items():
        print(f'{e:04X} {r["size"]:5} {len(r["callers"]):4} {len(r["callees"]):4} {fmt(r["ports"], 4):15} {fmt(r["ints"], 3):6} {len(r["reads"])}/{len(r["writes"])}')

if __name__ == '__main__':
    main()
