#include "networking.h"
#include "../devices/devices.h"

#define ARP_REQUEST 1
#define ARP_RESPONSE 2

typedef struct arp_cache_entry {
  mac_t mac;
  ip_t ip;
} arp_cache_entry_t;

#define ARP_CACHE_SIZE 8
static arp_cache_entry_t arp_cache[8];
static uint32_t arp_cache_len = 0;
static uint32_t arp_cache_counter = 0;

static void arp_send_packet(uint16_t operation, const mac_t *tha, const ip_t *tpa) {
  // TODO magic
  uint8_t packet[64];
  uint8_t* start;
  uint8_t* payload;
  uint32_t size;
  payload = start = packet + ETHERNET_HEADER_OFFSET;
  write_u16(ARP_ETHERNET, &payload); // HTYPE
  write_u16(ETHERTYPE_IP4, &payload); // PTYPE
  write_u8(MAC_SIZE, &payload); //HLEN
  write_u8(IP_SIZE, &payload); //PLEN
  write_u16(ARP_REQUEST, &payload); //OPERATION
  write_struct(mac_t, our_mac, &payload); // our HADDR
  write_struct(ip_t, our_ip, &payload); // our PADDR
  write_struct(mac_t, *tha, &payload); // their HADDR
  write_struct(ip_t, *tpa, &payload); // their PADDR
  size = ethernet_write_packet(tha, ETHERTYPE_ARP, packet, payload - start);
  send_packet(packet, size);
}

// TODO
static void arp_add_cache(mac_t* mac, ip_t* ip) {
  uint32_t i, j;
  for (i = 0; i < arp_cache_len; i++) {
    if (!compare_mac(mac, &(arp_cache[i].mac))) {
      for (j = 0; j < IP_SIZE; j++) {
        arp_cache[i].ip.v[j] = ip->v[j];
      }
      return;
    }
  }
  for (j = 0; j < IP_SIZE; j++) {
    arp_cache[arp_cache_counter].ip.v[j] = ip->v[j];
  }
  for (j = 0; j < MAC_SIZE; j++) {
    arp_cache[arp_cache_counter].mac.v[j] = mac->v[j];
  }
  arp_cache_counter += 1;
  if (arp_cache_len < ARP_CACHE_SIZE) arp_cache_len += 1;
  if (arp_cache_counter == ARP_CACHE_SIZE) arp_cache_counter = 0;
}

// TODO
static int arp_try_cache(const ip_t* ip, mac_t* mac) {
  uint32_t i, j;
  for (i = 0; i < arp_cache_len; i++) {
    if (!compare_ip(ip, &(arp_cache[i].ip))) {
      for (j = 0; j < MAC_SIZE; j++) {
        mac->v[j] = arp_cache[i].mac.v[j];
      }
      return 0;
    }
  }
  return -1;
}

// TODO
int arp_get(const ip_t* ip, mac_t* mac) {
  if (arp_try_cache(ip, mac) == 0) { // found in cache
    return 0;
  } else {
    arp_send_packet(ARP_REQUEST, &broadcast_mac, ip);
    uint32_t stop = rtc_wait(1000);// 1 second timeout
    while (rtc_check(stop)) {
      if (arp_try_cache(ip, mac) == 0) return 0;
    }
    return -1;
  }
}

//TODO
void arp_parse_packet(uint8_t* packet) {
  // TODO magic
  uint16_t htype = read_u16(&packet);
  if (htype != ARP_ETHERNET) return;
  uint16_t ptype = read_u16(&packet);
  if (ptype != ETHERTYPE_IP4) return;
  uint8_t hlen = read_u8(&packet);
  if (hlen != MAC_SIZE) return;
  uint8_t plen = read_u8(&packet);
  if (plen != IP_SIZE) return;
  uint16_t oper = read_u16(&packet);
  mac_t sha = read_struct(mac_t, &packet);
  ip_t spa = read_struct(ip_t, &packet);
  mac_t tha __attribute__((unused)) = read_struct(mac_t, &packet);
  ip_t tpa __attribute__((unused)) = read_struct(ip_t, &packet); // should be our ip
  switch (oper) {
    case ARP_REQUEST:
      // send back arp response
      arp_send_packet(ARP_RESPONSE, &sha, &spa);
      break;
    case ARP_RESPONSE:
    default:
      break;
  }
  arp_add_cache(&sha, &spa);
}

// TODO
int arp_init(void) {
  arp_add_cache(&broadcast_mac, &broadcast_ip);
  arp_add_cache(&broadcast_mac, &gateway_ip);
  return 0;
}
