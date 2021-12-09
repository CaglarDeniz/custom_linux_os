#include "networking.h"
#include "../devices/devices.h"

#define BUFFER_SIZE 2048
#define MSS (1500-IP_HEADER_LENGTH-TCP_HEADER_LENGTH-TCP_MAX_OPTION_LENGTH)
#define NUM_CONNECTIONS 8

#define TCP_FIN (1<<0)
#define TCP_SYN (1<<1)
#define TCP_RST (1<<2)
#define TCP_PSH (1<<3)
#define TCP_ACK (1<<4)
// whatever about rest

typedef union {
  struct __attribute__((packed)) {
    uint32_t ns          : 1;
    uint32_t reserved    : 3;
    uint32_t data_offset : 4;
    uint32_t fin         : 1;
    uint32_t syn         : 1;
    uint32_t rst         : 1;
    uint32_t psh         : 1;
    uint32_t ack         : 1;
    uint32_t urg         : 1;
    uint32_t ece         : 1;
    uint32_t cwr         : 1;
  };
  uint16_t v;
} flags_t;

typedef struct connection {
  uint32_t is_valid;
  uint32_t is_open;
  uint32_t is_closed;
  uint32_t rx_read; // what we've read up to
  uint32_t rx_readable; // what we've received up to (ack number)
  // window size sent is BUFFER_SIZE - (rx_readable - rx_read)
  uint32_t tx_ackd; // latest acked send position (sequence number)
  uint32_t tx_sendable; // amount we can send up to (possibly in flight)
  uint32_t wait_time; // wait time to resend packet
  uint32_t mss; // min of ours and theirs
  uint32_t window_size; // for them
  uint32_t wait; // returned from wait()
  uint32_t got_ack;
  uint16_t source_port; // our port
  uint16_t dest_port; // their port
  uint8_t window_scale; // theirs (ours is 0)
  ip_t dest_ip;
  uint8_t rx_buffer[BUFFER_SIZE];
  uint8_t tx_buffer[BUFFER_SIZE];
} connection_t;

connection_t connections[NUM_CONNECTIONS];

// TODO
static uint32_t tcp_write(uint32_t idx, uint32_t flags, uint32_t len) {
  // flags is or'd together like TCP_ACK | TCP_PSH
  // printf("WRITE STUFF\n");
  uint8_t packet[1518];
  uint8_t *start, *payload;
  start = payload = packet + IP_HEADER_OFFSET;
  connection_t *conn = &connections[idx];
  write_u16(conn->source_port, &payload);
  write_u16(conn->dest_port, &payload);
  write_u32(conn->tx_ackd, &payload);
  write_u32(conn->rx_readable, &payload);
  flags_t *flg = (flags_t*)payload;
  write_u16(0, &payload); // flags (come back later)
  write_u16(BUFFER_SIZE - (conn->rx_readable - conn->rx_read), &payload);
  uint8_t *chksum = payload;
  write_u16(0, &payload); // checksum (come back later)
  write_u16(0, &payload); // urgent pointer
  if (flags & TCP_FIN) flg->fin = 1;
  if (flags & TCP_SYN) flg->syn = 1;
  if (flags & TCP_RST) flg->rst = 1;
  if (flags & TCP_PSH) flg->psh = 1;
  if (flags & TCP_ACK) flg->ack = 1;
  if (flg->syn) { // send options
    flg->data_offset = 7; // 8 bytes of options
    write_u8(2, &payload);// mss
    write_u8(4, &payload);
    write_u16(conn->mss, &payload);
    write_u8(3, &payload);// window scale
    write_u8(3, &payload);
    write_u8(0, &payload);// no shift
    write_u8(0, &payload);// end options
  } else {
    flg->data_offset = 5; // no options
  }
  //printf("LOLLL: %d\n", flg->v);
  int i;
  uint8_t* buf = conn->tx_buffer;
  for (i = 0; i < len; ++i) {
    // len should be vetted by whatever calls this
    write_u8(buf[(conn->tx_ackd + i + 1)%BUFFER_SIZE], &payload);
  }
  uint8_t* pseudo_header = start - 12;
  write_struct(ip_t, our_ip, &pseudo_header);
  write_struct(ip_t, conn->dest_ip, &pseudo_header);
  write_u8(0, &pseudo_header);
  write_u8(IPTYPE_TCP, &pseudo_header);
  write_u16(payload - start, &pseudo_header);
  write_u16(checksum(start - 12, 12 + (payload - start)), &chksum);//
  //printf("Checksum: %d\n", checksum(start - 12, 12 + (payload - start)));
  uint32_t size = ip_write_packet(&conn->dest_ip, IPTYPE_TCP, packet, payload - start);
  if (size == 0) return 0;
  send_packet(packet, size);
  return size;
}

