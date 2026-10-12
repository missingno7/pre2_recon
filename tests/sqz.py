"""SQZ resource loader (sub_107B) vs c/sqz.c on every shipped SQZ and TRK file.

State: memory top A000h paragraphs (640 KiB, cs:cvar_0006), free paragraph pointer
right after the program image, the file name written at ds:000C (the KEYB.SQZ name
slot). Compared: DATA, code-segment variables, the decoded output and the work area
after it (both sides leave it in memory), AX = output segment, and the DOS requests."""
from difftest import Case
from common import ROOT

FREE = 0x2530          # first paragraph after the loaded image (1010h + 151Bh + slack)
NAME = 0x000C

def make(name):
    def setup(m):
        m.ww(m.code('ConventionalTopSeg'), 0xA000, seg=0x1030)
        m.ww(m.data('NextFreeSegment'), FREE)
        m.write(0x1A0F, NAME, name.encode() + b'\0')
    return Case(name, 'LoadSqzFile', 'c_LoadSqzFile', regs=dict(DX=NAME), setup=setup, out_regs=('AX',),
                ranges=[(FREE, 0, 0xA000 - FREE - 0x10 and 0x10000)] + [(FREE + 0x1000*k, 0, 0x10000) for k in range(1, 7)],
                files=True)

CASES = [make(p.name) for p in sorted((ROOT/'assets').iterdir()) if p.suffix.upper() in ('.SQZ', '.TRK') and p.name != 'PRE2.SQZ']
