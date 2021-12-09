#ifndef HTTP_H
#define HTTP_H

#include "../lib.h"

int http_get(char* domain, char* path);
uint32_t http_recv(int idx, char* buffer, uint32_t len);

#endif /* HTTP_H */
