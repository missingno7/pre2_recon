"""Address -> source line, from the last build's listings and TLINK map.

    python tools/where.py 107B          code offset (frame 0020, the cs: offsets in the source)
    python tools/where.py ds:2875       data offset (frame 09FF, DS for all game code)
    python tools/where.py lin:0A3F0     linear program-image offset
    --disasm N   also disassemble N instructions of the original from that address
"""
from common import *
from image import CODE_START, DATA_FRAME, original
import re, sys

BUILD = ROOT/'build'/'asm'

def parts():
    """(linear start, segment name, module file) for every module contribution."""
    out = []
    for m in re.finditer(r'^([0-9A-F]{4}):([0-9A-F]{4}) ([0-9A-F]{4}) C=\w+ S=(\w+) .*M=(\w+\.ASM)',
                         (BUILD/'PRE2.MAP').read_text(), re.M):
        out.append((int(m.group(1), 16)*16 + int(m.group(2), 16), m.group(4), m.group(5)))
    return out

def rows():
    """Every listing row with an offset: (linear, file, source line number, text)."""
    starts = {(seg, mod): lin for lin, seg, mod in parts()}
    out = []
    import lst
    for path in sorted(BUILD.glob('*.LST')):
        mod = path.stem + '.ASM'
        src = (ROOT/'src'/mod).read_text(encoding='latin-1').splitlines()
        offs = lst.parse(path.read_text(encoding='latin-1'), src, mod)
        seg = None
        for n, (text, off) in enumerate(zip(src, offs), 1):
            m = re.match(r'^(\w+) segment\b', text.strip(), re.I)
            if m: seg = m.group(1).upper(); continue
            if re.match(r'^\w+ ends\b', text.strip(), re.I): seg = None; continue
            if off is not None and seg and (seg, mod) in starts:
                out.append((starts[(seg, mod)] + off, mod, n, text.rstrip()))
    return sorted(out)

def parse(arg):
    a = arg.lower()
    if a.startswith('ds:'): return DATA_FRAME*16 + int(a[3:], 16)
    if a.startswith('lin:'): return int(a[4:], 16)
    if a.startswith('cs:'): a = a[3:]
    return CODE_START + int(a, 16)

def main():
    args = sys.argv[1:]
    n = 0
    if '--disasm' in args:
        k = args.index('--disasm'); n = int(args[k + 1]); del args[k:k + 2]
    table = rows()
    for arg in args:
        lin = parse(arg)
        best = [r for r in table if r[0] <= lin]
        if not best: print(f'{arg}: before the first listed row'); continue
        r = best[-1]
        same = [x for x in best if x[0] == r[0]]
        print(f'{arg} = lin {lin:05X} = cs:{lin - CODE_START:04X} = ds:{lin - DATA_FRAME*16:04X}  ->  src/{r[1]}:{same[0][2]}  {r[3].strip()}')
        if n:
            from explore import decode
            img = original()[1]; off = lin - CODE_START
            for _ in range(n):
                ins = decode(img, off)
                if ins is None: break
                print(f'    {off:04X}: {ins.bytes.hex():14} {ins.mnemonic} {ins.op_str}'); off += ins.size

if __name__ == '__main__':
    main()
