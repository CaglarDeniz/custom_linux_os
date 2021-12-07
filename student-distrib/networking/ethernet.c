#include "networking.h"

// TODO
uint32_t ethernet_write_packet(const mac_t* dest, uint16_t type, uint8_t* payload, uint32_t payload_length) {
  // payload should have ETHERNET_HEADER_OFFSET space at the front
  // payload_length is just the length of the actual payload
  uint8_t *packet;
  packet = payload;
  write_struct(mac_t, *dest, &packet);
  write_struct(mac_t, our_mac, &packet);
  write_u16(type, &packet);
  packet += payload_length;
  return packet - payload; // network card pads and writes fcs
}

// TODO
void ethernet_parse_packet(uint8_t* packet) {
  mac_t dest __attribute__((unused)) = read_struct(mac_t, &packet);
  mac_t src __attribute__((unused)) = read_struct(mac_t, &packet);
  uint16_t type = read_u16(&packet);
  // TODO magic
  switch (type) {
    case ETHERTYPE_IP4:
      ip_parse_packet(packet);
      break;
    case ETHERTYPE_ARP:
      arp_parse_packet(packet);
      break;
    default:
      // shouldn't happen?
      // most things should be going through an IP packet
      break;
  }
}

/*

maybe start and finish?

so start sets up header and returns where to put payload
finish finishes header, maybe adds footer and returns length

so arp would call ip start
which would call ethernet start
which writes some header
and then ip adds it's header
and then arp does it's header + payload
and then it asks ip to finish

maybe not


let's just copy stuff

so

*/
