// TODO

#include "networking.h"
#include "../devices/e1000.h"

// TODO magic
mac_t our_mac;
ip_t our_ip = {{0, 0, 0, 0}};
mac_t broadcast_mac = {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};
ip_t broadcast_ip = {{255, 255, 255, 255}};
ip_t zero_ip = {{0, 0, 0, 0}};
ip_t gateway_ip = {{0, 0, 0, 0}};
ip_t dns_ip = {{0, 0, 0, 0}};
ip_t subnet_mask = {{255, 255, 255, 255}};

int (*send_packet) (uint8_t*, uint32_t);

// TODO
int init_networking(void) {
  /*if (rtl8139_init()) {
    printf("Sorry, we currently only support the RTL8139 NIC\n");
    printf("Please add '-device RTL8139' to your qemu command");
    return -1;
  }*/
  if (e1000_init()) return -1;
  if (arp_init()) return -1;
  send_packet = e1000_send_packet;
  printf("Our mac address: ");
  print_mac(&our_mac);
  return 0;
}

// TODO
int print_mac(mac_t* mac) {
  int i;
  printf("%x", mac->v[0]);
  for (i = 1; i < MAC_SIZE; i++) {
    printf(":%x", mac->v[i]);
  }
  printf("\n");
  return 0;
}

// TODO
int print_ip(ip_t* ip) {
  int i;
  printf("%d", ip->v[0]);
  for (i = 1; i < IP_SIZE; i++) {
    printf(".%d", ip->v[i]);
  }
  printf("\n");
  return 0;
}

// TODO
void write_u8(uint8_t d, uint8_t** location) {
  **location = d;
  *location += 1;
}

// TODO
void write_u16(uint16_t d, uint8_t** location) {
  // TODO magic
  write_u8((d>>8)&0xFF, location);
  write_u8(d&0xFF, location);
}

// TODO
void write_u32(uint32_t d, uint8_t** location) {
  // TODO magic
  write_u8((d>>24)&0xFF, location);
  write_u8((d>>16)&0xFF, location);
  write_u8((d>>8)&0xFF, location);
  write_u8((d)&0xFF, location);
}

// TODO
uint8_t read_u8(uint8_t** location) {
  return *((*location)++);
}

// TODO
uint16_t read_u16(uint8_t** location) {
  // TODO magic?????? (it's really no more magic than the magic 8 in uint8_t)
  return ((uint16_t)read_u8(location) << 8) + (uint16_t)read_u8(location);
}

// TODO
uint32_t read_u32(uint8_t** location) {
  // TODO magic??????
  return ((uint32_t)read_u16(location) << 16) + (uint32_t)read_u16(location);
}

// TODO
void* read_n(uint32_t n, uint8_t** location) {
  *location += n;
  return (*location) - n;
}

// TODO
void parse_packet(uint8_t* packet) {
  // is there anything else to do here?
  ethernet_parse_packet(packet);
}

// TODO
int compare_mac(const mac_t* a, const mac_t* b) {
  int i;
  for (i = 0; i < MAC_SIZE; i++) {
    if (a->v[i] != b->v[i]) return -1;
  }
  return 0;
}

// TODO
int compare_ip(const ip_t* a, const ip_t* b) {
  int i;
  for (i = 0; i < IP_SIZE; i++) {
    if (a->v[i] != b->v[i]) return ~i;
  }
  return 0;
}

// TODO
int check_subnet(const ip_t *ip) {
  int i;
  for (i = 0; i < 4; i++) {
    if ((ip->v[i] & subnet_mask.v[i]) != (gateway_ip.v[i] & subnet_mask.v[i])) return 0;
  }
  return 1;
}

// TODO
int parse_ip(int8_t* s, ip_t *ip) {
  int i, n;
  for (i = 0; i < 3; ++i) {
    s += atoi(s, &n);
    if (*s != '.') return -1;
    s += 1;
    if (n < 0 || n > 255) return -1;
    ip->v[i] = n;
  }
  s += atoi(s, &n);
  if (*s != '\0') return -1;
  if (n < 0 || n > 255) return -1;
  ip->v[i] = n;
  return 0;
}

/* TODO

v Network Driver
v Ethernet frame
v ARP
v IP
v UDP
v DHCP (not really necessary since gateway is 10.0.2.2, dns is 10.0.2.3)
v DNS (probably over UDP)
- TCP
- HTTP
- TLS + HTTPS (really ambitious)

*/

