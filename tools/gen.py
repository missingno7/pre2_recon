"""Bootstrap generator: original image + hints -> TASM source that rebuilds it exactly.

Code found by tools/explore.py is written as instructions; everything else as data
rows (`; UNKNOWN` heads unclassified bytes). Relocation sites are written
symbolically so TLINK emits them. The generator then assembles, compares each
instruction with the original and rewrites any line TASM encodes differently as an
explicit encoding (db rows with the instruction in a comment), until the build
matches. Used only during bootstrap; after the switch to hand maintenance it goes.
"""
from common import *
from image import *
from explore import explore, all_seeds, decode, relocated, table_entries, X86
from hints import CODE_TABLES, CS_DATA, DEAD_CODE
from dos import dos
import re, shutil, sys

OUT = ROOT/'build'/'gen'

def h(v, width=0):
    s = f'{v:0{width}X}h'
    return '0' + s if s[0] in 'ABCDEF' else s

def hexnum(m):
    v = int(m.group(1), 16)
    return h(v)

def fix_numbers(s):
    return re.sub(r'0x([0-9a-f]+)', hexnum, s)

STRING_OPS = {'movsb','movsw','lodsb','lodsw','stosb','stosw','scasb','scasw','cmpsb','cmpsw','insb','insw','outsb','outsw'}
SEG_PREFIX = {0x26: 'es', 0x2E: 'cs', 0x36: 'ss', 0x3E: 'ds'}

def mem_text(op_str):
    """capstone memory operand -> TASM: explicit ds: on bare direct addresses."""
    s = op_str.replace(' + ', '+').replace(' - ', '-')
    s = re.sub(r'(?<![:\w])\[(0x[0-9a-f]+|\d+)\]', r'ds:[\1]', s)
    return fix_numbers(s)

class Labels:
    def __init__(self): self.names = {}
    def want(self, off, kind):
        if off not in self.names or (kind == 'sub' and self.names[off].startswith('loc_')):
            self.names[off] = f'{kind}_{off:04X}'
        return self.names[off]

DATA_END = 0xB1BC          # end of DATA including the uninitialized tail (ds offset)
CS_ALIAS = (DATA_FRAME - CODE_FRAME)*16   # cs:X with X >= CODE size is ds:(X - CS_ALIAS)
SIZE_KIND = {1: 'byte', 2: 'word', 4: 'dword'}

def data_refs(insns, states):
    """Memory operands that address DATA or code-segment data, from the segment dataflow.

    Returns ({code offset: (disp, segment kind)}, {ds offset: access sizes},
    {code offset of cs variable: access sizes}). Only DS-default operands with DS = DATA,
    explicit es:/ds: with that register = DATA, and cs: operands qualify; bp-based
    (SS) operands never do. Indexed operands qualify only with a 16-bit displacement:
    TASM always encodes a relocatable symbol with disp16, so a disp8 is a constant."""
    refs, dsizes, csizes = {}, {}, {}
    for off, ins in insns.items():
        st = states.get(off)
        for op in ins.operands:
            if op.type != X86.X86_OP_MEM: continue
            mem = op.mem
            direct = mem.base == 0 and mem.index == 0
            if not direct and ins.disp_size != 2: continue
            disp = mem.disp & 0xFFFF
            seg = ins.reg_name(mem.segment) if mem.segment else None
            if seg is None:
                if mem.base == X86.X86_REG_BP: continue
                seg = 'ds'
                explicit = False
            if seg == 'cs':
                if disp >= CODE_END - CODE_START:
                    d = disp - CS_ALIAS
                    if 0x000C <= d < DATA_END: refs[off] = (disp, 'cs-alias', d); dsizes.setdefault(d, []).append(op.size)
                else:
                    refs[off] = (disp, 'cs', disp); csizes.setdefault(disp, []).append(op.size)
                continue
            if seg in ('ds', 'es') and st is not None and st.get(seg) == 'DATA' and 0x000C <= disp < DATA_END:
                refs[off] = (disp, seg, disp); dsizes.setdefault(disp, []).append(op.size)
    return refs, dsizes, csizes

def size_name(sizes):
    import collections
    c = collections.Counter(x for x in sizes if x in SIZE_KIND)
    return SIZE_KIND[c.most_common(1)[0][0]] if c else 'byte'

DREFS, DNAMES, CNAMES = {}, {}, {}