// TODO
static void tcp_ack(uint32_t idx) {
  // send ack ...
  if (!connections[idx].is_valid) return;
  //printf("ACK!\n");
  connections[idx].wait = 0;
  tcp_write(idx, TCP_ACK, 0);
}

// TODO
static void tcp_push(uint32_t idx) {
  // send push
  connection_t *conn = &connections[idx];
  tcp_write(idx, TCP_PSH | TCP_ACK, min(conn->tx_sendable - conn->tx_ackd, min(conn->mss, conn->window_size)));
}

// TODO
void tcp_parse_packet(uint8_t* packet, uint32_t len) {
  uint16_t source_port = read_u16(&packet);
  uint16_t dest_port = read_u16(&packet);
  uint32_t seq = read_u32(&packet);
  uint32_t ack = read_u32(&packet);
  flags_t flags = read_struct(flags_t, &packet);
  uint16_t window_size = read_u16(&packet);
  uint16_t chksum __attribute__((unused)) = read_u16(&packet);
  uint16_t urg_pointer __attribute__((unused)) = read_u16(&packet);
  connection_t *connection = NULL;
  int i, j;
  for (i = 0; i < NUM_CONNECTIONS; i++) {
    if (connections[i].is_valid
     && connections[i].source_port == dest_port
     && connections[i].dest_port == source_port) {
      connection = &connections[i];
      break;
    }
  }
  if (connection == NULL) return;
  if (flags.ack) {
    connection->got_ack = 1;
    connection->tx_ackd = ack;
  }
  if (flags.fin) {
    if (!connection->is_closed) {
      ++connection->rx_readable;
      if (connection->wait == 0) connection->wait = rtc_register_handler(&tcp_ack, i, 100);
      connection->is_closed = 1;
    }
  }
  if (flags.syn) {
    connection->rx_read = connection->rx_readable = seq + 1;
    connection->tx_sendable = connection->tx_ackd;
    // send an ack
    if (connection->wait == 0) connection->wait = rtc_register_handler(&tcp_ack, i, 100);
    // mark connection open
    connection->is_open = 1;
  }
  if (flags.rst) {
    connection->is_valid = 0;
  }
  // go through options
  uint8_t* options = packet;
  if (flags.data_offset*4 > TCP_HEADER_LENGTH) {
    while ((j = read_u8(&options))) {
      switch (j) {
        case 1:
          break;
        case 2:
          j = read_u8(&options);
          if (j != 4) return;
          j = read_u16(&options);
          if (j < connection->mss) connection->mss = j;
          break;
        case 3:
          j = read_u8(&options);
          if (j != 3) return;
          connection->window_scale = read_u8(&options);
          break;
        default:
          j = read_u8(&options);
          options += j;
          break;
      }
    }
    packet += 4*flags.data_offset - TCP_HEADER_LENGTH;
  }
  // set window size
  connection->window_size = window_size << connection->window_scale;
  // TODO magic
  if (flags.data_offset*4 < len) { // received data
    int n = 0;
    for (j = 0; j < len - flags.data_offset*4; ++j) {
      if (seq + j < connection->rx_readable) continue;
      if (seq + j > connection->rx_readable) break;
      if (connection->rx_readable + 1 < connection->rx_read + BUFFER_SIZE) {
        ++n;
        ++connection->rx_readable;
        connection->rx_buffer[(connection->rx_readable)%BUFFER_SIZE] = read_u8(&packet);
      } else {
        break;
      }
    }
    //printf("Received %d bytes\n", n);
    // send ack
    if (connection->wait == 0) connection->wait = rtc_register_handler(&tcp_ack, i, 100);
    //tcp_ack(i);
  }
  // send data if we can
  if (connection->tx_sendable > connection->tx_ackd) {
    tcp_push(i);
  }
}

// TODO
int tcp_connect(int8_t* domain, uint16_t port) {
  // waits a maximum of 30 seconds?
  // blocks until handshake is mostly finished (we send ack to their syn)
  int i;
  uint16_t source_port = 0x8000;
  while (1) {
    for (i = 0; i < NUM_CONNECTIONS; ++i) {
      if (connections[i].is_valid && connections[i].source_port == source_port) goto failed;
    }
    break;
    failed:
    ++source_port;
  }
  connection_t *connection = NULL;
  for (i = 0; i < NUM_CONNECTIONS; ++i) {
    if (!connections[i].is_valid) {
      connection = &connections[i];
      break;
    }
  }
  if (connection == NULL) return -1; // no space
  connection->is_valid = 1;
  connection->dest_port = port;
  connection->source_port = source_port;
  // translate domain (maybe)
  if (parse_ip(domain, &connection->dest_ip) && dns_query((uint8_t*)domain, &connection->dest_ip)) return -1;
  connection->rx_readable = 0;
  connection->rx_read = 0;
  connection->tx_ackd = 0;
  connection->tx_sendable = 0;
  connection->wait_time = 10;//??
  connection->mss = 1460;
  connection->window_scale = 0;
  connection->is_open = 0;
  connection->is_closed = 0; // not yet
  connection->wait = 0;
  // create connection/send syn
  tcp_write(i, TCP_SYN, 0);
  // wait for connection to be open
  while (connection->is_valid && !connection->is_open);
  if (!connection->is_valid) return -1;
  return i;
}

