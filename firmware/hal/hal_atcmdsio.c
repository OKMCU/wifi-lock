#include "hal_atcmdsio.h"
#include "hal_uart.h"
#include "hal_types.h"
#include "hal_drivers.h"
#include "hal_assert.h"
#include "hal_terminal.h"

#include "hci_wifi.h"

#include "stringx.h"

#include "osal.h"
#include "osal_fifo.h"
#include "M051Series.h"

#include <string.h>

#define HAL_ATCMDSIO_MSG_MAX_LEN        128
#define HAL_ATCMDSIO_IPD_MAX_LEN        1460

typedef struct {
    uint8_t *pBuffer;
    uint16_t rxCnt;
} RxBufCtrl_t;

static RxBufCtrl_t *pRxBufCtrl;
static uint8_t *pRxDataStream;
static OSAL_FIFO_t txFIFO;

extern void HalATCmdSIOInit(void)
{
    uint8_t err;

    HalATCmdSIODisable();
    pRxBufCtrl = NULL;
    pRxDataStream = NULL;
    txFIFO = NULL;
    
    err = HalUartOpen(HAL_UART_PORT_1);
    HAL_ASSERT(err == HAL_UART_ERR_None);
    
}

extern void HalATCmdSIOFirmwareUpdateEnable(void)
{
    P07 = 0;
}

extern void HalATCmdSIOEnable(void)
{
    P40 = 1;
}

extern void HalATCmdSIODisable(void)
{
    P40 = 0;
}

extern void HalATCmdSIOPollRx(void)
{
    HciWiFiMsgATCmd_t *msg;
    uint32_t u32tmp;
    uint8_t byte = 0;
    bool flag = FALSE;
    static uint16_t lenIPD = 0;
    //receive process
    while(HalUartGet(HAL_UART_PORT_1, &byte) == HAL_UART_ERR_None)
    {   
        #if 0
        HalTerminalPrintChar((uint8_t)byte);
        continue;
        #endif
        
        if(lenIPD > 0)
        {
            if(pRxDataStream != NULL)
                pRxDataStream[pRxBufCtrl->rxCnt] = byte;
            pRxBufCtrl->rxCnt++;
            if(pRxBufCtrl->rxCnt == lenIPD)
            {
                if(pRxDataStream != NULL)
                    pRxDataStream[pRxBufCtrl->rxCnt] = (uint8_t)'\0';
                //send out the msg +IPD,1024:{(uint8_t *)}
                pRxDataStream = NULL;
                lenIPD = 0;
                flag = TRUE;
            }
        }
        else
        {
            if(pRxBufCtrl == NULL)
            {
                pRxBufCtrl = osal_mem_alloc(sizeof(RxBufCtrl_t));
                HAL_ASSERT(pRxBufCtrl != NULL);
                pRxBufCtrl->pBuffer = NULL;
                pRxBufCtrl->rxCnt = 0;
            }
            
            if(pRxBufCtrl->rxCnt < HAL_ATCMDSIO_MSG_MAX_LEN - 1)
            {
                if(pRxBufCtrl->pBuffer == NULL)
                    pRxBufCtrl->pBuffer = osal_mem_alloc(HAL_ATCMDSIO_MSG_MAX_LEN);
                HAL_ASSERT(pRxBufCtrl->pBuffer != NULL);
                
                pRxBufCtrl->pBuffer[pRxBufCtrl->rxCnt++] = byte;
                pRxBufCtrl->pBuffer[pRxBufCtrl->rxCnt] = (uint8_t)'\0';
            }

            if(byte == (uint8_t)':' && pRxBufCtrl->rxCnt <= 10)//"+IPD,2920:"
            {
                if(strStartsWith((char *)(pRxBufCtrl->pBuffer), "+IPD,"))
                {
                    osal_set_event(HciWiFiTaskID, HCI_WIFI_EVT_Receiving);
                    //point to ':' and re-write it to '\0'
                    pRxBufCtrl->pBuffer[--pRxBufCtrl->rxCnt] = (uint8_t)'\0';
                    HAL_ASSERT(decstr2uint((char *)(pRxBufCtrl->pBuffer + 5), &u32tmp) != 0);
                    //recover ':' and rx length
                    pRxBufCtrl->pBuffer[pRxBufCtrl->rxCnt++] = (uint8_t)':';
                    //get the rx length
                    //HAL_ASSERT(u32tmp <= HAL_ATCMDSIO_IPD_MAX_LEN);
                    lenIPD = (uint16_t)u32tmp;
                    if(lenIPD <= HAL_ATCMDSIO_IPD_MAX_LEN)
                        pRxDataStream = (uint8_t *)osal_mem_alloc(lenIPD+1);//+1 for '\0'
                    //HAL_ASSERT(pRxDataStream != NULL);
                    pRxBufCtrl->pBuffer[pRxBufCtrl->rxCnt++] = ((uint32_t)pRxDataStream>>24) & 0xFF;
                    pRxBufCtrl->pBuffer[pRxBufCtrl->rxCnt++] = ((uint32_t)pRxDataStream>>16) & 0xFF;
                    pRxBufCtrl->pBuffer[pRxBufCtrl->rxCnt++] = ((uint32_t)pRxDataStream>> 8) & 0xFF;
                    pRxBufCtrl->pBuffer[pRxBufCtrl->rxCnt++] = ((uint32_t)pRxDataStream>> 0) & 0xFF;
                    pRxBufCtrl->rxCnt = 0;
                }
            }

            if(byte == '\n')
            {
                flag = TRUE;
            }
            
        }

        
        if(flag == TRUE)
        {
            msg = (HciWiFiMsgATCmd_t *)osal_msg_allocate(sizeof(HciWiFiMsgATCmd_t));
            HAL_ASSERT(msg != NULL);
            msg->head = HCI_WIFI_MSG_HEAD_ATCmd;
            msg->pATCmd = (char *)pRxBufCtrl->pBuffer;
            osal_msg_send(HciWiFiTaskID, (uint8_t *) msg);
            osal_mem_free(pRxBufCtrl);
            pRxBufCtrl = NULL;
            flag = FALSE;
        }
    }
}