def symbolize(ins, text):
    """Replace the numeric displacement of a DATA / cs-variable operand by its label."""
    if text is None or ins.address not in DREFS: return text
    disp, kind, target = DREFS[ins.address]
    name = DNAMES.get(target) if kind != 'cs' else CNAMES.get(target)
    if name is None: return text
    num = re.escape(h(disp))
    explicit_ds = 0x3E in ins.bytes[:3]
    new = re.sub(r'\b(ds|es|cs):\[' + num + r'\]', lambda m: (m.group(1) + ':' if m.group(1) != 'ds' or explicit_ds else '') + name, text)
    if new == text:
        new = re.sub(r'\[(\w\w(?:\+\w\w)?)\+' + num + r'\]', lambda m: '[' + m.group(1) + '+' + name + ']', text)
    return new

def ins_text(ins, img, labels):
    """TASM text for one instruction, or None when it needs an explicit encoding."""
    m, ops, b = ins.mnemonic, ins.op_str, ins.bytes
    off = ins.address
    prefixes = [x for x in b[:4] if x in SEG_PREFIX]
    # relocation sites inside the instruction
    rel = [i for i in range(ins.size) if relocated(CODE_START + off + i)]
    if m in ('jmp', 'call') or m.startswith('j') or m.startswith('loop'):
        if ins.operands and ins.operands[0].type == X86.X86_OP_IMM and m not in ('ljmp', 'lcall'):
            t = ins.operands[0].imm & 0xFFFF
            name = labels.names[t]
            if m == 'jmp': return f'jmp short {name}' if b[0] == 0xEB else f'jmp near ptr {name}'
            return f'{m} {name}'
    if m in ('lcall', 'ljmp'):
        word_ = 'call' if m == 'lcall' else 'jmp'
        if len(ins.operands) == 2:
            if rel: return f'{word_} far ptr {labels.names[ins.operands[1].imm & 0xFFFF]}'
            return None
        return f'{word_} dword ptr ' + mem_text(ops.replace('ptr ', ''))
    if m == 'cwde' or b[-1:] == b'\x98' and ins.size == 1: return 'cbw'
    if m == 'cdq' or b == b'\x99': return 'cwd'
    if m == 'xlatb': return f'xlat byte ptr {SEG_PREFIX[prefixes[0]]}:[bx]' if prefixes else 'xlat'
    if m in ('int3',): return 'int 3'
    if m in ('int1', 'salc', 'sal') or m.startswith('f'): return None
    base = m.split()[-1]
    if base in STRING_OPS:
        rep = m.split()[0] + ' ' if ' ' in m else ''
        if not prefixes: return rep + base
        size = 'byte' if base.endswith('b') else 'word'
        seg = SEG_PREFIX[prefixes[0]]
        k = base[:-1]
        if k in ('movs', 'cmps'): operands = f'{size} ptr es:[di], {size} ptr {seg}:[si]'
        elif k in ('lods', 'outs'): operands = f'{size} ptr {seg}:[si]'
        else: return None
        if k == 'outs': operands = 'dx, ' + operands
        return f'{rep}{k} {operands}'
    if m == 'les' or m == 'lds':
        return f'{m} ' + mem_text(ops.replace('ptr ', 'dword ptr '))
    text = f'{m} {mem_text(ops)}'.strip()
    core = [x for x in b if x not in SEG_PREFIX and x not in (0xF2, 0xF3)]
    if core and core[0] in (0xD0, 0xD1) and ',' not in text: text += ', 1'
    if re.search(r'\be[abcd]x\b|\be[sd]i\b|\be[sb]p\b', text) or core and core[0] in (0x66, 0x67):
        text = 'P386 ' + text          # generate() writes P386 / instruction / P8086
    if rel:
        if m != 'mov' and m != 'add' and m != 'push' and m != 'cmp':
            return None
        imm = [o for o in ins.operands if o.type == X86.X86_OP_IMM]
        if len(imm) != 1: return None
        v = imm[0].imm & 0xFFFF
        sym = {DATA_FRAME: 'DATA', DATA_FRAME + 1: 'DATA+1', CODE_FRAME: 'CODE'}.get(v)
        if sym is None: return None
        text = re.sub(r', [0-9A-F]+h$', ', seg ' + sym, text)   # seg: DATA is a forward reference
    return text

SEGSYM = {DATA_FRAME: 'DATA', DATA_FRAME + 1: 'DATA+1', CODE_FRAME: 'CODE'}

def alternative(ins, text):
    """Second spelling for instructions whose encoding depends on operand order."""
    if text is None: return None
    m = re.match(r'^(xchg|test) (\w\w), (\w\w)$', text)
    if m: return f'{m.group(1)} {m.group(3)}, {m.group(2)}'
    if ins.mnemonic == 'xlatb':
        seg = SEG_PREFIX.get(ins.bytes[0])
        if seg: return f'xlat byte ptr {seg}:[bx]'
    return None

