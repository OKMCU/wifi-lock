#include "hal_terminal.h"
#include "hal_uart.h"
#include "hal_types.h"
#include "hal_drivers.h"
#include "hal_assert.h"
#include "lib\stringx.h"

#include "app_lock.h"

#include "osal.h"
#include "osal_fifo.h"
#include "M051Series.h"

#define HAL_TERMINAL_MSG_MAX_LEN        128

typedef struct {
    char *pBuffer;
    uint16_t rxCnt;
} RxBufCtrl_t;

static RxBufCtrl_t *pRxBufCtrl = NULL;
static OSAL_FIFO_t txFIFO = NULL;

extern void HalTerminalInit(void)
{
    uint8_t err;

    err = HalUartOpen(HAL_UART_PORT_0);
    HAL_ASSERT(err == HAL_UART_ERR_None);
}

extern void HalTerminalPollRx(void)
{
    uint8_t byte = 0;
    char c;
    bool flag = FALSE;
    AppLockMsgTerminal_t *pMsg;
    
    //receive process
    while(HalUartGet(HAL_UART_PORT_0, &byte) == HAL_UART_ERR_None)
    {
        c = (char)byte;

        if(c != '\n')
            HalTerminalPrintChar(c);

        if(c != '\r' && c != '\n')
        {
            if(pRxBufCtrl == NULL)
            {
                pRxBufCtrl = osal_mem_alloc(sizeof(RxBufCtrl_t));
                HAL_ASSERT(pRxBufCtrl != NULL);
                pRxBufCtrl->pBuffer = NULL;
                pRxBufCtrl->rxCnt = 0;
            }
            
            if(pRxBufCtrl->rxCnt < HAL_TERMINAL_MSG_MAX_LEN - 1)
            {
                if(pRxBufCtrl->pBuffer == NULL)
                    pRxBufCtrl->pBuffer = osal_mem_alloc(HAL_TERMINAL_MSG_MAX_LEN);
                HAL_ASSERT(pRxBufCtrl->pBuffer != NULL);
                pRxBufCtrl->pBuffer[pRxBufCtrl->rxCnt++] = c;
            }
        }
        
        if(c == '\r')
        {
            HalTerminalPrintChar('\n');
            
            if(pRxBufCtrl != NULL)
            {
                if(pRxBufCtrl->pBuffer != NULL)
                    flag = TRUE;
            }
        }
        
        if(flag == TRUE)
        {
            pRxBufCtrl->pBuffer[pRxBufCtrl->rxCnt++] = 0;
            pMsg = (AppLockMsgTerminal_t *)osal_msg_allocate(sizeof(AppLockMsgTerminal_t));
            HAL_ASSERT(pMsg != NULL);
            pMsg->head = APP_LOCK_MSG_HEAD_Terminal;
            pMsg->pCmd = pRxBufCtrl->pBuffer;
            osal_msg_send(AppLockTaskID, (uint8_t *) pMsg);
            osal_mem_free(pRxBufCtrl);
            pRxBufCtrl = NULL;
            flag = FALSE;
        }
    }


}

extern void HalTerminalPollTx(void)
{
    uint16_t space = 0;
    if(txFIFO != NULL)
    {
        while(osal_fifo_len(txFIFO))
        {
            if(space == 0)
                space = HalUartQryTxBuf(HAL_UART_PORT_0);
            if(space > 0)
            {
                HAL_ASSERT(HalUartPut(HAL_UART_PORT_0, osal_fifo_get(txFIFO)) == HAL_UART_ERR_None);
                space--;
            }
            else
            {
                break;
            }
        }

        if(osal_fifo_len(txFIFO) == 0)
        {
            osal_fifo_delete(txFIFO);
            txFIFO = NULL;
        }
    }
}

extern void HalTerminalPrintChar(char c)
{
    if(txFIFO == NULL)
        txFIFO = osal_fifo_create();
    HAL_ASSERT(txFIFO != NULL);
    HAL_ASSERT(osal_fifo_put(txFIFO, (uint8_t)c) != NULL);
    osal_set_event(Hal_TaskID, HAL_UART0_TX_DATA_EVT);
}

extern void HalTerminalPrintStr(const char *s)
{
    while(*s)
    {
        HalTerminalPrintChar(*s++);
    }
}

extern void HalTerminalPrintSint(int32_t num)
{
    char str[SINT_STR_LEN_MAX];
    uint8_t len;
    uint8_t i;
    
    len = sintstr(num, str);
    
    for(i = 0; i < len; i++)
    {
        HalTerminalPrintChar(str[i]);
    }
    
}

extern void HalTerminalPrintUint(uint32_t num)
{
    char str[UINT_STR_LEN_MAX];
    uint8_t len;
    uint8_t i;
    
    len = uintstr(num, str);
    
    for(i = 0; i < len; i++)
    {
        HalTerminalPrintChar(str[i]);
    }
}

extern void HalTerminalPrintHex8(uint8_t num)
{
    char str[HEX8_STR_LEN_MAX];
    uint8_t len;
    uint8_t i;
    
    len = hex8str(num, str);
    
    for(i = 0; i < len; i++)
    {
        HalTerminalPrintChar(str[i]);
    }
}


extern void HalTerminalPrintHex16(uint16_t num)
{
    char str[HEX16_STR_LEN_MAX];
    uint8_t len;
    uint8_t i;
    
    len = hex16str(num, str);
    
    for(i = 0; i < len; i++)
    {
        HalTerminalPrintChar(str[i]);
    }
}

extern void HalTerminalPrintHex32(uint32_t num)
{
    char str[HEX32_STR_LEN_MAX];
    uint8_t len;
    uint8_t i;
    
    len = hex32str(num, str);
    
    for(i = 0; i < len; i++)
    {
        HalTerminalPrintChar(str[i]);
    }
}

