#include "hal_uart.h"
#include "hal_assert.h"
#include "hal_drivers.h"
#include "M051Series.h"

#include "osal.h"

#define HAL_UART0_TX_BUFFER_SIZE    8
#define HAL_UART0_RX_BUFFER_SIZE    8
#define HAL_UART1_TX_BUFFER_SIZE    8
#define HAL_UART1_RX_BUFFER_SIZE    16

typedef struct {
    uint8_t *pBuffer;
    uint16_t maxBufSize;
    uint16_t head;
    uint16_t tail;
} HalUartBufCtrl_t;

typedef struct {
    uint8_t opened;
    HalUartBufCtrl_t rxBuf;
    HalUartBufCtrl_t txBuf;
} HalUartCfg_t;

static uint8_t uart0RxBuf[HAL_UART0_RX_BUFFER_SIZE];
static uint8_t uart0TxBuf[HAL_UART0_TX_BUFFER_SIZE];
static uint8_t uart1RxBuf[HAL_UART1_RX_BUFFER_SIZE];
static uint8_t uart1TxBuf[HAL_UART1_TX_BUFFER_SIZE];

static HalUartCfg_t uartCfg[2];

extern uint8_t HalUartOpen(uint8_t port)
{
    if(port > HAL_UART_PORT_1)
        return HAL_UART_ERR_InvalidArg;
    
    if(uartCfg[port].opened == HAL_UART_STATE_Opened)
        return HAL_UART_ERR_Opened;
    
    if(port == HAL_UART_PORT_0)
    {
        uartCfg[port].rxBuf.pBuffer = uart0RxBuf;
        uartCfg[port].rxBuf.maxBufSize = HAL_UART0_RX_BUFFER_SIZE;
        uartCfg[port].txBuf.pBuffer = uart0TxBuf;
        uartCfg[port].txBuf.maxBufSize = HAL_UART0_TX_BUFFER_SIZE;
        
        /* Reset UART0 */
        SYS_ResetModule(UART0_RST);
        /* Init UART0 to 115200-8n1 for print message */
        UART_Open(UART0, 115200);
        /* Enable Interrupt and install the call back function */
        //UART_ENABLE_INT(UART0, (UART_IER_RDA_IEN_Msk | UART_IER_THRE_IEN_Msk | UART_IER_RTO_IEN_Msk));
        UART_ENABLE_INT(UART0, (UART_IER_RDA_IEN_Msk | UART_IER_RTO_IEN_Msk));
        NVIC_EnableIRQ(UART0_IRQn);
    }
    else if(port == HAL_UART_PORT_1)
    {
        uartCfg[port].rxBuf.pBuffer = uart1RxBuf;
        uartCfg[port].rxBuf.maxBufSize = HAL_UART1_RX_BUFFER_SIZE;
        uartCfg[port].txBuf.pBuffer = uart1TxBuf;
        uartCfg[port].txBuf.maxBufSize = HAL_UART1_TX_BUFFER_SIZE;
        
        /* Reset UART1 */
        SYS_ResetModule(UART1_RST);
        /* Init UART1 to 115200-8n1 for print message */
        UART_Open(UART1, 115200);
        /* Enable Interrupt and install the call back function */
        //UART_ENABLE_INT(UART1, (UART_IER_RDA_IEN_Msk | UART_IER_THRE_IEN_Msk | UART_IER_RTO_IEN_Msk));
        UART_ENABLE_INT(UART1, (UART_IER_RDA_IEN_Msk | UART_IER_RTO_IEN_Msk));
        NVIC_EnableIRQ(UART1_IRQn);
    }
    
    uartCfg[port].rxBuf.head = 0;
    uartCfg[port].rxBuf.tail = 0;
    uartCfg[port].txBuf.head = 0;
    uartCfg[port].txBuf.tail = 0;
    uartCfg[port].opened = HAL_UART_STATE_Opened;
    
	return HAL_UART_ERR_None;
}
extern uint8_t HalUartClose(uint8_t port)
{
	if(port > HAL_UART_PORT_1)
        return HAL_UART_ERR_InvalidArg;
    if(uartCfg[port].opened == HAL_UART_STATE_Closed)
        return HAL_UART_ERR_Closed;
    
    uartCfg[port].opened = HAL_UART_STATE_Closed;
    
    if(port == HAL_UART_PORT_0)
    {
        UART_DISABLE_INT(UART0, (UART_IER_RDA_IEN_Msk | UART_IER_THRE_IEN_Msk | UART_IER_RTO_IEN_Msk));
        NVIC_DisableIRQ(UART0_IRQn);
        UART_Close(UART0);
    }
    else if(port == HAL_UART_PORT_1)
    {
        UART_DISABLE_INT(UART1, (UART_IER_RDA_IEN_Msk | UART_IER_THRE_IEN_Msk | UART_IER_RTO_IEN_Msk));
        NVIC_DisableIRQ(UART1_IRQn);
        UART_Close(UART1);
    }
	
	return HAL_UART_ERR_None;
}


