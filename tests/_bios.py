"""Oracle side of INT 10h AH=00h (video mode set), shared by every suite's INT 10h handler.

The VGA part is the shared model's vga_bios_mode_set (c/vga_bios.c: register tables,
default palette, memory clear unless AL bit 7); the BIOS data area fields are written into
the oracle's memory exactly as c/platform.c bios_set_mode writes them into the C side's."""

def set_mode(m, al):
    mode = al & 0x7F
    cols, page, rows, charh = 40, 0x2000, 24, 8
    if mode == 0x03: cols, page, charh = 80, 0x1000, 16
    elif mode == 0x12: cols, page, rows, charh = 80, 0xA000, 29, 16
    elif mode == 0x13: page = 0xFA00
    m.vga.vga_bios_mode_set(al)
    m.wb(0x49, mode, seg=0x40)
    m.ww(0x4A, cols, seg=0x40)
    m.ww(0x4C, page, seg=0x40)
    m.ww(0x4E, 0, seg=0x40)
    m.write(0x40, 0x50, bytes(16))
    m.wb(0x62, 0, seg=0x40)
    m.ww(0x63, 0x03D4, seg=0x40)
    m.wb(0x84, rows, seg=0x40)
    m.ww(0x85, charh, seg=0x40)
    m.wb(0x87, (m.rb(0x87, seg=0x40) & 0x7F) | (al & 0x80), seg=0x40)
