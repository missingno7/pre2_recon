"""Recursive-descent code discovery over the code segment (bootstrap analysis).

Seeds: the entry point, explicit code addresses in hints (interrupt handlers, jump
and call tables, far pointers), and far code pointers proven by relocations.
Indirect jumps/calls are reported so their tables can be bounded by hand in hints.
"""
from common import *
from image import *
import capstone, re

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
md.detail = True
X86 = capstone.x86

COND = {'je','jne','jb','jae','jbe','ja','jl','jge','jle','jg','js','jns','jo','jno','jp','jnp',
        'jcxz','loop','loope','loopne'}

def decode(img, off):
    """Decode one instruction at code offset `off` (relative to CODE_FRAME)."""
    lin = CODE_START + off
    return next(md.disasm(img[lin:min(lin + 16, CODE_END)], off), None)

def near_target(ins):
    op = ins.operands[0]
    if op.type == X86.X86_OP_IMM: return op.imm & 0xFFFF
    return None

RELOCS = None
def relocated(lin):
    global RELOCS
    if RELOCS is None: RELOCS = set(original()[2])
    return lin in RELOCS

def explore(img, seeds, stop=()):
    """Return (insns: off->insn, xrefs: target->set(src), indirect: list of insn, calls: set)."""
    insns, xrefs, indirect, calls = {}, {}, [], set()
    work = list(seeds)
    stop = set(stop)
    while work:
        off = work.pop()
        while True:
            if off in insns or off in stop or not 0 <= off < CODE_END - CODE_START: break
            ins = decode(img, off)
            if ins is None: raise ValueError(f'Undecodable code at {off:04X}')
            insns[off] = ins
            m = ins.mnemonic
            nxt = off + ins.size
            if m in COND or m == 'jmp' or m == 'call':
                t = near_target(ins)
                if t is not None:
                    xrefs.setdefault(t, set()).add(off)
                    work.append(t)
                    if m == 'call': calls.add(t)
                else:
                    indirect.append(ins)
                if m == 'jmp': break
            elif m in ('lcall', 'ljmp'):
                if len(ins.operands) == 2 and not relocated(CODE_START + off + 3):
                    pass                     # far pointer patched at run time (not a program target)
                elif len(ins.operands) == 2:   # direct far: seg:off immediate, segment relocated
                    if word(img, CODE_START + off + 3) != CODE_FRAME: raise ValueError(f'far target frame at {off:04X}')
                    t = ins.operands[1].imm & 0xFFFF
                    xrefs.setdefault(t, set()).add(off); work.append(t); calls.add(t)
                else: indirect.append(ins)
                if m == 'ljmp': break
            elif m in ('ret', 'retf', 'iret', 'hlt'):
                break
            off = nxt
    return insns, xrefs, indirect, calls

def table_entries(img, seg, off, n):
    base = CODE_START if seg == 'cs' else DATA_FRAME*16
    return [word(img, base + off + 2*i) for i in range(n)]

def all_seeds(img):
    from hints import CODE_SEEDS, CODE_TABLES
    seeds = list(CODE_SEEDS)
    for seg, off, n, _ in CODE_TABLES: seeds += table_entries(img, seg, off, n)
    return seeds

def gaps(insns):
    """Code-segment byte ranges not covered by discovered instructions."""
    covered = bytearray(CODE_END - CODE_START)
    for off, ins in insns.items():
        for i in range(ins.size): covered[off + i] = 1
    out, i = [], 0
    while i < len(covered):
        if not covered[i]:
            j = i
            while j < len(covered) and not covered[j]: j += 1
            out.append((i, j)); i = j
        else: i += 1
    return out

if __name__ == '__main__':
    import sys
    h, img, rel = original()
    insns, xrefs, indirect, calls = explore(img, all_seeds(img))
    print(len(insns), 'instructions')
    for ins in indirect: print(f'indirect {ins.address:04X}: {ins.mnemonic} {ins.op_str}')
    g = gaps(insns)
    print(len(g), 'gaps', sum(b - a for a, b in g), 'bytes')
    if '-g' in sys.argv:
        for a, b in g: print(f'gap {a:04X}..{b:04X} ({b-a}) {img[CODE_START+a:CODE_START+min(b,a+16)].hex()}')
