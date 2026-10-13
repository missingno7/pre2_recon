"""Known-answer tests of the VGA model (c/vga.c, c/vga_bios.c) against documented IBM VGA
behaviour. The differential tests run the oracle and the C on one shared model, so they cannot
show that the model itself is right; these expectations come from the VGA register reference
(write modes 0-3, set/reset, rotate, logical function, bit mask, latches, read modes 0/1,
chain-4, DAC cycling, the attribute flip-flop) and the standard BIOS register tables of the
modes the game uses (0Dh, 13h). Run: python tests/_vga_kat.py (not a difftest suite: the underscore keeps difftest from loading it).
"""
import ctypes, sys
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
import cbuild

class Vga(ctypes.Structure):
    _fields_ = [('plane', (ctypes.c_uint8 * 0x10000) * 4), ('latch', ctypes.c_uint8 * 4),
                ('seq_index', ctypes.c_uint8), ('seq', ctypes.c_uint8 * 8),
                ('gc_index', ctypes.c_uint8), ('gc', ctypes.c_uint8 * 16),
                ('crtc_index', ctypes.c_uint8), ('crtc', ctypes.c_uint8 * 32),
                ('attr_index', ctypes.c_uint8), ('attr', ctypes.c_uint8 * 32), ('attr_flipflop', ctypes.c_uint8),
                ('misc_output', ctypes.c_uint8), ('dac', (ctypes.c_uint8 * 3) * 256),
                ('dac_write_index', ctypes.c_uint8), ('dac_read_index', ctypes.c_uint8),
                ('dac_component', ctypes.c_uint8), ('dac_read_component', ctypes.c_uint8),
                ('pel_mask', ctypes.c_uint8), ('status_reads', ctypes.c_uint32), ('retrace_period', ctypes.c_uint8)]

d = ctypes.CDLL(str(cbuild.build(ROOT/'build'/'c'/'pre2core_vgakat.dll')))
d.pre2_vga_state.restype = ctypes.POINTER(Vga)
d.vga_read.restype = ctypes.c_uint8
d.vga_in.restype = ctypes.c_uint8
assert ctypes.sizeof(Vga) == d.pre2_vga_state_size(), 'VgaState layout changed: update tests/vga_kat.py'
V = d.pre2_vga_state().contents
fails = []

def check(name, got, want):
    if got != want: fails.append(f'{name}: got {got!r}, want {want!r}')

def seq(i, v): d.vga_out(0x3C4, i); d.vga_out(0x3C5, v)
def gc(i, v): d.vga_out(0x3CE, i); d.vga_out(0x3CF, v)
def planes(off): return [V.plane[p][off] for p in range(4)]
def fresh():
    d.vga_bios_mode_set(0x0D)
    seq(2, 0x0F)

