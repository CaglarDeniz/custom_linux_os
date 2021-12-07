#include "networking.h"

#define BUFFER_SIZE 2048
#define MSS (1500-IP_HEADER_LENGTH-TCP_HEADER_LENGTH-TCP_MAX_OPTION_LENGTH)
#define NUM_CONNECTIONS 8

typedef struct __attribute__((packed)) flags {
  uint32_t data_offset : 4;
  uint32_t reserved    : 3;
  uint32_t ns          : 1;
  uint32_t cwr         : 1;
  uint32_t ece         : 1;
  uint32_t urg         : 1;
  uint32_t ack         : 1;
  uint32_t psh         : 1;
  uint32_t rst         : 1;
  uint32_t syn         : 1;
  uint32_t fin         : 1;
} flags_t;

typedef struct connection {
  uint32_t is_valid;
  uint32_t is_open;
  uint32_t rx_read;
  uint32_t rx_readable;
  uint32_t tx_ackd;
  uint32_t tx_sendable;
  uint32_t wait_time; // wait time to resend packet
  uint32_t mss; // min of ours and theirs
  uint32_t window_size; // for them
  uint32_t wait; // returned from wait()
  uint32_t got_ack;
  uint16_t source_port; // our port
  uint16_t dest_port; // their port
  ip_t dest_ip;
  uint8_t rx_buffer[BUFFER_SIZE];
  uint8_t tx_buffer[BUFFER_SIZE];
} connection_t;

connection_t connections[NUM_CONNECTIONS];

static void tcp_ack(uint32_t idx) {
  // send ack ...
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
    die("FIN!");
  }
  if (flags.syn) {
    connection->rx_read = connection->rx_readable = seq + 1;
    // send an ack
    tcp_ack(i);
    // mark connection open
    connection->is_open = 1;
  }
  if (flags.rst) {
    die("RST!");
  }
  // go through options
  // ****************
  // TODO magic
  if (flags.data_offset*4 < len) { // received data
    for (j = 0; j < len - flags.data_offset*4; ++j) {
      if ((connection->rx_readable + 1)%BUFFER_SIZE != connection->rx_read % BUFFER_SIZE) {
        ++connection->rx_readable;
        connection->rx_buffer[connection->rx_readable] = read_u8(&packet);
      }
    }
    // send ack
    tcp_ack(i);
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
    }
  }
  if (connection == NULL) return -1; // no space
  connection->is_valid = 1;
  connection->dest_port = port;
  connection->source_port = source_port;
  // translate domain (maybe)
  if (parse_ip(domain, &connection->dest_ip) && dns_query((uint8_t*)domain, &connection->dest_ip)) return -1;
  // create connection
  // send syn
  // wait for connection to be open
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
