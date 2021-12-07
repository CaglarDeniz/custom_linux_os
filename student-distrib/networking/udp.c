#include "networking.h"

typedef struct udp_datagram {
  uint8_t *data;
  uint16_t source_port; // from perspective of sender
  uint16_t dest_port; // so this would be the port you were listening on
  uint16_t n;
  int is_valid; // 0 for not valid
} datagram_t;

#define NUM_PORTS 128
static datagram_t open_ports[NUM_PORTS]; // 0 is empty

// TODO
uint32_t udp_send_packet(ip_t *dest, uint16_t dest_port, uint16_t source_port, const uint8_t *data, uint16_t len) {
  uint8_t packet[1518];// TODO magic (also seems a little big for the stack)
  uint8_t *start, *payload;
  start = payload = packet + IP_HEADER_OFFSET;
  write_u16(source_port, &payload);
  write_u16(dest_port, &payload);
  write_u16(len + UDP_HEADER_LENGTH, &payload);
  write_u16(0, &payload); // UDP checksum (optional) TODO checksum?
  memcpy(payload, data, len);
  payload += len;
  uint32_t size = ip_write_packet(dest, IPTYPE_UDP, packet, payload - start);
  if (size == 0) { // probably couldn't find mac address
    return 0;
  }
  send_packet(packet, size);
  return size;
}

// TODO
static int is_open(uint16_t port) {
  int i;
  for (i = 0; i < NUM_PORTS; i++) {
    if (open_ports[i].is_valid && (open_ports[i].dest_port == port)) return 0;
  }
  return 1;
}

// TODO
static int add_listener(uint16_t port, uint8_t* data, uint16_t n) {
  int i;
  datagram_t* d;
  for(i = 0; i < NUM_PORTS; i++) {
    if (!open_ports[i].is_valid) {
      d = &open_ports[i];
      d->data = data;
      d->dest_port = port;
      d->n = n;
      d->is_valid = 1;
      return i;
    }
  }
  return -1;
}

// TODO
int udp_recv_start(uint16_t port, uint8_t *data, uint16_t n) {
  // race condition?
  if (!is_open(port)) return 0; // maybe wait?
  return add_listener(port, data, n);
}

// TODO
uint16_t udp_recv_join(int i) {
  datagram_t* d = &open_ports[i];
  while ((volatile int)d->is_valid);
  return d->source_port;
}

// TODO
uint16_t udp_recv(uint16_t port, uint8_t *data, uint16_t n) {
  return udp_recv_join(udp_recv_start(port, data, n));
}

// TODO
void udp_parse_packet(uint8_t* packet) {
  uint16_t source_port = read_u16(&packet);
  uint16_t dest_port = read_u16(&packet);
  if (is_open(dest_port)) return; // ignore
  uint16_t len = read_u16(&packet) - UDP_HEADER_LENGTH;
  uint16_t chksum __attribute__((unused)) = read_u16(&packet);
  int i;
  for (i = 0; i < NUM_PORTS; i++) {
    datagram_t *d = &open_ports[i];
    if (d->is_valid && (d->dest_port == dest_port)) {
      d->source_port = source_port;
      if (d->n < len) len = d->n;
      memcpy(d->data, packet, len); // drop rest of packet (maybe when we have malloc...)
      d->is_valid = 0;
      return;
    }
  }
}
