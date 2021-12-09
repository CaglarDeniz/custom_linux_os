#ifndef NETWORKING_H
#define NETWORKING_H

#include "networking_structs.h"

extern mac_t our_mac;
extern ip_t our_ip;
extern mac_t broadcast_mac;
extern ip_t broadcast_ip;
extern ip_t gateway_ip;
extern ip_t dns_ip;
extern ip_t subnet_mask;
extern ip_t zero_ip;
extern int (*send_packet) (uint8_t*, uint32_t);

/* In networking.c */
int init_networking(void);
int print_mac(mac_t* mac);
int print_ip(ip_t* ip);
int compare_mac(const mac_t* a, const mac_t* b);
int compare_ip(const ip_t* a, const ip_t* b);
void parse_packet(uint8_t* packet);
int check_subnet(const ip_t *ip);
int parse_ip(int8_t* s, ip_t *ip);

void write_u8(uint8_t d, uint8_t** location);
void write_u16(uint16_t d, uint8_t** location);
void write_u32(uint32_t d, uint8_t** location);
#define write_struct(T, s, p) *(T*)(*(p))=s;*(p)+=sizeof(T)

uint8_t read_u8(uint8_t** location);
uint16_t read_u16(uint8_t** location);
uint32_t read_u32(uint8_t** location);
void* read_n(uint32_t n, uint8_t** location);
#define read_struct(T, p) *(T*)read_n(sizeof(T), p)

/* In ethernet.c */
uint32_t ethernet_write_packet(const mac_t* dest, uint16_t type, uint8_t* payload, uint32_t payload_length);
void ethernet_parse_packet(uint8_t* packet);

#define ETHERNET_HEADER_LENGTH 14
#define ETHERNET_HEADER_OFFSET 14
#define ETHERTYPE_IP4 0x0800
#define ETHERTYPE_ARP 0x0806

/* In arp.c */
int arp_get(const ip_t* ip, mac_t* mac);
void arp_parse_packet(uint8_t* packet);
int arp_init(void);
#define ARP_ETHERNET 0x01

/* In ip.c */
uint32_t ip_write_packet(const ip_t* dest, uint8_t protocol, uint8_t* packet, uint32_t payload_length);
void ip_parse_packet(uint8_t* packet);
uint16_t checksum(uint8_t* packet, uint32_t len);

#define IP_HEADER_LENGTH 20
#define IP_HEADER_OFFSET (ETHERNET_HEADER_LENGTH + IP_HEADER_LENGTH)
#define IPTYPE_ICMP 1
#define IPTYPE_TCP 6
#define IPTYPE_UDP 17

/* In udp.c */
uint32_t udp_send_packet(ip_t *dest, uint16_t dest_port, uint16_t source_port, const uint8_t *data, uint16_t len);
void udp_parse_packet(uint8_t* packet);
uint16_t udp_recv(uint16_t port, uint8_t *data, uint16_t n);
int udp_recv_start(uint16_t port, uint8_t *data, uint16_t n);
uint16_t udp_recv_join(int i);

#define UDP_HEADER_LENGTH 8
#define UDP_HEADER_OFFSET (IP_HEADER_OFFSET + UDP_HEADER_LENGTH)

/* In dhcp.c */
int dhcp_init(void);

/* In dns.c */
int dns_query(uint8_t* name, ip_t* ip);

/* In tcp.c */
void tcp_parse_packet(uint8_t* packet, uint32_t len);
int tcp_connect(int8_t* domain, uint16_t port);
uint32_t tcp_send(uint32_t idx, uint8_t* data, uint32_t len);
uint32_t tcp_recv(uint32_t idx, uint8_t* buffer, uint32_t len);
int tcp_sendall(uint32_t idx, uint8_t* data, uint32_t len);
int tcp_recvall(uint32_t idx, uint8_t* buffer, uint32_t len);

#define TCP_HEADER_LENGTH 20
#define TCP_HEADER_OFFSET (IP_HEADER_OFFSET + UDP_HEADER_LENGTH)
#define TCP_MAX_OPTION_LENGTH 40

#endif /* NETWORKING_H */