# write mode 0: map mask, set/reset, enable set/reset, rotate, function, bit mask
fresh(); d.vga_write(0, 0xA5); check('mode 0 replace, all planes', planes(0), [0xA5] * 4)
fresh(); seq(2, 0x05); d.vga_write(1, 0x3C); check('map mask 0101b', planes(1), [0x3C, 0, 0x3C, 0])
fresh(); gc(0, 0x0A); gc(1, 0x0F); d.vga_write(2, 0x00); check('set/reset 1010b, enabled for all', planes(2), [0, 0xFF, 0, 0xFF])
fresh(); gc(0, 0x0F); gc(1, 0x03); d.vga_write(3, 0x55); check('set/reset enabled for planes 0, 1 only', planes(3), [0xFF, 0xFF, 0x55, 0x55])
fresh(); gc(3, 0x01); d.vga_write(4, 0x01); check('rotate right by 1', planes(4), [0x80] * 4)
fresh(); d.vga_write(5, 0xF0); d.vga_read(5); gc(3, 0x08); d.vga_write(5, 0x3C); check('function AND with latches', planes(5), [0x30] * 4)
fresh(); d.vga_write(5, 0xF0); d.vga_read(5); gc(3, 0x10); d.vga_write(5, 0x3C); check('function OR', planes(5), [0xFC] * 4)
fresh(); d.vga_write(5, 0xF0); d.vga_read(5); gc(3, 0x18); d.vga_write(5, 0x3C); check('function XOR', planes(5), [0xCC] * 4)
fresh(); d.vga_write(6, 0xAA); d.vga_read(6); gc(8, 0x0F); d.vga_write(6, 0x55); check('bit mask 0Fh keeps latch bits', planes(6), [0xA5] * 4)
# latches: a read loads all four; write mode 1 stores them
fresh(); seq(2, 1); d.vga_write(7, 0x11); seq(2, 2); d.vga_write(7, 0x22); seq(2, 4); d.vga_write(7, 0x33); seq(2, 8); d.vga_write(7, 0x44)
seq(2, 0x0F); d.vga_read(7); check('latches after read', list(V.latch), [0x11, 0x22, 0x33, 0x44])
gc(5, 0x01); d.vga_write(8, 0x00); check('write mode 1: latch copy', planes(8), [0x11, 0x22, 0x33, 0x44])
# write mode 2: CPU data bits 0-3 are the colour, bit mask applies per pixel
fresh(); gc(5, 0x02); gc(8, 0xFF); d.vga_write(9, 0x05); check('write mode 2 colour 5', planes(9), [0xFF, 0, 0xFF, 0])
fresh(); d.vga_write(10, 0xFF); d.vga_read(10); gc(5, 0x02); gc(8, 0x0F); d.vga_write(10, 0x02); check('write mode 2 with bit mask', planes(10), [0xF0, 0xFF, 0xF0, 0xF0])
# write mode 3: rotated data AND bit mask is the mask, set/reset gives the colour
fresh(); gc(5, 0x03); gc(0, 0x06); gc(8, 0xFF); d.vga_write(11, 0x0F); check('write mode 3', planes(11), [0, 0x0F, 0x0F, 0])
# read mode 0: read map select
fresh(); seq(2, 4); d.vga_write(12, 0x77); seq(2, 0x0F); gc(4, 2); check('read mode 0, map 2', d.vga_read(12), 0x77)
gc(4, 1); check('read mode 0, map 1', d.vga_read(12), 0x00)
# read mode 1: colour compare (GC2) with colour don't care (GC7)
fresh(); gc(5, 0x02); d.vga_write(13, 0x09); d.vga_read(13); gc(8, 0x0F); d.vga_write(13, 0x01); gc(8, 0xFF); gc(5, 0x08); gc(2, 0x09); gc(7, 0x0F)
check('read mode 1 compare colour 9', d.vga_read(13), 0xF0)
gc(7, 0x01); check('read mode 1, only plane 0 compared', d.vga_read(13), 0xFF)
# chain 4 (mode 13h): offset o -> plane (o & 3), offset (o >> 2) in the model's storage
d.vga_bios_mode_set(0x13); d.vga_write(0x1235, 0x5A); check('chain-4 store', V.plane[1][0x1235 >> 2], 0x5A)
check('chain-4 read', d.vga_read(0x1235), 0x5A)
# DAC: write index, three components, auto-increment; read index likewise
d.vga_out(0x3C8, 0x10)
for v in (1, 2, 3, 4, 5, 6): d.vga_out(0x3C9, v)
check('DAC writes cycle and advance', [list(V.dac[0x10]), list(V.dac[0x11])], [[1, 2, 3], [4, 5, 6]])
d.vga_out(0x3C7, 0x11); check('DAC read', [d.vga_in(0x3C9) for _ in range(3)], [4, 5, 6])
# attribute controller: reading 3DAh resets the flip-flop to the index state
d.vga_out(0x3C0, 0x31); d.vga_in(0x3DA); d.vga_out(0x3C0, 0x33); d.vga_out(0x3C0, 0x04)
check('attr flip-flop reset by 3DAh', (V.attr_index, V.attr[0x13]), (0x33, 0x04))

# BIOS mode tables (IBM VGA standard register values)
MODES = {
    0x0D: dict(misc=0x63, seq=[0x03, 0x09, 0x0F, 0x00, 0x06],
               crtc=[0x2D, 0x27, 0x28, 0x90, 0x2B, 0x80, 0xBF, 0x1F, 0x00, 0xC0, 0, 0, 0, 0, 0, 0,
                     0x9C, 0x8E, 0x8F, 0x14, 0x00, 0x96, 0xB9, 0xE3, 0xFF],
               gc=[0, 0, 0, 0, 0, 0x00, 0x05, 0x0F, 0xFF],
               attr=[0, 1, 2, 3, 4, 5, 6, 7, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x01, 0x00, 0x0F, 0x00, 0x00]),
    0x13: dict(misc=0x63, seq=[0x03, 0x01, 0x0F, 0x00, 0x0E],
               crtc=[0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F, 0x00, 0x41, 0, 0, 0, 0, 0, 0,
                     0x9C, 0x8E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3, 0xFF],
               gc=[0, 0, 0, 0, 0, 0x40, 0x05, 0x0F, 0xFF],
               attr=list(range(16)) + [0x41, 0x00, 0x0F, 0x00, 0x00]),
}
for mode, t in MODES.items():
    d.vga_write(0, 0x99)
    d.vga_bios_mode_set(mode)
    check(f'mode {mode:02X}h misc', V.misc_output, t['misc'])
    check(f'mode {mode:02X}h sequencer', list(V.seq[:5]), t['seq'])
    check(f'mode {mode:02X}h CRTC', list(V.crtc[:25]), t['crtc'])
    check(f'mode {mode:02X}h graphics', list(V.gc[:9]), t['gc'])
    check(f'mode {mode:02X}h attribute', list(V.attr[:21]), t['attr'])
    check(f'mode {mode:02X}h clears memory', sum(V.plane[p][0] for p in range(4)), 0)
d.vga_write(0, 0x99); d.vga_bios_mode_set(0x8D)
check('AL bit 7 keeps memory', V.plane[0][0] != 0 or V.plane[1][0] != 0, True)

print('\n'.join(fails) if fails else 'PASS vga_kat: all known-answer checks')
sys.exit(1 if fails else 0)
