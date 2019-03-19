#ifndef _MD5_H
#define _MD5_H
#include <stdint.h>

#define MD5_SIZE    16

extern void md5( uint8_t md5val[MD5_SIZE] , const uint8_t *src , uint32_t len );
#endif
