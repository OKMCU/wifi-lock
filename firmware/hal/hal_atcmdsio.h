#ifndef __HAL_ATCMDSIO_H__
#define __HAL_ATCMDSIO_H__


#include <stdint.h>
#include <hal_types.h>

extern void HalATCmdSIOInit(void);
extern void HalATCmdSIOFirmwareUpdateEnable(void);
extern void HalATCmdSIOEnable(void);
extern void HalATCmdSIODisable(void);
extern void HalATCmdSIOPollRx(void);
extern void HalATCmdSIOPollTx(void);

extern void HalATCmdSIOSendByte(uint8_t byte);
extern void HalATCmdSIOPrintChar(char c);
extern void HalATCmdSIOPrintStr(const char *s);
extern void HalATCmdSIOPrintSint(int32_t num);
extern void HalATCmdSIOPrintUint(uint32_t num);
extern void HalATCmdSIOPrintHex8(uint8_t num);
extern void HalATCmdSIOPrintHex16(uint16_t num);
extern void HalATCmdSIOPrintHex32(uint32_t num);

#endif /* __HAL_ATCMDSIO_H__ */
