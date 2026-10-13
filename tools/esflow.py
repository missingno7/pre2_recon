"""ES value at chosen instructions, with per-routine ES summaries (static).

    python tools/esflow.py 01C4 [...]     ES before the instruction at each code offset

Like segflow.py, but for ES and without optimism: a routine's ES effect is 'IN' (ES restored
on every return), 'DATA' (ES = DATA on every return) or None (anything else), computed to a
fixpoint; calls into routines with unknown effect leave ES unknown. Interrupts are assumed
to preserve ES (the DOS/BIOS functions the game calls do, except INT 21h/35h and INT 2Fh
XMS entry queries, which only occur in vector and memory set-up). Used to settle stale-ES
contracts at call sites the C translation must reproduce.
"""
from segflow import State, merge, table_call_sites, GPR
from segflow import step as seg_step
from explore import explore, all_seeds, near_target, X86, relocated, table_entries
from image import original, CODE_START
import sys

def step(ins, st):
    """segflow.step, plus symbolic values: an immediate (`A000`) or a word variable (`[2875]`)."""
    out = seg_step(ins, st)
    if ins.mnemonic == 'mov' and len(ins.operands) == 2 and ins.operands[0].type == X86.X86_OP_REG:
        dst = ins.reg_name(ins.operands[0].reg); src = ins.operands[1]
        if dst in GPR or dst == 'es':
            if src.type == X86.X86_OP_IMM and out.get(dst) is None and \
                    not any(relocated(CODE_START + ins.address + i) for i in range(ins.size)):
                out.set(dst, f'{src.imm & 0xFFFF:04X}')
            elif src.type == X86.X86_OP_MEM and src.mem.base == 0 and src.mem.index == 0:
                seg = ins.reg_name(src.mem.segment) if src.mem.segment else 'ds'
                out.set(dst, f'{seg}:[{src.mem.disp & 0xFFFF:04X}]')
    return out

def run(img, insns, entries, sites, summary, start):
    states, work, rets = {}, [], []
    def flow(off, st):
        if off not in insns: return
        old = states.get(off)
        new = merge(old, st) if old is not None else st.copy()
        if old is None or new.key() != old.key(): states[off] = new; work.append(off)
    for e, st in start: flow(e, st)
    while work:
        off = work.pop(); ins = insns[off]; st = states[off]; m = ins.mnemonic; nxt = off + ins.size
        if m in ('ret', 'retf'): rets.append(st.get('es')); continue
        if m in ('iret', 'hlt'): continue
        if m in ('call', 'lcall'):
            t = near_target(ins) if m == 'call' else None
            if entries is not None and t is not None: flow(t, State(st.regs, ()))
            eff = summary.get(t, None) if t is not None else None
            out = State({k: v for k, v in st.regs.items() if k in ('ds', 'es')}, st.stack)
            out.set('es', st.get('es') if eff == 'IN' else ('DATA' if eff == 'DATA' else None))
            flow(nxt, out); continue
        if m == 'int':
            out = State({k: v for k, v in st.regs.items() if k in ('ds', 'es')}, st.stack); flow(nxt, out); continue
        out = step(ins, st)
        if m in ('jmp', 'ljmp'):
            t = near_target(ins) if m == 'jmp' else None
            if t is not None: flow(t, out)
            else: rets.append(None)
            continue
        if m.startswith('j') or m.startswith('loop'):
            t = near_target(ins)
            if t is not None: flow(t, out)
        flow(nxt, out)
    return states, rets

def summaries(img, insns, calls):
    summary = {t: 'IN' for t in calls}
    for _ in range(12):
        new = {}
        for t in calls:
            _, rets = run(img, insns, None, None, summary, [(t, State({'es': 'IN'}))])
            r = set(rets)
            new[t] = 'IN' if r <= {'IN'} else ('DATA' if r == {'DATA'} else None)
        if new == summary: break
        summary = new
    return summary

# ---- value sets (--sets) ----------------------------------------------------------------
# ES as the set of values it can hold: 'IN' (the routine's entry ES), an immediate ('A000'),
# 'DATA', 'CODE', a word variable ('ds:[2875]'), 'UNK'. A routine's summary is the set at its
# returns; a call replaces 'IN' in it by the caller's set. Indirect calls through the hint
# tables take the union of their targets. Join points take unions (no path is dropped).

def _es(v):
    if v is None: return ('UNK',)
    return (v,) if isinstance(v, str) else tuple(v)

def _merge_sets(a, b):
    if a is None: return b.copy()
    out = merge(a, b)
    out.set('es', tuple(sorted(set(_es(a.get('es'))) | set(_es(b.get('es'))))))
    return out

