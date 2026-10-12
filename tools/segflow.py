"""Static segment-register values (DS, ES) before every discovered instruction.

A forward dataflow over the discovered code: values are 'DATA' (the data segment
frame, loaded from a relocated `seg DATA` word or copied from a register holding it),
'ZERO' (segment 0000, the interrupt vector table), 'CODE' (copied from CS) or None
(unknown). Calls propagate the caller's state into the callee; after a call DS is
assumed preserved (the game's convention: routines that change DS restore it or
reload DATA) and ES and the general registers are unknown. Interrupt handlers and
the program entry start unknown. Join points merge to None on disagreement.

Used to decide which DS/ES-relative operands are DATA symbols (tools/gen.py).
"""
from image import *
from explore import X86, relocated, near_target
import capstone

GPR = ('ax', 'bx', 'cx', 'dx', 'si', 'di', 'bp')
REG8 = {'al': 'ax', 'ah': 'ax', 'bl': 'bx', 'bh': 'bx', 'cl': 'cx', 'ch': 'cx', 'dl': 'dx', 'dh': 'dx'}
SEGVAL = {DATA_FRAME: 'DATA', CODE_FRAME: 'CODE'}

class State:
    __slots__ = ('regs', 'stack')
    def __init__(self, regs=None, stack=()):
        self.regs = dict(regs or {}); self.stack = tuple(stack)
    def key(self): return (tuple(sorted(self.regs.items())), self.stack)
    def get(self, r): return self.regs.get(r)
    def set(self, r, v):
        if v is None: self.regs.pop(r, None)
        else: self.regs[r] = v
    def copy(self): return State(self.regs, self.stack)

def merge(a, b):
    if a is None: return b.copy()
    regs = {k: v for k, v in a.regs.items() if b.regs.get(k) == v}
    stack = a.stack if a.stack == b.stack else ()
    return State(regs, stack)

def step(ins, st):
    """State after `ins` (for the fall-through / branch edges)."""
    st = st.copy()
    m = ins.mnemonic
    ops = ins.operands
    name = lambda op: ins.reg_name(op.reg) if op.type == X86.X86_OP_REG else None
    if m == 'mov' and len(ops) == 2:
        dst, src = name(ops[0]), ops[1]
        if dst in GPR or dst in ('ds', 'es'):
            if src.type == X86.X86_OP_IMM:
                v = src.imm & 0xFFFF
                rel = any(relocated(CODE_START + ins.address + i) for i in range(ins.size))
                st.set(dst, SEGVAL.get(v) if rel else ('ZERO' if v == 0 else None))
            elif src.type == X86.X86_OP_REG:
                s = name(src)
                st.set(dst, 'CODE' if s == 'cs' else st.get(s))
            else: st.set(dst, None)
            return st
    if m in ('xor', 'sub') and len(ops) == 2 and name(ops[0]) and name(ops[0]) == name(ops[1]) and name(ops[0]) in GPR:
        st.set(name(ops[0]), 'ZERO'); return st
    if m == 'push':
        r = name(ops[0]) if ops else None
        st.stack = (st.stack + ('CODE' if r == 'cs' else st.get(r) if r else None,))[-16:]
        return st
    if m == 'pop':
        v = st.stack[-1] if st.stack else None
        st.stack = st.stack[:-1]
        r = name(ops[0])
        if r: st.set(r, v)
        return st
    if m in ('pushf',): st.stack = (st.stack + (None,))[-16:]; return st
    if m in ('popf',): st.stack = st.stack[:-1]; return st
    if m in ('pushal', 'pushaw', 'popal', 'popaw', 'leave', 'enter'): st.stack = ();
    if m == 'xchg' and len(ops) == 2 and name(ops[0]) in GPR and name(ops[1]) in GPR:
        a, b = name(ops[0]), name(ops[1]); va, vb = st.get(a), st.get(b); st.set(a, vb); st.set(b, va); return st
    # anything else: every written register becomes unknown
    _, written = ins.regs_access()
    for r in written:
        n = ins.reg_name(r)
        n = REG8.get(n, n)
        if n in GPR or n in ('ds', 'es'): st.set(n, None)
        if n == 'sp': st.stack = ()
    # add/sub sp,imm, mov sp,... break the stack model
    return st

SUMMARY = {}   # routine entry -> 'IN' (DS preserved) | 'DATA' (DS = DATA at every return) | None

def after_call(st, target=None):
    eff = SUMMARY.get(target, 'IN')
    # 'DATA': the routine reloads DATA on every path. Unknown effects are treated as
    # preserving (optimistic): only names depend on this, never bytes.
    ds = eff if eff == 'DATA' else st.get('ds')
    return State({'ds': ds} if ds else {}, st.stack)

