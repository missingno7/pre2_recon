/* Port I/O for the translated code: VGA ports (3C0h..3DAh) go to the VGA model, every
 * other port is recorded in a request log and IN returns a value preset per port (0 by
 * default), mirroring tools/emu.py's PortLog so differential tests compare the traffic.
 * Device models (keyboard controller, PIT, PIC, Sound Blaster) replace the preset values
 * port by port as they are added; the native host installs its own device callbacks. */
#include "pre2.h"
#include "vga.h"
#include "ports.h"
#include <stdio.h>
#include <string.h>

static uint8_t preset[0x400];           /* test/host configuration, not game state */
static char logbuf[1 << 20];          /* joystick calibration polls the port ~10k times */
static size_t loglen;
static PortDevice *devices;

EXPORT void pre2_ports_reset(void) { memset(preset, 0, sizeof preset); loglen = 0; logbuf[0] = 0; }
EXPORT void pre2_ports_preset(uint16_t port, uint8_t value) { if (port < 0x400) preset[port] = value; }
EXPORT const char *pre2_ports_log(void) { return logbuf; }
void ports_install(PortDevice *d) { devices = d; }

static void logline(const char *s)
{
    size_t n = strlen(s);
    if (loglen && loglen + 1 < sizeof logbuf) logbuf[loglen++] = ';';
    if (loglen + n < sizeof logbuf) { memcpy(logbuf + loglen, s, n); loglen += n; }
    logbuf[loglen] = 0;
}

void port_out8(uint16_t port, uint8_t value)
{
    if (vga_port(port)) { vga_out(port, value); return; }
    char line[40]; snprintf(line, sizeof line, "out,%u,1,%u", port, value); logline(line);
    if (devices && devices->out) devices->out(port, value);
}

void port_out16(uint16_t port, uint16_t value)
{
    if (vga_port(port)) { vga_out(port, (uint8_t)value); vga_out((uint16_t)(port + 1), (uint8_t)(value >> 8)); return; }
    char line[40]; snprintf(line, sizeof line, "out,%u,2,%u", port, value); logline(line);
    if (devices && devices->out) { devices->out(port, (uint8_t)value); devices->out((uint16_t)(port + 1), (uint8_t)(value >> 8)); }
}

uint8_t port_in8(uint16_t port)
{
    if (vga_port(port)) return vga_in(port);
    char line[40]; snprintf(line, sizeof line, "in,%u,1", port); logline(line);
    if (devices && devices->in) return devices->in(port);
    return port < 0x400 ? preset[port] : 0;
}