extern void HalATCmdSIOPollTx(void)
{
    uint16_t space = 0;
    if(txFIFO != NULL)
    {
        while(osal_fifo_len(txFIFO))
        {
            if(space == 0)
                space = HalUartQryTxBuf(HAL_UART_PORT_1);
            if(space > 0)
            {
                HAL_ASSERT(HalUartPut(HAL_UART_PORT_1, osal_fifo_get(txFIFO)) == HAL_UART_ERR_None);
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

extern void HalATCmdSIOSendByte(uint8_t byte)
{
    if(txFIFO == NULL)
        txFIFO = osal_fifo_create();
    HAL_ASSERT(txFIFO != NULL);
    HAL_ASSERT(osal_fifo_put(txFIFO, byte) != NULL);
    osal_set_event(Hal_TaskID, HAL_UART1_TX_DATA_EVT);
}

extern void HalATCmdSIOPrintChar(char c)
{
    HalATCmdSIOSendByte((uint8_t)c);
}

extern void HalATCmdSIOPrintStr(const char *s)
{
    while(*s)
    {
        HalATCmdSIOSendByte((uint8_t)(*s++));
    }
}

extern void HalATCmdSIOPrintSint(int32_t num)
{
    char str[SINT_STR_LEN_MAX];
    uint8_t len;
    uint8_t i;
    
    len = sintstr(num, str);
    
    for(i = 0; i < len; i++)
    {
        HalATCmdSIOSendByte((uint8_t)str[i]);
    }
    
}

extern void HalATCmdSIOPrintUint(uint32_t num)
{
    char str[UINT_STR_LEN_MAX];
    uint8_t len;
    uint8_t i;
    
    len = uintstr(num, str);
    
    for(i = 0; i < len; i++)
    {
        HalATCmdSIOSendByte((uint8_t)str[i]);
    }
}

extern void HalATCmdSIOPrintHex8(uint8_t num)
{
    char str[HEX8_STR_LEN_MAX];
    uint8_t len;
    uint8_t i;
    
    len = hex8str(num, str);
    
    for(i = 0; i < len; i++)
    {
        HalATCmdSIOSendByte((uint8_t)str[i]);
    }
}


extern void HalATCmdSIOPrintHex16(uint16_t num)
{
    char str[HEX16_STR_LEN_MAX];
    uint8_t len;
    uint8_t i;
    
    len = hex16str(num, str);
    
    for(i = 0; i < len; i++)
    {
        HalATCmdSIOSendByte((uint8_t)str[i]);
    }
}

extern void HalATCmdSIOPrintHex32(uint32_t num)
{
    char str[HEX32_STR_LEN_MAX];
    uint8_t len;
    uint8_t i;
    
    len = hex32str(num, str);
    
    for(i = 0; i < len; i++)
    {
        HalATCmdSIOSendByte((uint8_t)str[i]);
    }
}

