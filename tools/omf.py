"""Minimal OMF reader: segment-relative LEDATA ranges and FIXUPP locations in record order.

Used to study how TASM/TLINK order relocations (docs/executable.md)."""
import struct, sys

def records(data):
    p = 0
    while p < len(data):
        t = data[p]; n = struct.unpack_from('<H', data, p + 1)[0]
        yield t, data[p + 3:p + 2 + n]   # without checksum
        p += 3 + n

def index(b, p):
    if b[p] & 0x80: return ((b[p] & 0x7F) << 8) | b[p + 1], p + 2
    return b[p], p + 1

def fixups(data):
    """Yield (segment index, LEDATA offset, location offset in segment, kind) in record order."""
    seg = base = None
    for t, b in records(data):
        if t == 0xA0:
            seg, p = index(b, 0); base = struct.unpack_from('<H', b, p)[0]
        elif t == 0x9C:
            p = 0
            while p < len(b):
                if b[p] & 0x80:
                    locat = (b[p] << 8) | b[p + 1]; p += 2
                    loc = (locat >> 10) & 0xF; off = locat & 0x3FF
                    fixdat = b[p]; p += 1
                    if not fixdat & 0x80: p = index(b, p)[1] if (fixdat >> 4) & 7 < 3 else p
                    if not fixdat & 0x08: p = index(b, p)[1]
                    if not fixdat & 0x04: p += 2
                    yield seg, base, base + off, loc
                else:
                    p += 1 + (1 if False else 0)
                    # THREAD subrecord: method index follows for frame methods 0..2 / targets
                    thread = b[p - 1]
                    if (thread >> 2) & 7 < 3 or not thread & 0x40: p = index(b, p)[1]

if __name__ == '__main__':
    for seg, base, at, loc in fixups(open(sys.argv[1], 'rb').read()):
        if loc in (2, 3): print(f'seg{seg} rec@{base:04X} {at:04X} loc{loc}')