def db_rows(data, per=12):
    for i in range(0, len(data), per):
        yield '    db ' + ', '.join(h(x, 2) for x in data[i:i+per])

class Source:
    """Lines plus the original image span each instruction line must produce."""
    def __init__(self, name): self.name, self.lines, self.spans = name, [], {}
    def add(self, text, span=None):
        self.lines.append(text)
        if span: self.spans[len(self.lines)] = span   # 1-based line number

# Link modules (see docs/executable.md): the relocation order proves two; module 2's
# CODE part is word aligned, which produces the 00h pad at 23D3.
MODULES = [
    ('MODULE1', 0x0000, 0x23D3, True),    # name, code start, code end, holds STK + DATA
    ('MODULE2', 0x23D4, 0x454D, False),   # the 00h pads at 454D and 951F are word alignment
    ('MODULE3', 0x454E, 0x951F, False),   # of further module starts, like 23D3 (TASM's
    ('MODULE4', 0x9520, CODE_END - CODE_START, False),   # `even` would pad with 90h)
]
BSS_BYTES = 16   # uninitialized DATA tail: MZ min_extra 2, end of data B1BCh (entry code)

def emit_code(S, img, insns, labels, explicit, start, end):
    cs_tables = {toff: n for seg, toff, n, ev in CODE_TABLES if seg == 'cs'}
    off = start
    while off < end:
        if off in cs_tables:
            n = cs_tables[off]
            S(f'tbl_{off:04X} label word')
            for i, v in enumerate(table_entries(img, 'cs', off, n)):
                if i and off + 2*i in CNAMES: S(f'{CNAMES[off + 2*i]} label word')
                S(f'    dw offset {labels.names[v]}')
            off += 2*n; continue
        if off in insns:
            ins = insns[off]
            lab = labels.names.get(off)
            if lab: S(f'{lab}:')
            level = explicit.get(off, 0)
            text = symbolize(ins, ins_text(ins, img, labels))
            if level == 1: text = alternative(ins, text)
            if level >= 2 or text is None:
                t2 = ins_text(ins, img, labels)
                if ins.mnemonic == 'jmp' and ins.bytes[0] == 0xE9:
                    t = labels.names[ins.operands[0].imm & 0xFFFF]
                    S('    db 0E9h ; ENCODED: jmp near ptr ' + t + ' (TASM would encode short)', (off, 1))
                    S(f'    dw {t} - $ - 2', (off + 1, 2))
                else:
                    sites = [i for i in range(ins.size) if relocated(CODE_START + off + i)]
                    pieces, i = [], 0
                    for r in sites:
                        if r > i: pieces.append('db ' + ', '.join(h(x, 2) for x in ins.bytes[i:r]))
                        pieces.append('dw seg ' + SEGSYM[word(img, CODE_START + off + r)]); i = r + 2
                    if i < ins.size: pieces.append('db ' + ', '.join(h(x, 2) for x in ins.bytes[i:]))
                    note = f' ; ENCODED: {t2 or (ins.mnemonic + " " + ins.op_str)}'
                    if len(pieces) == 1: S('    ' + pieces[0] + note, (off, ins.size))
                    else:
                        S('    ' + pieces[0] + note, (off, None))
                        for pc in pieces[1:]: S('    ' + pc)
            elif text.startswith('P386 '):
                S('    P386'); S(f'    {text[5:]}', (off, ins.size)); S('    P8086')
            else:
                S(f'    {text}', (off, ins.size))
            off += ins.size; continue
        lin = CODE_START + off
        if relocated(lin + 2) and word(img, lin + 2) == CODE_FRAME and word(img, lin) in labels.names:
            if off in CNAMES: S(f'{CNAMES[off]} label {CKIND[off]}')
            S(f'    dd {labels.names[word(img, lin)]}'); off += 4; continue
        if relocated(lin):
            if off in CNAMES: S(f'{CNAMES[off]} label {CKIND[off]}')
            S(f'    dw seg {SEGSYM[word(img, lin)]}'); off += 2; continue
        j = off + 1
        while j < end and j not in insns and j not in cs_tables and j not in labels.names and j not in CNAMES \
                and not relocated(CODE_START + j) and not (relocated(CODE_START + j + 2)
                and word(img, CODE_START + j + 2) == CODE_FRAME and word(img, CODE_START + j) in labels.names):
            j += 1
        lab = labels.names.get(off)
        if lab: S(f'{lab}:')
        if off in CNAMES: S(f'{CNAMES[off]} label {CKIND[off]}')
        cls = [c for a, b, c in CS_DATA if a <= off < b]
        if cls:
            j = min(j, [b for a, b, c in CS_DATA if a <= off < b][0])
            if not any(a == off for a, b, c in CS_DATA): cls = None
        S(f'; DATA: {cls[0]}' if cls else '; UNKNOWN: not yet classified as code or data')
        for row in db_rows(img[CODE_START + off:CODE_START + j]): S(row)
        off = j

