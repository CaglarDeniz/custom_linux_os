#include "networking.h"

#define BOOTREQUEST 1
#define BOOTREPLY 2
#define DHCP_MAGIC 0x63825363
#define CODE_PAD 0
#define CODE_SUBNET_MASK 1
#define CODE_ROUTER 3
#define CODE_DNS 6
#define CODE_TYPE 53
#define DHCPDISCOVER 1
#define DHCPOFFER 2
#define DHCPREQUEST 3
#define DHCPACK 5
#define CODE_REQUEST 50
#define CODE_LEASE_TIME 51
#define CODE_SID 54
#define CODE_PARAMS 55
#define CODE_END 255
#define DHCP_DEST_PORT 67
#define DHCP_SOURCE_PORT 68
#define DHCP_MAX_LEN 576

static ip_t siaddr;

// TODO
static void write_tlv(uint8_t type, uint8_t len, uint8_t** packet) {
  write_u8(type, packet);
  write_u8(len, packet);
}

// TODO
static void dhcp_discover(uint8_t* packet, uint32_t xid) {
  uint8_t* pointer = packet;
  write_u8(BOOTREQUEST, &pointer);
  write_u8(ARP_ETHERNET, &pointer);
  write_u8(MAC_SIZE, &pointer);
  write_u8(0, &pointer); // Hops
  write_u32(xid, &pointer);
  write_u16(0, &pointer); // Seconds since client began DHCP process
  write_u16(0, &pointer); // flags
  write_struct(ip_t, our_ip, &pointer); // ciaddr
  write_struct(ip_t, zero_ip, &pointer); // yiaddr
  write_struct(ip_t, gateway_ip, &pointer); // siaddr
  write_struct(ip_t, zero_ip, &pointer); // giaddr (relay?)
  write_struct(mac_t, our_mac, &pointer); // chaddr
  pointer += 16 - sizeof(mac_t); // chaddr is 16 bytes
  pointer += 64; // sname
  pointer += 128; // file
  write_u32(DHCP_MAGIC, &pointer); // magic cookie

  write_tlv(CODE_TYPE, 1, &pointer);
  write_u8(DHCPDISCOVER, &pointer);

  write_tlv(CODE_PARAMS, 3, &pointer); // request subnet mask, router, and dns server
  write_u8(CODE_SUBNET_MASK, &pointer);
  write_u8(CODE_ROUTER, &pointer);
  write_u8(CODE_DNS, &pointer);

  write_u8(CODE_END, &pointer);
  udp_send_packet(&broadcast_ip, DHCP_DEST_PORT, DHCP_SOURCE_PORT, packet, pointer - packet);
}

// TODO
static int dhcp_parse_offer(uint8_t* packet, uint32_t xid) {
  uint8_t* pointer = packet;
  if (read_u8(&pointer) != BOOTREPLY) return -1;//not a reply
  if (read_u8(&pointer) != ARP_ETHERNET) return -1;//wrong hardware type
  if (read_u8(&pointer) != MAC_SIZE) return -1;//wrong mac size
  read_u8(&pointer);//hops ... don't care
  if (read_u32(&pointer) != xid) return -1;//Wrong xid
  read_u16(&pointer);//seconds ... dont' care
  read_u16(&pointer);//flags ... don't care (only flag is broadcast, which doesn't matter to us)
  pointer += IP_SIZE;//read_struct(ip_t, &pointer);//ciaddr ... should be 0.0.0.0, but don't care
  our_ip = read_struct(ip_t, &pointer);//yiaddr
  siaddr = read_struct(ip_t, &pointer);//siaddr
  pointer += IP_SIZE;//read_struct(ip_t, &pointer);//giaddr (relay) ... don't care
  pointer += 16; // chaddr ... don't care (we could check, but whatever)
  pointer += 64; // sname ... don't care
  pointer += 128; // file ... don't care (for netbios booting or something)
  if (read_u32(&pointer) != DHCP_MAGIC) return -1; // wrong cookie
  while (1) {
    switch (read_u8(&pointer)) {
      case CODE_SUBNET_MASK:
        if (read_u8(&pointer) != IP_SIZE) return -1; // wrong size
        subnet_mask = read_struct(ip_t, &pointer);
        break;
      case CODE_ROUTER:
        if (read_u8(&pointer) != IP_SIZE) return -1; // wrong size (actually could be more, but not in qemu)
        gateway_ip = read_struct(ip_t, &pointer);
        break;
      case CODE_DNS:
        if (read_u8(&pointer) != IP_SIZE) return -1; // wrong size (actually could be more, but not in qemu)
        dns_ip = read_struct(ip_t, &pointer);
        break;
      case CODE_TYPE:
        if (read_u8(&pointer) != 1) return -1; // should be 1
        if (read_u8(&pointer) != DHCPOFFER) return -1;
        break;
      case CODE_SID:
        if (read_u8(&pointer) != IP_SIZE) return -1; // wrong size
        siaddr = read_struct(ip_t, &pointer);//probably incorrect... whatever
        break;
      case CODE_LEASE_TIME:
        if (read_u8(&pointer) != sizeof(uint32_t)) return -1; // wrong size
        read_u32(&pointer); // lease time
        break;
      case CODE_END:
        return 0;
      case CODE_PAD:
        break;
      default:
        pointer += read_u8(&pointer); // skip length bytes
    }
  }
}

// TODO
void dhcp_request(uint8_t* packet, uint32_t xid) {
  uint8_t* pointer = packet;
  write_u8(BOOTREQUEST, &pointer);
  write_u8(ARP_ETHERNET, &pointer);
  write_u8(MAC_SIZE, &pointer);
  write_u8(0, &pointer); // Hops
  write_u32(xid, &pointer);
  write_u16(0, &pointer); // Seconds since client began DHCP process
  write_u16(0, &pointer); // flags
  write_struct(ip_t, our_ip, &pointer); // ciaddr
  write_struct(ip_t, zero_ip, &pointer); // yiaddr
  write_struct(ip_t, siaddr, &pointer); // siaddr
  write_struct(ip_t, zero_ip, &pointer); // giaddr (relay?)
  write_struct(mac_t, our_mac, &pointer); // chaddr
  pointer += 16 - sizeof(mac_t); // chaddr is 16 bytes
  pointer += 64; // sname
  pointer += 128; // file
  write_u32(DHCP_MAGIC, &pointer); // magic cookie

  write_tlv(CODE_TYPE, 1, &pointer);
  write_u8(DHCPREQUEST, &pointer);

  write_tlv(CODE_REQUEST, IP_SIZE, &pointer);
  write_struct(ip_t, our_ip, &pointer);

  write_tlv(CODE_SID, IP_SIZE, &pointer);
  write_struct(ip_t, siaddr, &pointer);

  write_u8(CODE_END, &pointer);
  udp_send_packet(&broadcast_ip, DHCP_DEST_PORT, DHCP_SOURCE_PORT, packet, pointer - packet);
}

// TODO
int dhcp_init(void) {
  uint8_t packet[DHCP_MAX_LEN];// c.f. rfc2131
  uint32_t xid = 0x26a08845; // XKCD random
  int i;
  i = udp_recv_start(DHCP_SOURCE_PORT, packet, DHCP_MAX_LEN);
  dhcp_discover(packet, xid);
  udp_recv_join(i);
  if (dhcp_parse_offer(packet, xid)) {
    die("How did this happen?");
  }
  dhcp_request(packet,xid);
  return 0;
}
