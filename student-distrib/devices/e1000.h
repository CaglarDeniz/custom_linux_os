#ifndef E1000_H
#define E1000_H

#include "../lib.h"

int e1000_init(void);
int e1000_send_packet(uint8_t* packet, uint32_t size);
uint32_t in_long_e1000(uint32_t port);

#endif /* E1000_H */
