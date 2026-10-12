"""Contact sheet of host frame dumps (build/host/shots/*.bmp) as one PNG, for reviewing
presentation without a viewer: python tools/montage.py OUT.png FILE.bmp ...

Frames are shown at 320x200 (640-wide frames are halved), four per row, with no labels:
the caller prints the file order. Standard library only."""
import struct, sys, zlib

def read_bmp(path):
    d = open(path, 'rb').read()
    w, h = struct.unpack_from('<ii', d, 18)
    h = abs(h)
    px = struct.unpack_from('<%dI' % (w * h), d, 54)
    return w, h, px

def thumb(w, h, px):
    sx = max(1, w // 320); sy = max(1, h // 200)
    out = []
    for y in range(200):
        row = []
        for x in range(320):
            sxp, syp = x * sx, y * sy
            row.append(px[syp * w + sxp] if sxp < w and syp < h else 0)
        out.append(row)
    return out

def write_png(path, rows):
    raw = b''.join(b'\x00' + b''.join(bytes([(p >> 16) & 255, (p >> 8) & 255, p & 255]) for p in r) for r in rows)
    def chunk(t, b): return struct.pack('>I', len(b)) + t + b + struct.pack('>I', zlib.crc32(t + b) & 0xFFFFFFFF)
    h, w = len(rows), len(rows[0])
    open(path, 'wb').write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)) +
                           chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b''))

if __name__ == '__main__':
    out, files = sys.argv[1], sys.argv[2:]
    cols = 4
    thumbs = [thumb(*read_bmp(f)) for f in files]
    grid_rows = (len(thumbs) + cols - 1) // cols
    canvas = [[0x404040] * (cols * 324) for _ in range(grid_rows * 204)]
    for i, t in enumerate(thumbs):
        ox, oy = (i % cols) * 324, (i // cols) * 204
        for y in range(200):
            canvas[oy + y][ox:ox + 320] = t[y]
    write_png(out, canvas)
