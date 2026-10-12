"""The original program image: MZ parsing and the fixed layout constants.

Image offsets are linear offsets into the MZ load module (file offset - header size).
`segment:offset` pairs are relative to the DOS load segment, as in the MZ header.
"""
from common import *
import struct

ASSET = ROOT/'assets'/'PRE2.EXE'

# Address frames proven by the relocation table and the entry code (docs/executable.md).
STACK_FRAME = 0x0000      # SS at entry; 200h bytes
CODE_FRAME = 0x0020       # the only code segment; entry 0020:0008
DATA_FRAME = 0x09FF       # DS for all game code; the data segment starts at 09FF:000C
CODE_START = CODE_FRAME*16
DATA_START = 0x9FFC       # linear: first byte after the code segment (word aligned)
CODE_END = DATA_START

def mz(data):
    """Return (header dict, load module, relocation list [(seg, off)], appended bytes)."""
    if data[:2] != b'MZ' or len(data) < 28: raise ValueError('Not MZ')
    names = 'magic last_page pages relocations header_paragraphs min_extra max_extra ss sp checksum ip cs relocation_offset overlay_number'.split()
    h = dict(zip(names, struct.unpack_from('<14H', data)))
    end = (h['pages'] - 1)*512 + (h['last_page'] or 512)
    start = h['header_paragraphs']*16
    if not 28 <= start <= end <= len(data): raise ValueError('Invalid MZ bounds')
    h.update(header_bytes=start, executable_bytes=end, appended_bytes=len(data) - end)
    rel = [struct.unpack_from('<HH', data, h['relocation_offset'] + 4*n)[::-1] for n in range(h['relocations'])]
    return h, data[start:end], rel, data[end:]

_cache = None
def original():
    """(header, image, relocation linear offsets in table order)."""
    global _cache
    if _cache is None:
        h, img, rel, extra = mz(ASSET.read_bytes())
        _cache = (h, img, [s*16 + o for s, o in rel])
    return _cache

def word(data, p): return struct.unpack_from('<H', data, p)[0]
