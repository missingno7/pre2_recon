"""The oracle boundary: facts extracted from the original executable.

PRE2.EXE is an unpacked MZ program linked by TLINK 2.0 (header bytes 1C..21 are
01 00 FB 20 'rj'). Its load module is the reconstructed program image. The shipped
game stores it as PRE2.SQZ, an LZW container that its loader expands; unpack_sqz_exe
reproduces PRE2.EXE from PRE2.SQZ byte for byte (checked by tools/verify.py).
"""
from common import *
from image import mz, ASSET

def manifest(data):
    h, image, rel, extra = mz(data)
    return {
        'asset': ASSET.name,
        'asset_sha256': sha(data),
        'mz': {k: h[k] for k in sorted(h)},
        'header_signature': data[0x1C:0x22].hex(),
        'program_bytes': len(image),
        'program_sha256': sha(image),
        'entry': f"{h['cs']:04X}:{h['ip']:04X}",
        'stack': f"{h['ss']:04X}:{h['sp']:04X}",
        'relocations': [f'{s:04X}:{o:04X}' for s, o in rel],
    }

def unpack_sqz_exe(data):
    """Expand the LZW container that holds PRE2.EXE.

    Header: byte 0 = size bits 16..23, byte 1 = 10h (LZW), bytes 2..3 = size bits 0..15.
    Codes are MSB-first, 9..12 bits, read through a 24-bit window; 101h clears the
    dictionary, 100h ends the stream (this container's variant; the game's own SQZ
    LZW uses the opposite assignment)."""
    size = data[0] << 16 | data[2] | data[3] << 8
    if data[1] != 0x10: raise ValueError('not the LZW container')
    src = iter(data[4:])
    get = lambda: next(src, 0)
    CLEAR, END, FIRST = 0x101, 0x100, 0x102
    prefix, suffix, first = [], [], []
    nbit, window, bitpos, prev = 9, get() << 16 | get() << 8 | get(), 0, CLEAR
    out = bytearray()
    while prev != END:
        if prev == CLEAR:
            nbit = 9; prefix.clear(); suffix.clear(); first.clear()
        bitpos += nbit
        code = (window >> (24 - bitpos)) & ((1 << nbit) - 1)
        window = (window << 8 | get()) & 0xFFFFFF
        if bitpos >= 16: window = (window << 8 | get()) & 0xFFFFFF
        bitpos &= 7
        if code not in (CLEAR, END):
            n = FIRST + len(prefix)
            head = lambda c: c if c < FIRST else first[c - FIRST]
            if code < n: new = head(code)
            elif code == n and prev != CLEAR: new = head(prev)
            else: raise ValueError('invalid LZW code')
            if prev != CLEAR and n < 0x1000:
                prefix.append(prev); suffix.append(new); first.append(head(prev))
                if n + 1 == 1 << nbit and nbit < 12: nbit += 1
            seq, c = bytearray(), code
            while c >= FIRST: seq.append(suffix[c - FIRST]); c = prefix[c - FIRST]
            seq.append(c)
            out += seq[::-1]
        prev = code
    if len(out) != size: raise ValueError(f'LZW size {len(out)} != header {size}')
    return bytes(out)

def extract():
    return manifest(ASSET.read_bytes())

if __name__ == '__main__':
    import sys
    if '--write' in sys.argv: write_json(ROOT/'metadata'/'oracle.json', extract())
    else: print(json.dumps(extract(), indent=1)[:2000])
