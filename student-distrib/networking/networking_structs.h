#ifndef NETWORKING_STRUCTS_H
#define NETWORKING_STRUCTS_H

#include "../lib.h"

#define MAC_SIZE 6

typedef struct mac {
  uint8_t v[MAC_SIZE];
} mac_t;

#define IP_SIZE 4

typedef struct ip {
  uint8_t v[IP_SIZE];
} ip_t;

#endif /* NETWORKING_STRUCTS_H */
