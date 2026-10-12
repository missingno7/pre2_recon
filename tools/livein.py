"""Registers each routine reads before writing them (live at entry), static.

    python tools/livein.py [LABEL ...]     all routines, or the named ones

A backward liveness pass over each routine's body (calls are followed into their own
live-in sets; a register a callee may change is conservatively treated as preserved, so
only the routine's own and its callees' reads count). `push reg` is not a read: saving a
register to restore it is not consuming its value. Flags are reported as CF/ZF/DF when an
instruction reads them before any instruction sets them. The result lists the register
contract a C translation must take as parameters (or deliberately ignore with a reason).
Conservative: a callee is assumed to preserve registers, so a register a callee RETURNS
(e.g. ReadJoystickTimes' AX/BX) and the caller then reads shows up as a live-in of the
caller. Treat the output as candidates to adjudicate, not as proven contracts.
"""
from explore import explore, all_seeds, near_target, X86
from image import original
import sys

REG16 = {'al': 'ax', 'ah': 'ax', 'ax': 'ax', 'bl': 'bx', 'bh': 'bx', 'bx': 'bx', 'cl': 'cx', 'ch': 'cx',
         'cx': 'cx', 'dl': 'dx', 'dh': 'dx', 'dx': 'dx', 'si': 'si', 'di': 'di', 'bp': 'bp', 'es': 'es',
         'eax': 'ax', 'ebx': 'bx', 'ecx': 'cx', 'edx': 'dx', 'esi': 'si', 'edi': 'di', 'ebp': 'bp'}
TRACK = ('ax', 'bx', 'cx', 'dx', 'si', 'di', 'bp', 'es', 'CF', 'ZF', 'DF')
FLAG_READ = {'jb': 'CF', 'jae': 'CF', 'jbe': 'CF', 'ja': 'CF', 'adc': 'CF', 'sbb': 'CF', 'rcl': 'CF', 'rcr': 'CF',
             'je': 'ZF', 'jne': 'ZF', 'cmc': 'CF'}

def rw(ins):
    """(registers read, registers written) as 16-bit names plus CF/ZF/DF."""
    try: r, w = ins.regs_access()
    except Exception: r, w = [], []
    reads = {REG16.get(ins.reg_name(x)) for x in r} - {None}
    writes = {REG16.get(ins.reg_name(x)) for x in w} - {None}
    m = ins.mnemonic
    if m == 'push': reads = set()
    if m in ('xor', 'sub') and len(ins.operands) == 2 and ins.operands[0].type == X86.X86_OP_REG \
            and ins.operands[1].type == X86.X86_OP_REG and ins.operands[0].reg == ins.operands[1].reg:
        reads = set()
    base = m.split()[-1]
    if base in ('movsb', 'movsw', 'stosb', 'stosw', 'lodsb', 'lodsw', 'scasb', 'scasw', 'cmpsb', 'cmpsw'): reads.add('DF')
    if m in FLAG_READ: reads.add(FLAG_READ[m])
    if m in ('jbe', 'ja'): reads.add('ZF')
    flags_set = m in ('add', 'sub', 'cmp', 'and', 'or', 'xor', 'test', 'inc', 'dec', 'neg', 'shl', 'shr', 'sar',
                      'sal', 'rol', 'ror', 'adc', 'sbb', 'clc', 'stc', 'mul', 'imul', 'div', 'idiv', 'scasb', 'scasw',
                      'cmpsb', 'cmpsw', 'popf', 'iret')
    if flags_set: writes |= {'CF', 'ZF'} if m not in ('inc', 'dec') else {'ZF'}
    if m in ('cld', 'std', 'popf'): writes.add('DF')
    return reads, writes

def routine_livein(insns, entry, summaries):
    """Live-in set at `entry` (fixpoint over the intra-routine CFG)."""
    body, work = set(), [entry]
    succ = {}
    while work:
        o = work.pop()
        if o in body or o not in insns: continue
        body.add(o); ins = insns[o]; m = ins.mnemonic; nxt = o + ins.size; s = []
        if m in ('ret', 'retf', 'iret', 'hlt'): pass
        elif m in ('jmp', 'ljmp'):
            t = near_target(ins) if m == 'jmp' else None
            if t is not None: s.append(t)
        elif m.startswith('j') or m.startswith('loop'):
            t = near_target(ins)
            if t is not None: s.append(t)
            s.append(nxt)
        else: s.append(nxt)
        succ[o] = s; work.extend(s)
    live = {o: set() for o in body}
    changed = True
    while changed:
        changed = False
        for o in sorted(body, reverse=True):
            ins = insns[o]
            out = set().union(*(live.get(s, set()) for s in succ[o])) if succ[o] else set()
            reads, writes = rw(ins)
            m = ins.mnemonic
            if m == 'xchg' and len(ins.operands) == 2 and all(op.type == X86.X86_OP_REG for op in ins.operands):
                a, b = (REG16.get(ins.reg_name(op.reg)) for op in ins.operands)
                if a and b and ins.operands[0].size == 2:     # a value move, not a consumption
                    swapped = {b if x == a else a if x == b else x for x in out}
                    if swapped != live[o]: live[o] = swapped; changed = True
                    continue
            if m == 'int':
                reads, writes = {'ax'}, {'CF', 'ZF'}           # function number in; carry/zero out
            if ins.mnemonic == 'call':
                t = near_target(ins)
                reads = reads | summaries.get(t, set())
                writes = set()        # conservative: callee assumed to preserve
            new = (out - writes) | reads
            if new != live[o]: live[o] = new; changed = True
    return live.get(entry, set()) & set(TRACK)

def all_liveins():
    h, img, rel = original()
    insns, xrefs, ind, calls = explore(img, all_seeds(img))
    summ = {t: set() for t in calls}
    for _ in range(8):
        new = {t: routine_livein(insns, t, summ) for t in calls}
        if new == summ: break
        summ = new
    return summ

if __name__ == '__main__':
    import where
    L = where.labels(case=True)
    names = {}
    for n, lin in L.items():
        if 0x200 <= lin < 0x9FFC: names.setdefault(lin - 0x200, n)
    summ = all_liveins()
    want = {L[a] - 0x200 for a in sys.argv[1:]} if sys.argv[1:] else None
    for t in sorted(summ):
        if want is not None and t not in want: continue
        regs = sorted(summ[t] - {'DF'})
        if regs: print(f'{t:04X} {names.get(t, "?"):32} {" ".join(regs)}')
