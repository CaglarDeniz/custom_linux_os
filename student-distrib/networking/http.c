#include "http.h"
#include "networking.h"
#include "../tty.h"

static int send_str(int idx, char* str) {
  return tcp_sendall(idx, (uint8_t*)str, strlen(str));
}

static void matchi(int idx, char* str, int i) {
  int l = strlen(str);
  char buf[1];
  while (i < l) {
    tcp_recv(idx, (uint8_t*)buf, 1);
    if (buf[0]==str[i]) {
      ++i;
    } else {
      i = 0;
    }
  }
}

static void match(int idx, char* str) {
  matchi(idx, str, 0);
}

static int get_option(int idx, char* op, char* buffer) {
  // assumes idx has read up to a "\r\n"
  int i = 0;
  int l = strlen(op);
  uint8_t buf[1];
  match_option:
  for (i = 0; i < l; ++i) {
    tcp_recv(idx, buf, 1);
    if (buf[0] != op[i]) {
      if (buf[0] == '\r') {
        tcp_recv(idx, buf, 1);
        if (buf[0] == '\n') return -1;//no more options
      }
      match(idx, "\r\n");
      goto match_option;
    }
  }
  tcp_recv(idx, buf, 1);
  if (buf[0] != ':') {
    match(idx, "\r\n");
    goto match_option;
  }
  do {
    tcp_recv(idx, buf, 1);
  } while (buf[0] == ' ');
  for (i = 0;;++i) {
    buffer[i] = buf[0];
    tcp_recv(idx, buf, 1);
    if ((buffer[i] == '\r') && (buf[0] == '\n')) {
      buffer[i] = '\0';
      break;
    }
  }
  return 0;
}

// TODO
int http_get(char* domain, char* path) {
  int idx = tcp_connect(domain, 80);
  if (idx == -1) return -1; // couldn't connect
  if (send_str(idx, "GET ")) return -1;
  if (send_str(idx, path)) return -1;
  if (send_str(idx, " HTTP/1.0\r\nConnection: close\r\n\r\n")) return -1;
  /*char content_length[10];
  int cl;
  if (get_option(idx, "Content-Length", content_length)) {
    // uhhhhh
    printf("FAILURE\n");
    return -1;
  }
  atoi(content_length, &cl);
  get_option(idx, "not-real", 0);
  printf("Length? '%d'\n", cl);*/
  match(idx, "\r\n\r\n");
  return idx;
}

// TODO
uint32_t http_recv(int idx, char* buffer, uint32_t len) {
  // just a thin wrapper for now
  uint32_t i = tcp_recv(idx, (uint8_t*)buffer, len);
  buffer[i] = '\0';
  return i;
}
