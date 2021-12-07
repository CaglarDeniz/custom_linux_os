#include "networking.h"

#define DNS_LENGTH 512
#define DNS_RECURSIVE 0x100
#define DNS_QUERY 0
#define DNS_RESPONSE 0x8000
#define DNS_RECURSIVE_ALLOWED 0x80
#define TYPE_A 0x0001
#define TYPE_CNAME 0x0005
#define CLASS_IN 0x0001

#define COMPRESSED 0xc0

#define PORT_OFFSET 0x8000
#define DNS_PORT 53

static uint16_t xid_base = 0x1234;

// TODO
static void write_name(uint8_t* name, uint8_t** pointer) {
  int i = 0, j = 0;
  uint8_t c;
  while (1) {
    switch (c = name[i]) {
      case '.':
        (*pointer)[j] = i - j;
        j = ++i;
        break;
      case '\0':
        (*pointer)[j] = i - j;
        j = ++i;
        (*pointer)[j] = 0;
        *pointer += j+1;
        return;
      default:
        ++i;
        (*pointer)[i] = c;
        break;
    }
  }
}

// TODO
static void skip_name(uint8_t** pointer) {
  uint8_t c;
  while ((c = read_u8(pointer))) {
    if ((c & COMPRESSED) == COMPRESSED) {
      read_u8(pointer);
      return;
    }
    *pointer += c;
  }
}

// TODO
static void dns_request(uint8_t* packet, uint8_t* name, uint16_t xid) {
  uint8_t* pointer = packet;
  write_u16(xid, &pointer); // id
  write_u16(DNS_QUERY | DNS_RECURSIVE, &pointer); // flags (only use recursive)
  write_u16(1, &pointer); // # questions
  write_u16(0, &pointer); // # answers
  write_u16(0, &pointer); // # authority RRs
  write_u16(0, &pointer); // # additional RRs
  write_name(name, &pointer); // # name
  write_u16(TYPE_A, &pointer); // # type
  write_u16(CLASS_IN, &pointer); // # class
  udp_send_packet(&dns_ip, DNS_PORT, PORT_OFFSET + xid, packet, pointer - packet);
}

// TODO
static int dns_parse(uint8_t* packet, ip_t* ip, uint16_t xid) {
  uint8_t* pointer = packet;
  int i;
  if (read_u16(&pointer) != xid) return -1; // really unlucky?
  uint16_t mask = DNS_RECURSIVE_ALLOWED | DNS_RECURSIVE | DNS_RESPONSE;
  if ((read_u16(&pointer) & mask) != mask) return -1; // wrong flags
  uint16_t questions = read_u16(&pointer); // should be 1
  uint16_t answers = read_u16(&pointer);
  if (answers == 0) return -1; // no answers
  read_u16(&pointer); // # authority RRs
  read_u16(&pointer); // # additional RRs
  for (i = 0; i < questions; ++i) {
    skip_name(&pointer);
    if (read_u16(&pointer) != TYPE_A) return -1; // wrong type
    if (read_u16(&pointer) != CLASS_IN) return -1; // wrong class
  }
  for (i = 0; i < answers; ++i) {
    skip_name(&pointer);
    uint16_t type = read_u16(&pointer);
    uint16_t class = read_u16(&pointer);
    if (class != CLASS_IN) return -1; // wrong class
    read_u32(&pointer); // TTL ... we don't cache anything anyways
    switch (type) {
      case TYPE_A:
        if (read_u16(&pointer) != IP_SIZE) return -1; // data should be just an IP address
        *ip = read_struct(ip_t, &pointer);
        return 0;
      case TYPE_CNAME:
        pointer += read_u16(&pointer); // length of alias
        break;
      default:
        return -1; // unknown type
    }
  }
  return -1; // no TYPE_A answer found
}

// TODO
int dns_query(uint8_t* name, ip_t* ip) {
  uint8_t packet[DNS_LENGTH];
  uint16_t xid = xid_base;
  xid_base += 1;
  int i = udp_recv_start(PORT_OFFSET + xid, packet, DNS_LENGTH);
  dns_request(packet, name, xid);
  udp_recv_join(i);
  return dns_parse(packet, ip, xid);
}
