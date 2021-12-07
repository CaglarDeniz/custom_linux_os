#ifndef RTL8139_H
#define RTL8139_H
#include "../lib.h"

int rtl8139_init(void);
int send_packet(uint8_t* packet, uint32_t size);

#endif /* RTL8139_H */
