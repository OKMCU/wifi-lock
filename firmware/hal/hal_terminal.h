#ifndef __HAL_TERMINAL_H__
#define __HAL_TERMINAL_H__


#include <stdint.h>
#include <hal_types.h>

extern void HalTerminalInit(void);
extern void HalTerminalPollRx(void);
extern void HalTerminalPollTx(void);


extern void HalTerminalPrintChar(char c);
extern void HalTerminalPrintStr(const char *s);
extern void HalTerminalPrintSint(int32_t num);
extern void HalTerminalPrintUint(uint32_t num);
extern void HalTerminalPrintHex8(uint8_t num);
extern void HalTerminalPrintHex16(uint16_t num);
extern void HalTerminalPrintHex32(uint32_t num);

#endif /* __HAL_TERMINAL_H__ */