DKIND, CKIND = {}, {}

def emit_data(S, img, labels):
    dbase = DATA_FRAME*16
    ds_tables = {toff: n for seg, toff, n, _ in CODE_TABLES if seg == 'ds'}
    rel = set(x for x in original()[2] if x >= DATA_START)
    stops = sorted(set([dbase + t for t in ds_tables] + [r - 2 for r in rel] + [dbase + d for d in DNAMES]))
    p = DATA_START
    end = dbase + DATA_END
    while p < end:
        doff = p - dbase
        if doff in DNAMES and doff not in ds_tables: S(f'{DNAMES[doff]} label {DKIND[doff]}')
        if doff in ds_tables:
            n = ds_tables[doff]
            S(f'dtbl_{doff:04X} label word')
            for i, v in enumerate(table_entries(img, 'ds', doff, n)):
                if i and doff + 2*i in DNAMES: S(f'{DNAMES[doff + 2*i]} label word')
                S(f'    dw offset {labels.names[v]}')
            p += 2*n; continue
        if p + 2 in rel:
            assert word(img, p + 2) == CODE_FRAME
            S(f'    dd {labels.names[word(img, p)]}'); p += 4; continue
        nxt = min([x for x in stops if x > p] + [end])
        if p >= len(img):
            S(f'    db {nxt - p} dup(?)')
        else:
            nxt = min(nxt, len(img))
            S('; UNKNOWN: not yet classified as code or data')
            for row in db_rows(img[p:nxt]): S(row)
        p = nxt

SYM = r'(?:sub|loc|unused|tbl|dtbl|byte|word|dword|cvar)_[0-9A-F]{4}'
LABEL_RE = re.compile(r'\b(' + SYM + r')\b')

def generate(img, insns, labels, explicit):
    bodies = []
    for name, start, end, first in MODULES:
        body = Source(name); S = body.add
        S('CODE segment para public \'CODE\'' if first else 'CODE segment word public \'CODE\'')
        S('assume cs:CODE, ds:DATA, es:nothing, ss:nothing')
        emit_code(S, img, insns, labels, explicit, start, end)
        S('CODE ends')
        S('DATA segment word public \'DATA\'')
        if first: emit_data(S, img, labels)
        S('DATA ends')
        if first: S('end sub_0008')
        else: S('end')
        bodies.append(body)
    # cross-module symbols
    defined = [set(re.findall(r'^(' + SYM + r')(?::| label)', '\n'.join(b.lines), re.M)) for b in bodies]
    used = [set(LABEL_RE.findall('\n'.join(b.lines))) for b in bodies]
    datanames = {v: DKIND[k] for k, v in DNAMES.items()} | {f'dtbl_{t:04X}': 'word' for sg, t, n, _ in CODE_TABLES if sg == 'ds'}
    out = []
    for k, (name, start, end, first) in enumerate(MODULES):
        src = Source(name); S = src.add
        exported = sorted(defined[k] & set().union(*(used[j] for j in range(len(bodies)) if j != k)))
        imported = sorted((used[k] - defined[k]))
        S(f'; Prehistorik 2 (PRE2.EXE) {name} - bootstrap reconstruction generated by tools/gen.py')
        if first:
            S('STK segment para stack \'STACK\'')
            S(f'    db {h(CODE_START)} dup(0)')
            S('STK ends')
        for x in exported:
            if x in datanames: S(f'public {x}')
        if not first:      # a DATA block before CODE gives the externs their segment
            S('DATA segment word public \'DATA\'')
            for x in imported:
                if x in datanames: S(f'extrn {x}:{datanames[x]}')
            S('DATA ends')
        body = bodies[k]
        for i, line in enumerate(body.lines, 1):
            S(line, body.spans.get(i))
            if i == 2:      # after the CODE segment / assume header
                for x in exported:
                    if x not in datanames: S(f'public {x}')
                for x in imported:
                    if x not in datanames: S(f'extrn {x}:{"word" if x.startswith(("tbl_", "cvar_")) else "near"}')
        out.append(src)
    return out

def parse_lst(text, src):
    """1-based source line number -> segment offset, for lines whose row shows one."""
    import lst
    return {n: off for n, off in enumerate(lst.parse(text, src.lines, src.name), 1) if off is not None}

