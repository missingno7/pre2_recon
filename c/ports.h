/* Port I/O used by the translated code (see ports.c). */
#ifndef PORTS_H
#define PORTS_H
#include <stdint.h>

void    port_out8(uint16_t port, uint8_t value);
void    port_out16(uint16_t port, uint16_t value);   /* OUT DX, AX: low byte to port, high to port+1 */
uint8_t port_in8(uint16_t port);

/* Device callbacks the native host installs (NULL members fall back to the log/preset). */
typedef struct PortDevice {
    void    (*out)(uint16_t port, uint8_t value);
    uint8_t (*in)(uint16_t port);
} PortDevice;
void ports_install(PortDevice *d);
#endif
