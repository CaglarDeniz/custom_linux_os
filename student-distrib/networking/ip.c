#include "networking.h"

// TODO
static uint16_t checksum(uint8_t* packet) {
  uint32_t sum;
  uint32_t i;
  uint16_t n;
  for (i = 0; i < 10; i++) {
    n = read_u16(&packet);
    //Tprintf("0x%x\n", n);
    sum += n;
  }
  while ((i = sum >> 16)) {
    sum = (sum & 0xffff) + i;
  }
  return ~((uint16_t)sum);
}

// TODO
uint32_t ip_write_packet(const ip_t* dest, uint8_t protocol, uint8_t* packet, uint32_t payload_length) {
  mac_t dest_mac;
  if (check_subnet(dest)) { // in our subnet
    if (arp_get(dest, &dest_mac)) return 0; // timeout
  } else {
    if (arp_get(&gateway_ip, &dest_mac)) return 0; // timeout
  }
  uint16_t type = ETHERTYPE_IP4;
  uint8_t *start, *payload, *chksum;
  uint16_t cs;
  start = payload = packet + ETHERNET_HEADER_OFFSET;
  // TODO magic
  write_u8(0x45, &payload);//vihl
  write_u8(0, &payload);//tos
  write_u16(20 + payload_length, &payload);//tlen
  write_u16(0, &payload);//identification
  write_u16(0, &payload);//fragmentation stuff
  write_u8(64, &payload);//ttl (arbitrary?)
  write_u8(protocol, &payload);//protocol
  chksum = payload;
  write_u16(0, &payload);//checksum (come back later)
  write_struct(ip_t, our_ip, &payload);
  write_struct(ip_t, *dest, &payload);
  cs = checksum(start);
  //printf("0x%x\n", cs);
  write_u16(cs, &chksum);
  payload += payload_length;
  ethernet_write_packet(&dest_mac, type, packet, payload - start);
  return payload - packet;
}

// TODO
void ip_parse_packet(uint8_t* packet) {
  // TODO magic
  uint8_t vihl = read_u8(&packet);
  if (vihl != 0x45) {
    // maybe support options later
    return;
  }
  uint8_t tos __attribute__((unused)) = read_u8(&packet);
  uint16_t tlen = read_u16(&packet);
  uint16_t id __attribute__((unused)) = read_u16(&packet);
  uint16_t frag = read_u16(&packet);
  if (frag & (1<<13)) return; // don't deal with fragmentation
  uint8_t ttl __attribute__((unused)) = read_u8(&packet);
  uint8_t protocol = read_u8(&packet);
  uint16_t chksum __attribute__((unused)) = read_u16(&packet);
  ip_t source __attribute__((unused)) = read_struct(ip_t, &packet);
  ip_t dest __attribute__((unused)) = read_struct(ip_t, &packet);
  switch (protocol) {
    case IPTYPE_ICMP:
      break;
    case IPTYPE_TCP:
      tcp_parse_packet(packet, tlen - IP_HEADER_LENGTH);
      break;
    case IPTYPE_UDP:
      udp_parse_packet(packet);
      break;
    default:
      break;
  }
}