// TODO
uint32_t tcp_send(uint32_t idx, uint8_t* data, uint32_t len) {
  // "send"
  // basically justs adds to tx buffer
  // calls send if nothing is in flight
  connection_t *conn = &connections[idx];
  if (conn->is_closed) return 0;
  // wait for space
  while (conn->is_valid && (conn->tx_ackd + BUFFER_SIZE == conn->tx_sendable));
  if (!conn->is_valid) return 0;
  uint32_t i = conn->tx_ackd + BUFFER_SIZE - conn->tx_sendable;
  if (i < len) len = i;
  if (conn->mss < len) len = conn->mss;
  if (conn->window_size < len) len = conn->window_size;
  uint32_t should_send = (conn->tx_ackd == conn->tx_sendable);
  for (i = 0; i < len; ++i) {
    conn->tx_buffer[(conn->tx_sendable+i+1)%BUFFER_SIZE] = data[i];
  }
  conn->tx_sendable += len;
  if (should_send) {
    tcp_push(idx);
  }
  return len;
}

// TODO
uint32_t tcp_recv(uint32_t idx, uint8_t* buffer, uint32_t len) {
  connection_t *conn = &connections[idx];
  while (conn->is_valid && (conn->rx_readable == conn->rx_read));
  if (!conn->is_valid) return 0;
  uint32_t read_to = len;
  if (conn->rx_readable - conn->rx_read < len) {
    read_to = conn->rx_readable - conn->rx_read;
  }
  uint32_t i;
  for (i = 0; i < read_to; ++i) {
    buffer[i] = conn->rx_buffer[(++conn->rx_read)%BUFFER_SIZE];
  }
  //conn->rx_read += read_to;
  if (conn->is_closed && (conn->rx_read == conn->rx_readable)) {
    //close
    tcp_write(idx, TCP_FIN | TCP_ACK, 0);
    conn->is_valid = 0;
  } 
  return read_to;
}

// TODO
int tcp_sendall(uint32_t idx, uint8_t* data, uint32_t len) {
  if (connections[idx].is_closed) return -1;
  uint32_t sent = 0;
  while (connections[idx].is_valid && (sent < len)) {
    sent += tcp_send(idx, data + sent, len - sent);
  }
  if (sent < len) return -1;
  return 0;
}

// TODO
int tcp_recvall(uint32_t idx, uint8_t* buffer, uint32_t len) {
  uint32_t recieved = 0;
  while (connections[idx].is_valid && (recieved < len)) {
    recieved += tcp_recv(idx, buffer + recieved, len - recieved);
  }
  if (recieved < len) return -1;
  return 0;
}

/*

Each connection has
 - Rx buffer (pointers to last read and last readable)
 - Tx buffer (pointers to last sent and last acknowledged)
   (both circular)
   (do we need an actual Tx buffer?)
   Maybe just pointers will do
 - Acknowledgements (in each buffer?)
 - current wait time
 - their MSS
 - their window size


ack for every packet recieved
also ack when user reads stuff (to update window size)???

send sends a single packet with as much data as it can send, and returns amount sent
send_all sends as many packets as required to send all data (implemented as multiple send calls)

recv(n) blocks until some data is available and receives up to n bytes and returns amount read
recv_all(n) waits until enough data is available (implemented as multiple recv calls)

everything blocks

We probably won't offer listening capabilities... (because there isn't an easy way to send stuff to the guest)

connect(url/ip, port) starts connection and makes file descriptor stuff?
(presumably we'll only use inode as an index into our own array)
(don't use open)
connect should block until the handshake is finished

close(fd) closes connection

our MSS is presumably 1460

length of tcp packet is tlen - IP_HEADER_LENGTH

maybe we can keep a predicted (conservative) window size

how should timeout stuff work?

I guess we do need to keep some sort of buffer to wait for stuff to be acked

transfer procedure:

Wait for space in tx buffer
Copy as much data as possible to tx buffer
If they were caught up
  Send data
  Reset gotack 
If there is no current timeout
  Make timeout waiting for ack
Return amount of data copied


On timeout
If gotack
  double timeout
else
  reduce timeout
Send as much data from ack as possible
Reset gotack
If data was sent, set a timeout




###

Maybe we can add some sort of timer thing
Keeps a list of callbacks and times and values that get called at some point
So we can have timeouts easily

###



(only copy as much as we intend to send?)
five things limit how much we can send...
  - our buffer space
  - our MTU
  - their MSS
  - their window size
  - amount of available data
*/
