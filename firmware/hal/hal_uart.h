#ifndef __HAL_UART_H__
#define __HAL_UART_H__


#include <stdint.h>

#define     HAL_UART_PORT_0         0
#define     HAL_UART_PORT_1         1

#define     HAL_UART_STATE_Closed   0
#define     HAL_UART_STATE_Opened   1

#define 	HAL_UART_ERR_None 		0   //no error
#define     HAL_UART_ERR_Opened  	1   //UART already opened
#define     HAL_UART_ERR_Closed   	2   //UART already closed
#define     HAL_UART_ERR_RxE        3   //UART RX buffer empty
#define 	HAL_UART_ERR_TxO        4   //UART TX buffer overflow
#define     HAL_UART_ERR_InvalidArg 5   //invalid arguments

extern uint8_t HalUartOpen(uint8_t port);
extern uint8_t HalUartClose(uint8_t port);

extern uint8_t  HalUartGet(uint8_t port, uint8_t *pByte);
extern uint8_t  HalUartPut(uint8_t port, uint8_t byte);
extern uint16_t HalUartQryTxBuf(uint8_t port);
extern void     HalUartIsr(uint8_t port);

#endif /* __HAL_UART_H__ */
