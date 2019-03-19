#ifndef __STRINGX_H__
#define __STRINGX_H__

#include <stdint.h>

#define SINT_STR_LEN_MAX    11
#define UINT_STR_LEN_MAX    10
#define HEX8_STR_LEN_MAX    2
#define HEX16_STR_LEN_MAX   4
#define HEX32_STR_LEN_MAX   8

extern uint8_t hexstr2uint(const char *str, uint32_t *num);
extern uint8_t decstr2uint(const char *str, uint32_t *num);
extern uint8_t decstr2sint(const char *str, int32_t *num);

extern uint8_t sintstr(int32_t num, char *str);
extern uint8_t uintstr(uint32_t num, char *str);
extern uint8_t hex8str(uint8_t num, char *str);
extern uint8_t hex16str(uint16_t num, char *str);
extern uint8_t hex32str(uint32_t num, char *str);

extern char *strStartsWith(char *str1, const char *str2);
extern char *strEndsWith(char *str1, const char *str2);

#endif