def build(srcs, out=OUT):
    out.mkdir(parents=True, exist_ok=True)
    lsts = []
    for src in srcs:
        (out/f'{src.name}.ASM').write_text('\n'.join(src.lines) + '\n', encoding='latin-1')
        (out/f'{src.name}.OBJ').unlink(missing_ok=True)
        dos('TASM.EXE', [f'{src.name}.ASM,{src.name}.OBJ,{src.name}.LST'], out)
        lsts.append(parse_lst((out/f'{src.name}.LST').read_text(encoding='latin-1'), src))
    (out/'PRE2.EXE').unlink(missing_ok=True)
    (out/'LINK.RSP').write_text('+\n'.join(s.name + '.OBJ' for s in srcs) + '\nPRE2.EXE\nPRE2.MAP\n')
    log = dos('TLINK.EXE', ['@LINK.RSP'], out)
    if not (out/'PRE2.EXE').exists(): raise RuntimeError(log)
    return lsts, (out/'PRE2.EXE').read_bytes()

def main(write=False):
    hdr, img, rel = original()
    insns, xrefs, indirect, calls = explore(img, all_seeds(img))
    labels = Labels()
    for t, srcs in xrefs.items(): labels.want(t, 'loc')
    for t in calls: labels.want(t, 'sub')
    for t in all_seeds(img): labels.want(t, 'sub')
    for t in DEAD_CODE:
        if not xrefs.get(t): labels.names[t] = f'unused_{t:04X}'
    from segflow import segflow, summaries, table_call_sites
    from hints import CODE_SEEDS
    summaries(img, insns, sorted(calls))
    states = segflow(img, insns, list(CODE_SEEDS) + DEAD_CODE, table_call_sites(img, insns, CODE_TABLES))
    refs, dsizes, csizes = data_refs(insns, states)
    DREFS.update(refs)
    covered = set()
    for o, ins in insns.items(): covered.update(range(o, o + ins.size))
    ds_table_offs = {t for sg, t, n, _ in CODE_TABLES if sg == 'ds'}
    for d, sz in dsizes.items():
        if d in ds_table_offs: DNAMES[d] = f'dtbl_{d:04X}'; DKIND[d] = 'word'; continue
        DKIND[d] = size_name(sz); DNAMES[d] = f'{DKIND[d]}_{d:04X}'
    cs_table_offs = {t for sg, t, n, _ in CODE_TABLES if sg == 'cs'}
    for c, sz in csizes.items():
        if c in covered: continue          # operand inside an instruction (self-modifying): stays numeric
        if c in cs_table_offs: CNAMES[c] = f'tbl_{c:04X}'; continue
        CKIND[c] = size_name(sz); CNAMES[c] = f'cvar_{c:04X}'
    explicit = {}
    for it in range(12):
        srcs = generate(img, insns, labels, explicit)
        lsts, exe = build(srcs)
        bad = set()
        for src, lst in zip(srcs, lsts):
            lines = sorted(lst)
            for k, ln in enumerate(lines[:-1]):
                if ln in src.spans and src.spans[ln][1] is not None and ln + 1 in lst:
                    off, size = src.spans[ln]
                    if lst[lines[k + 1]] - lst[ln] != size: bad.add(off)
        if not bad:
            _, built, brel, _ = mz(exe)
            diffs = [i for i in range(min(len(built), len(img))) if built[i] != img[i]]
            for i in diffs:
                if CODE_START <= i < CODE_END:
                    o = i - CODE_START
                    starts = [s for s in insns if s <= o < s + insns[s].size]
                    if starts: bad.add(starts[0])
            if not bad and not diffs and len(built) == len(img):
                print(f'iteration {it}: built {len(built)} bytes exact; '
                      f'{sum(1 for v in explicit.values() if v >= 2)} explicit encodings, '
                      f'{sum(1 for v in explicit.values() if v == 1)} alternative spellings')
                if write:
                    for src in srcs:
                        (ROOT/'src'/f'{src.name}.ASM').write_text('\n'.join(src.lines) + '\n', encoding='latin-1')
                    (ROOT/'src'/'sources.txt').write_text('\n'.join(s.name + '.ASM' for s in srcs) + '\n')
                return srcs, exe
        bad = {o for o in bad if explicit.get(o, 0) < 2}
        if not bad: raise RuntimeError('no progress; first diffs at ' + ' '.join(f'{i:05X}' for i in diffs[:10]))
        print(f'iteration {it}: {len(bad)} lines need another spelling')
        for o in bad: explicit[o] = explicit.get(o, 0) + 1
    raise RuntimeError('did not converge')

if __name__ == '__main__':
    main(write='--write' in sys.argv)