extern uint8_t HalUartGet(uint8_t port, uint8_t *pByte)
{
    if(port > HAL_UART_PORT_1)
        return HAL_UART_ERR_InvalidArg;
    if(uartCfg[port].opened == HAL_UART_STATE_Closed)
        return HAL_UART_ERR_Closed;
    
    if(uartCfg[port].rxBuf.tail != uartCfg[port].rxBuf.head)
    {
        *pByte = uartCfg[port].rxBuf.pBuffer[uartCfg[port].rxBuf.tail++];
        if(uartCfg[port].rxBuf.tail >= uartCfg[port].rxBuf.maxBufSize)
        {
            uartCfg[port].rxBuf.tail = 0;
        }
    }
    else
    {
        return HAL_UART_ERR_RxE;
    }
    
    return HAL_UART_ERR_None;
}


static void halUartRxByte(uint8_t port, uint8_t byte)
{
    uint16_t newHead;
    
    newHead = uartCfg[port].rxBuf.head;
    newHead++;
    if(newHead >= uartCfg[port].rxBuf.maxBufSize)
        newHead = 0;
    
    //if(newHead == uartCfg[port].rxBuf.tail)
    //    return; //buffer is full
    HAL_ASSERT(newHead != uartCfg[port].rxBuf.tail);
    
	uartCfg[port].rxBuf.pBuffer[uartCfg[port].rxBuf.head] = byte;
    uartCfg[port].rxBuf.head = newHead;
}

extern uint8_t HalUartPut(uint8_t port, uint8_t byte)
{   
    //while(UART_IS_TX_FULL(UART0));
    //UART_WRITE(UART0, c);
    uint16_t newHead;
    UART_T *UARTx[] = {UART0, UART1};
    
    if(port > HAL_UART_PORT_1)
        return HAL_UART_ERR_InvalidArg;
    if(uartCfg[port].opened == HAL_UART_STATE_Closed)
        return HAL_UART_ERR_Closed;
    
    newHead = uartCfg[port].txBuf.head;
    newHead++;
    if(newHead >= uartCfg[port].txBuf.maxBufSize)
    {
        newHead = 0;
    }
    if(newHead == uartCfg[port].txBuf.tail)
    {
        //Software TX buffer is full.
        return HAL_UART_ERR_TxO;
    }
    
    uartCfg[port].txBuf.pBuffer[uartCfg[port].txBuf.head] = byte;
    uartCfg[port].txBuf.head = newHead;
    
    UART_ENABLE_INT(UARTx[port], UART_IER_THRE_IEN_Msk);
    
    return HAL_UART_ERR_None;
}

extern uint16_t HalUartQryTxBuf(uint8_t port)
{
    uint16_t newHead;
    uint16_t space = 0;
    if(port > HAL_UART_PORT_1)
        return 0;
    if(uartCfg[port].opened == HAL_UART_STATE_Closed)
        return 0;
    newHead = uartCfg[port].txBuf.head;
    do
    {
        newHead++;
        if(newHead >= uartCfg[port].txBuf.maxBufSize)
        {
            newHead = 0;
        }
        if(newHead != uartCfg[port].txBuf.tail)
        {
            space++;
        }
    }
    while(newHead != uartCfg[port].txBuf.tail);

    return space;
}

extern void HalUartIsr(uint8_t port)
{
	uint8_t u8InChar = 0xFF;
    uint32_t u32IntSts;
    UART_T *UARTx[] = {UART0, UART1};
    uint16_t uartxTxEvt[] = {HAL_UART0_TX_DATA_EVT, HAL_UART1_TX_DATA_EVT};
    uint16_t uartxRxEvt[] = {HAL_UART0_RX_DATA_EVT, HAL_UART1_RX_DATA_EVT};
    u32IntSts = UARTx[port]->ISR;
    
    
    if(u32IntSts & UART_ISR_RDA_INT_Msk)
    {
        /* Get all the input characters */
        while(UART_IS_RX_READY(UARTx[port]))
        {
            /* Get the character from UART Buffer */
            u8InChar = UART_READ(UARTx[port]);
			halUartRxByte(port, u8InChar);
            osal_set_event(Hal_TaskID, uartxRxEvt[port]);
        }
    }
    
    
    if(u32IntSts & UART_ISR_THRE_INT_Msk)
    {
        if(uartCfg[port].txBuf.tail != uartCfg[port].txBuf.head)
        {
            u8InChar = uartCfg[port].txBuf.pBuffer[uartCfg[port].txBuf.tail++];
			if(uartCfg[port].txBuf.tail >= uartCfg[port].txBuf.maxBufSize)
			{
				uartCfg[port].txBuf.tail = 0;
			}
            while(UART_IS_TX_FULL(UARTx[port]));  //Wait Tx is not full to transmit data       
            UART_WRITE(UARTx[port], u8InChar);
        }
        else
        {
            UART_DISABLE_INT(UARTx[port], UART_IER_THRE_IEN_Msk);
        }
        osal_set_event(Hal_TaskID, uartxTxEvt[port]);
    }
}