def segflow(img, insns, entries_unknown, call_entries):
    """Return {code offset: State before the instruction}.

    entries_unknown: offsets that start with unknown state (entry, interrupt handlers).
    call_entries: {target: [indirect call site offsets]} for table-dispatched routines."""
    states = {}
    work = []
    def flow(off, st):
        if off not in insns: return
        old = states.get(off)
        new = merge(old, st) if old is not None else st.copy()
        if old is None or new.key() != old.key():
            states[off] = new; work.append(off)
    for e in entries_unknown: flow(e, State())
    while work:
        off = work.pop()
        ins = insns[off]; st = states[off]
        m = ins.mnemonic
        nxt = off + ins.size
        if m in ('ret', 'retf', 'iret', 'hlt'): continue
        if m in ('call', 'lcall'):
            t = near_target(ins) if m == 'call' else (ins.operands[1].imm & 0xFFFF if len(ins.operands) == 2 and relocated(CODE_START + off + 3) else None)
            callee_in = step(ins, st) if False else State(st.regs, ())
            if t is not None: flow(t, callee_in)
            for target in call_entries.get(off, ()): flow(target, callee_in)
            flow(nxt, after_call(st, t)); continue
        if m == 'int':
            out = State({'ds': st.get('ds')} if st.get('ds') else {}, st.stack); flow(nxt, out); continue
        out = step(ins, st)
        if m == 'jmp' or m == 'ljmp':
            t = near_target(ins) if m == 'jmp' else None
            if t is not None: flow(t, out)
            for target in call_entries.get(off, ()): flow(target, State(st.regs, ()))
            continue
        if m.startswith('j') or m.startswith('loop'):
            t = near_target(ins)
            if t is not None: flow(t, out)
        flow(nxt, out)
    return states

def summarize(img, insns, entry):
    """DS effect of the routine at `entry`, analysed with DS = 'IN' on entry."""
    states, work, rets = {}, [entry], set()
    def flow(off, st):
        if off not in insns: return
        old = states.get(off)
        new = merge(old, st) if old is not None else st.copy()
        if old is None or new.key() != old.key(): states[off] = new; work.append(off)
    states[entry] = State({'ds': 'IN'}); work = [entry]
    while work:
        off = work.pop(); ins = insns[off]; st = states[off]; m = ins.mnemonic; nxt = off + ins.size
        if m in ('ret', 'retf'): rets.add(st.get('ds')); continue
        if m in ('iret', 'hlt'): continue
        if m in ('call', 'lcall'):
            t = near_target(ins) if m == 'call' else None
            flow(nxt, after_call(st, t)); continue
        if m == 'int': flow(nxt, State({'ds': st.get('ds')} if st.get('ds') else {}, st.stack)); continue
        out = step(ins, st)
        if m in ('jmp', 'ljmp'):
            t = near_target(ins) if m == 'jmp' else None
            if t is not None: flow(t, out)
            else: rets.add(None)         # indirect tail jump: effect unknown
            continue
        if m.startswith('j') or m.startswith('loop'):
            t = near_target(ins)
            if t is not None: flow(t, out)
        flow(nxt, out)
    if rets == {'IN'}: return 'IN'
    if rets == {'DATA'}: return 'DATA'
    if not rets: return 'IN'
    return None

def summaries(img, insns, entries):
    SUMMARY.clear()
    for _ in range(6):
        new = {e: summarize(img, insns, e) for e in entries}
        if new == SUMMARY: break
        SUMMARY.clear(); SUMMARY.update(new)
    return dict(SUMMARY)

def table_call_sites(img, insns, tables):
    """{indirect call/jmp site: [table targets]} for tables indexed through [reg+table]."""
    from explore import table_entries
    out = {}
    for off, ins in insns.items():
        if ins.mnemonic not in ('call', 'jmp') or not ins.operands or ins.operands[0].type != X86.X86_OP_MEM: continue
        disp = ins.operands[0].mem.disp & 0xFFFF
        seg = 'cs' if ins.operands[0].mem.segment == X86.X86_REG_CS else 'ds'
        for tseg, toff, n, _ in tables:
            if tseg == seg and toff <= disp < toff + 2*n:
                out.setdefault(off, []).extend(table_entries(img, tseg, disp, (toff + 2*n - disp)//2))
    return out

def analyse():
    from explore import explore, all_seeds
    from hints import CODE_SEEDS, CODE_TABLES
    h, img, rel = original()
    insns, xrefs, indirect, calls = explore(img, all_seeds(img))
    sites = table_call_sites(img, insns, CODE_TABLES)
    summaries(img, insns, sorted(calls))
    return img, insns, segflow(img, insns, CODE_SEEDS, sites)

if __name__ == '__main__':
    import collections
    img, insns, states = analyse()
    print(len(states), 'of', len(insns), 'instructions reached')
    c = collections.Counter()
    for off, ins in insns.items():
        for op in ins.operands:
            if op.type != X86.X86_OP_MEM: continue
            seg = op.mem.segment
            if seg == X86.X86_REG_INVALID:
                seg = 'ss' if op.mem.base in (X86.X86_REG_BP,) else 'ds'
            else: seg = ins.reg_name(seg)
            st = states.get(off)
            v = st.get(seg) if st and seg in ('ds', 'es') else seg
            c[(seg, v)] += 1
    for k, v in sorted(c.items(), key=lambda x: -x[1]): print(k, v)