def indirect_sites(img):
    """Indirect call/jump sites -> targets, from the hint tables' evidence and callbacks."""
    import re
    from hints import CODE_TABLES
    sites = {}
    for seg, off, n, ev in CODE_TABLES:
        targets = set(table_entries(img, seg, off, n))
        for s in re.findall(r'\b([0-9A-F]{4})\b(?=[/ ])', ev.split(';')[0]):
            sites.setdefault(int(s, 16), set()).update(targets)
    sites.setdefault(0x97F8, set()).update({0x991F, 0x9985})   # FrameCallback (ds:B1AE)
    return sites

def run_sets(img, insns, summary, start, sites, follow_calls=False):
    states, work, rets = {}, [], set()
    def flow(off, st):
        if off not in insns: return
        old = states.get(off)
        new = _merge_sets(old, st)
        if old is None or new.key() != old.key(): states[off] = new; work.append(off)
    for e, st in start: flow(e, st)
    while work:
        off = work.pop(); ins = insns[off]; st = states[off]; m = ins.mnemonic; nxt = off + ins.size
        es = _es(st.get('es'))
        if m in ('ret', 'retf'): rets |= set(es); continue
        if m in ('iret', 'hlt'): continue
        if m in ('call', 'lcall'):
            t = near_target(ins) if m == 'call' else None
            targets = [t] if t is not None else sorted(sites.get(off, ()))
            if follow_calls:
                for c in targets: flow(c, State({'es': es}, ()))
            res = set()
            for c in targets:
                for v in summary.get(c, ('UNK',)): res |= set(es) if v == 'IN' else {v}
            out = State({k: v for k, v in st.regs.items() if k == 'ds'}, st.stack)
            out.set('es', tuple(sorted(res or {'UNK'})))
            flow(nxt, out); continue
        if m == 'int':
            out = State({k: v for k, v in st.regs.items() if k in ('ds', 'es')}, st.stack); flow(nxt, out); continue
        out = step(ins, st)
        if out.get('es') is None and st.get('es') is not None and not _writes_es(ins): out.set('es', st.get('es'))
        out.set('es', _es(out.get('es')))
        if PROVENANCE and _loads_es(ins):          # tag each value with the loading instruction
            out.set('es', tuple(sorted({v if '@' in v or v == 'IN' else f'{v}@{off:04X}' for v in out.get('es')})))
        if m in ('jmp', 'ljmp'):
            t = near_target(ins) if m == 'jmp' else None
            if t is not None: flow(t, out)
            else:
                for c in sorted(sites.get(off, ())): flow(c, out)
                if not sites.get(off): rets.add('UNK')
            continue
        if m.startswith('j') or m.startswith('loop'):
            t = near_target(ins)
            if t is not None: flow(t, out)
        flow(nxt, out)
    return states, rets

PROVENANCE = '--why' in sys.argv     # --why: values carry '@XXXX', the instruction that loaded ES

def _loads_es(ins):
    ops = ins.operands
    if ins.mnemonic in ('mov', 'pop') and ops and ops[0].type == X86.X86_OP_REG and ins.reg_name(ops[0].reg) == 'es':
        return True
    return ins.mnemonic == 'les'

def _writes_es(ins):
    _, written = ins.regs_access()
    return any(ins.reg_name(r) == 'es' for r in written) or ins.mnemonic in ('pop', 'les', 'mov')

def summaries_sets(img, insns, calls, sites):
    summary = {t: ('IN',) for t in calls}
    for _ in range(30):
        new = {}
        for t in calls:
            _, rets = run_sets(img, insns, summary, [(t, State({'es': ('IN',)}))], sites)
            new[t] = tuple(sorted(rets)) or ('IN',)
        if new == summary: break
        summary = new
    return summary

if __name__ == '__main__':
    h, img, rel = original()
    insns, xrefs, indirect, calls = explore(img, all_seeds(img))
    from hints import CODE_SEEDS
    args = [a for a in sys.argv[1:] if not a.startswith('-')]
    if '--sets' in sys.argv:
        sites = indirect_sites(img)
        summ = summaries_sets(img, insns, sorted(calls | {c for s in sites.values() for c in s}), sites)
        states, _ = run_sets(img, insns, summ, [(e, State({'es': ('UNK',)})) for e in CODE_SEEDS], sites, True)
        for a in args:
            if a.startswith('@'):                 # @XXXX: the summary of the routine at XXXX
                o = int(a[1:], 16); print(f'{o:04X}: returns ES in {summ.get(o)}')
            else:
                o = int(a, 16); st = states.get(o)
                print(f'{o:04X}: ES in {_es(st.get("es")) if st else "unreached"}')
        sys.exit(0)
    summ = summaries(img, insns, sorted(calls))
    states, _ = run(img, insns, True, None, summ, [(e, State()) for e in CODE_SEEDS])
    for a in args:
        o = int(a, 16); st = states.get(o)
        print(f'{o:04X}: ES = {st.get("es") if st else "unreached"}')
