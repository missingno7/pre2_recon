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
from explore import explore, all_seeds, near_target, X86, relocated
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

if __name__ == '__main__':
    h, img, rel = original()
    insns, xrefs, indirect, calls = explore(img, all_seeds(img))
    summ = summaries(img, insns, sorted(calls))
    from hints import CODE_SEEDS
    states, _ = run(img, insns, True, None, summ, [(e, State()) for e in CODE_SEEDS])
    for a in sys.argv[1:]:
        o = int(a, 16); st = states.get(o)
        print(f'{o:04X}: ES = {st.get("es") if st else "unreached"}')
