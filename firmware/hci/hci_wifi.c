#include <stdio.h>
#include <string.h>
#include "stringx.h"
#include "osal.h"
#include "hal_atcmdsio.h"
#include "hal_assert.h"
#include "hal_terminal.h"
#include "hci_wifi.h"
#include "hci_wificmd.h"
#include "hci_wifiatcop.h"
#include "osal_fifo.h"

//#define SERVER_IP                   "192.168.1.106"
//#define SERVER_PORT                 8080
//#define SSID                        "TP-LINK_EE43BA"
//#define PASSWD                      "87654321"
//#define SSID                        "nicolel"
//#define PASSWD                      "7335012asd"

#define HCI_WIFI_STATE_COMM_SET(state)                  st(curState |=  (state<<12);)
#define HCI_WIFI_STATE_COMM_CLR(state)                  st(curState &= ~(state<<12);)

uint8_t HciWiFiTaskID;

static uint16_t curState;
static uint16_t tarState;
static HciWiFiConnInitStrcuct_t *pConnInitStruct;
static HciWiFiPairingInitStruct_t *pPairingInitStruct;
static HciWiFiSocketInitStruct_t *pSocketInitStruct;
static OSAL_FIFO_t scktTxFIFO;

static void HciWiFi_ProcessOsalMsg(HciWiFiMsg_t *pMsg);
static void HciWiFi_ProcessHciState(uint16_t state);
static void HciWiFi_StateController(uint16_t *cur_state, uint16_t *tar_state);
static void HciWiFi_ProcessTxDataStream(uint16_t len);
static void HciWiFi_SocketSendKernel(uint8_t byte);

extern void HciWiFi_Init( uint8_t task_id )
{
    HciWiFiTaskID = task_id;

    curState = 0x0000;
    tarState = HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                              HCI_WIFI_STATE_SCKT_Closed, 
                              HCI_WIFI_STATE_CONN_Disconnected, 
                              HCI_WIFI_STATE_DEV_Initialized);
    pConnInitStruct = NULL;
    pPairingInitStruct = NULL;
    pSocketInitStruct = NULL;
    scktTxFIFO = NULL;
    
    osal_start_timerEx(task_id, HCI_WIFI_EVT_HciEnable, HCI_WIFI_CONFIG_HciEnableDelay);
}


extern uint16_t HciWiFi_ProcessEvent ( uint8_t task_id, uint16_t events )
{   
    uint8_t *pMsg;
    
    if ( events & SYS_EVENT_MSG )
    {
        pMsg = osal_msg_receive(task_id);

        while (pMsg)
        {
          /* Process msg */
          HciWiFi_ProcessOsalMsg((HciWiFiMsg_t *)pMsg);
          /* De-allocate */
          osal_msg_deallocate(pMsg);
          /* Next */
          pMsg = osal_msg_receive(task_id);
        }
        return events ^ SYS_EVENT_MSG;
    }
    
    if( events & HCI_WIFI_EVT_CheckHciWiFiState )
    {
        HciWiFi_StateController(&curState, &tarState);
        return events ^ HCI_WIFI_EVT_CheckHciWiFiState;
    }

    if( events & HCI_WIFI_EVT_HciEnable )
    {
        HalATCmdSIOEnable();
        return events ^ HCI_WIFI_EVT_HciEnable;
    }

    if( events & HCI_WIFI_EVT_TxPoll )
    {
        if(curState == HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                                      HCI_WIFI_STATE_SCKT_Opened, 
                                      HCI_WIFI_STATE_CONN_GotIP, 
                                      HCI_WIFI_STATE_DEV_Initialized) ||
           curState == HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Rx, 
                                      HCI_WIFI_STATE_SCKT_Opened, 
                                      HCI_WIFI_STATE_CONN_GotIP, 
                                      HCI_WIFI_STATE_DEV_Initialized))
        {
            if(scktTxFIFO != NULL)
            {
                HCI_WIFI_STATE_COMM_SET(HCI_WIFI_STATE_COMM_Tx);
                HciWiFiCmdTxDataStreamStart((uint16_t)osal_fifo_len(scktTxFIFO));
            }
        }
        return events ^ HCI_WIFI_EVT_TxPoll;
    }

    if( events & HCI_WIFI_EVT_Transmitted )
    {
        if(scktTxFIFO != NULL)
        {
            HciWiFiCmdTxDataStreamStart((uint16_t)osal_fifo_len(scktTxFIFO));
        }
        else
        {
            HCI_WIFI_STATE_COMM_CLR(HCI_WIFI_STATE_COMM_Tx);
        } 
        return events ^ HCI_WIFI_EVT_Transmitted;
    }

    if( events & HCI_WIFI_EVT_Receiving)
    {
        //receiving data
        HCI_WIFI_STATE_COMM_SET(HCI_WIFI_STATE_COMM_Rx);
        return events ^ HCI_WIFI_EVT_Receiving;
    }
    
    if( events & HCI_WIFI_EVT_PairingTimeout )
    {
        /*
        tarState = HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                                  HCI_WIFI_STATE_SCKT_Closed, 
                                  HCI_WIFI_STATE_CONN_PairingStopped, 
                                  HCI_WIFI_STATE_DEV_Initialized);
        
        osal_set_event(task_id, HCI_WIFI_EVT_CheckHciWiFiState);
        */
        curState = 0x0000;
        HciWiFiCmdSmartConfigStop();
        return events ^ HCI_WIFI_EVT_PairingTimeout;
    }
    
    return 0;
}

extern void HciWiFiReset(void)
{
    HalATCmdSIODisable();
    if(scktTxFIFO != NULL)
    {
        osal_fifo_delete(scktTxFIFO);
        scktTxFIFO = NULL;
    }
    
    curState = 0x0000;
    tarState = HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                              HCI_WIFI_STATE_SCKT_Closed, 
                              HCI_WIFI_STATE_CONN_Disconnected, 
                              HCI_WIFI_STATE_DEV_Initialized);

    if(pPairingInitStruct != NULL)
    {
        osal_mem_free(pPairingInitStruct);
        pPairingInitStruct = NULL;
    }
    
    if(pConnInitStruct != NULL)
    {
        if(pConnInitStruct->passwd != NULL)
            osal_mem_free(pConnInitStruct->passwd);
        if(pConnInitStruct->ssid != NULL)
            osal_mem_free(pConnInitStruct->ssid);
        osal_mem_free(pConnInitStruct);
        pConnInitStruct = NULL;
    }

    if(pSocketInitStruct != NULL)
    {
        if(pSocketInitStruct->addr != NULL)
            osal_mem_free(pSocketInitStruct->addr);
        osal_mem_free(pSocketInitStruct);
        pSocketInitStruct = NULL;
    }

    osal_stop_timerEx(HciWiFiTaskID, HCI_WIFI_EVT_PairingTimeout);
    osal_start_timerEx(HciWiFiTaskID, HCI_WIFI_EVT_HciEnable, HCI_WIFI_CONFIG_HciEnableDelay);
}

extern uint16_t HciWiFiGetState(void)
{
    return curState;
}

extern uint8_t HciWiFiSetConnection(HciWiFiConnInitStrcuct_t *p_arg)
{
    uint16_t len;
    //disconnect current Wi-Fi connection
    if(p_arg == NULL)
        return HCI_WIFI_CONN_ERR_InvalidArg;
    if(p_arg->ssid == NULL)
        return HCI_WIFI_CONN_ERR_InvalidArg;
    if(pConnInitStruct != NULL)
        return HCI_WIFI_CONN_ERR_Busy;

    pConnInitStruct = (HciWiFiConnInitStrcuct_t *)osal_mem_alloc(sizeof(HciWiFiConnInitStrcuct_t));
    HAL_ASSERT(pConnInitStruct != NULL);
    len = strlen(p_arg->ssid)+1;//+1 for '\0'
    pConnInitStruct->ssid = (char *)osal_mem_alloc(len);
    HAL_ASSERT(pConnInitStruct->ssid != NULL);
    osal_memcpy(pConnInitStruct->ssid, p_arg->ssid, len);

    if(p_arg->passwd == NULL)
    {
        //do not need password
        pConnInitStruct->passwd = NULL;
    }
    else
    {
        len = strlen(p_arg->passwd)+1;//+1 for '\0'
        pConnInitStruct->passwd = osal_mem_alloc(len);
        HAL_ASSERT(pConnInitStruct->passwd != NULL);
        osal_memcpy(pConnInitStruct->passwd, p_arg->passwd, len);
    }

    pConnInitStruct->onWiFiConnected = p_arg->onWiFiConnected;
    pConnInitStruct->onWiFiDisconnected = p_arg->onWiFiDisconnected;

    tarState = HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                              HCI_WIFI_STATE_SCKT_Closed, 
                              HCI_WIFI_STATE_CONN_GotIP, 
                              HCI_WIFI_STATE_DEV_Initialized);
    osal_set_event(HciWiFiTaskID, HCI_WIFI_EVT_CheckHciWiFiState);
    
    return HCI_WIFI_CONN_ERR_None;
}

extern void HciWiFiPairingStart(HciWiFiPairingInitStruct_t *p_arg)
{
    if(pPairingInitStruct == NULL)
    {
        pPairingInitStruct = (HciWiFiPairingInitStruct_t *)osal_mem_alloc(sizeof(HciWiFiPairingInitStruct_t));
    }
    HAL_ASSERT(pPairingInitStruct != NULL);
    osal_memcpy(pPairingInitStruct, p_arg, sizeof(HciWiFiPairingInitStruct_t));
    tarState = HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                              HCI_WIFI_STATE_SCKT_Closed, 
                              HCI_WIFI_STATE_CONN_PairingWaiting, 
                              HCI_WIFI_STATE_DEV_Initialized);
    osal_set_event(HciWiFiTaskID, HCI_WIFI_EVT_CheckHciWiFiState);
}

extern uint8_t HciWiFiSocketOpen(HciWiFiSocketInitStruct_t *p_arg)
{
    uint16_t len;

    //arguments check
    if(p_arg == NULL)
        return HCI_WIFI_SCKT_ERR_InvalidArg;
    if(p_arg->addr == NULL)
        return HCI_WIFI_SCKT_ERR_InvalidArg;
    if(p_arg->port == 0)
        return HCI_WIFI_SCKT_ERR_InvalidArg;
    if(HCI_WIFI_STATE_CONN(curState) != HCI_WIFI_STATE_CONN_GotIP)
        return HCI_WIFI_SCKT_ERR_WiFiNA;
    if(pSocketInitStruct != NULL)
        return HCI_WIFI_SCKT_ERR_ReOpen;
    
    pSocketInitStruct = (HciWiFiSocketInitStruct_t *)osal_mem_alloc(sizeof(HciWiFiSocketInitStruct_t));
    HAL_ASSERT(pSocketInitStruct != NULL);

    //save the addr & port
    len = strlen(p_arg->addr) + 1;//+1 for '\0'
    pSocketInitStruct->addr = (char *)osal_mem_alloc(len);
    HAL_ASSERT(pSocketInitStruct->addr != NULL);
    osal_memcpy(pSocketInitStruct->addr, p_arg->addr, len);
    
    pSocketInitStruct->port = p_arg->port;
    pSocketInitStruct->onSocketOpened = p_arg->onSocketOpened;
    pSocketInitStruct->onSocketClosed = p_arg->onSocketClosed;
    pSocketInitStruct->onSocketRxData = p_arg->onSocketRxData;

    
    tarState = HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                              HCI_WIFI_STATE_SCKT_Opened, 
                              HCI_WIFI_STATE_CONN_GotIP, 
                              HCI_WIFI_STATE_DEV_Initialized);
    
    osal_set_event(HciWiFiTaskID, HCI_WIFI_EVT_CheckHciWiFiState);
    
    return HCI_WIFI_SCKT_ERR_None;
}

extern uint8_t HciWiFiSocketClose(void)
{
    if(pSocketInitStruct == NULL)
        return HCI_WIFI_SCKT_ERR_ReClose;
    
    tarState = HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                              HCI_WIFI_STATE_SCKT_Closed, 
                              HCI_WIFI_STATE_CONN_GotIP, 
                              HCI_WIFI_STATE_DEV_Initialized);
    
    osal_set_event(HciWiFiTaskID, HCI_WIFI_EVT_CheckHciWiFiState);

    return HCI_WIFI_SCKT_ERR_None;
}

extern void HciWiFiSocketSend(uint8_t *pData, uint16_t len)
{
    uint16_t i;
    if(HCI_WIFI_STATE_DEV(curState)  != HCI_WIFI_STATE_DEV_Initialized ||
       HCI_WIFI_STATE_CONN(curState) != HCI_WIFI_STATE_CONN_GotIP      ||
       HCI_WIFI_STATE_SCKT(curState) != HCI_WIFI_STATE_SCKT_Opened)
        return;
    
    for(i = 0; i < len; i++)
    {
        HciWiFi_SocketSendKernel(pData[i]);
    }
    osal_set_event(HciWiFiTaskID, HCI_WIFI_EVT_TxPoll);
}

extern void HciWiFiSocketSendByte(uint8_t byte)
{
    if(HCI_WIFI_STATE_DEV(curState)  != HCI_WIFI_STATE_DEV_Initialized ||
       HCI_WIFI_STATE_CONN(curState) != HCI_WIFI_STATE_CONN_GotIP      ||
       HCI_WIFI_STATE_SCKT(curState) != HCI_WIFI_STATE_SCKT_Opened)
        return;
    
    HciWiFi_SocketSendKernel(byte);
    osal_set_event(HciWiFiTaskID, HCI_WIFI_EVT_TxPoll);
}

extern void HciWiFiSocketSendChar(char c)
{
    if(HCI_WIFI_STATE_DEV(curState)  != HCI_WIFI_STATE_DEV_Initialized ||
       HCI_WIFI_STATE_CONN(curState) != HCI_WIFI_STATE_CONN_GotIP      ||
       HCI_WIFI_STATE_SCKT(curState) != HCI_WIFI_STATE_SCKT_Opened)
        return;
    
    HciWiFi_SocketSendKernel((uint8_t)c);
    osal_set_event(HciWiFiTaskID, HCI_WIFI_EVT_TxPoll);
}

extern void HciWiFiSocketSendStr(char *str)
{
    if(HCI_WIFI_STATE_DEV(curState)  != HCI_WIFI_STATE_DEV_Initialized ||
       HCI_WIFI_STATE_CONN(curState) != HCI_WIFI_STATE_CONN_GotIP      ||
       HCI_WIFI_STATE_SCKT(curState) != HCI_WIFI_STATE_SCKT_Opened)
        return;
    while(*str)
    {
        HciWiFi_SocketSendKernel((uint8_t)(*str++));
    }
    osal_set_event(HciWiFiTaskID, HCI_WIFI_EVT_TxPoll);
}

extern void HciWiFiSocketSendSint(int32_t num)
{
    char str[SINT_STR_LEN_MAX];
    uint8_t len;
    uint8_t i;
    
    if(HCI_WIFI_STATE_DEV(curState)  != HCI_WIFI_STATE_DEV_Initialized ||
       HCI_WIFI_STATE_CONN(curState) != HCI_WIFI_STATE_CONN_GotIP      ||
       HCI_WIFI_STATE_SCKT(curState) != HCI_WIFI_STATE_SCKT_Opened)
        return;
    
    len = sintstr(num, str);
    
    for(i = 0; i < len; i++)
    {
        HciWiFi_SocketSendKernel((uint8_t)str[i]);
    }
    osal_set_event(HciWiFiTaskID, HCI_WIFI_EVT_TxPoll);
}

extern void HciWiFiSocketSendUint(uint32_t num)
{
    char str[UINT_STR_LEN_MAX];
    uint8_t len;
    uint8_t i;

    if(HCI_WIFI_STATE_DEV(curState)  != HCI_WIFI_STATE_DEV_Initialized ||
       HCI_WIFI_STATE_CONN(curState) != HCI_WIFI_STATE_CONN_GotIP      ||
       HCI_WIFI_STATE_SCKT(curState) != HCI_WIFI_STATE_SCKT_Opened)
        return;
    
    len = uintstr(num, str);
    
    for(i = 0; i < len; i++)
    {
        HciWiFi_SocketSendKernel((uint8_t)str[i]);
    }

    osal_set_event(HciWiFiTaskID, HCI_WIFI_EVT_TxPoll);
}

extern void HciWiFiSocketSendHex8(uint8_t num)
{
    char str[HEX8_STR_LEN_MAX];
    uint8_t len;
    uint8_t i;

    if(HCI_WIFI_STATE_DEV(curState)  != HCI_WIFI_STATE_DEV_Initialized ||
       HCI_WIFI_STATE_CONN(curState) != HCI_WIFI_STATE_CONN_GotIP      ||
       HCI_WIFI_STATE_SCKT(curState) != HCI_WIFI_STATE_SCKT_Opened)
        return;

    len = hex8str(num, str);
    
    for(i = 0; i < len; i++)
    {
        HciWiFi_SocketSendKernel((uint8_t)str[i]);
    }

    osal_set_event(HciWiFiTaskID, HCI_WIFI_EVT_TxPoll);
}

extern void HciWiFiSocketSendHex16(uint16_t num)
{
    char str[HEX16_STR_LEN_MAX];
    uint8_t len;
    uint8_t i;
    
    if(HCI_WIFI_STATE_DEV(curState)  != HCI_WIFI_STATE_DEV_Initialized ||
       HCI_WIFI_STATE_CONN(curState) != HCI_WIFI_STATE_CONN_GotIP      ||
       HCI_WIFI_STATE_SCKT(curState) != HCI_WIFI_STATE_SCKT_Opened)
        return;
    
    len = hex16str(num, str);
    
    for(i = 0; i < len; i++)
    {
        HciWiFi_SocketSendKernel((uint8_t)str[i]);
    }

    osal_set_event(HciWiFiTaskID, HCI_WIFI_EVT_TxPoll);
}

extern void HciWiFiSocketSendHex32(uint32_t num)
{
    char str[HEX32_STR_LEN_MAX];
    uint8_t len;
    uint8_t i;
    
    if(HCI_WIFI_STATE_DEV(curState)  != HCI_WIFI_STATE_DEV_Initialized ||
       HCI_WIFI_STATE_CONN(curState) != HCI_WIFI_STATE_CONN_GotIP      ||
       HCI_WIFI_STATE_SCKT(curState) != HCI_WIFI_STATE_SCKT_Opened)
        return;
    
    len = hex32str(num, str);
    
    for(i = 0; i < len; i++)
    {
        HciWiFi_SocketSendKernel((uint8_t)str[i]);
    }

    osal_set_event(HciWiFiTaskID, HCI_WIFI_EVT_TxPoll);
}


static void HciWiFi_ProcessOsalMsg(HciWiFiMsg_t *pMsg)
{
    switch ( pMsg->head )
    {
        case HCI_WIFI_MSG_HEAD_ATCmd:
            HciWiFiATCoP(((HciWiFiMsgATCmd_t *)pMsg)->pATCmd);
            osal_mem_free(((HciWiFiMsgATCmd_t *)pMsg)->pATCmd);
        break;

        case HCI_WIFI_MSG_HEAD_SSID:
            HAL_ASSERT(pPairingInitStruct != NULL);
            if(pPairingInitStruct->onWiFiPairingUpdateSSID != NULL)
                pPairingInitStruct->onWiFiPairingUpdateSSID(((HciWiFiMsgSSID_t *)pMsg)->ssid);
            osal_mem_free(((HciWiFiMsgSSID_t *)pMsg)->ssid);
        break;

        case HCI_WIFI_MSG_HEAD_PASSWD:
            HAL_ASSERT(pPairingInitStruct != NULL);
            if(pPairingInitStruct->onWiFiPairingUpdatePASSWD != NULL)
                pPairingInitStruct->onWiFiPairingUpdatePASSWD(((HciWiFiMsgPASSWD_t *)pMsg)->passwd);
            osal_mem_free(((HciWiFiMsgPASSWD_t *)pMsg)->passwd);
        break;

        case HCI_WIFI_MSG_HEAD_HciWiFiState:
            HciWiFi_ProcessHciState(((HciWiFiMsgHciWiFiState_t *)pMsg)->hciWiFiState);
        break;

        case HCI_WIFI_MSG_HEAD_RxDataStream:
            HCI_WIFI_STATE_COMM_CLR(HCI_WIFI_STATE_COMM_Rx);
            HAL_ASSERT(pSocketInitStruct != NULL);
            if(pSocketInitStruct->onSocketRxData != NULL)
                pSocketInitStruct->onSocketRxData(((HciWiFiMsgRxDataStream_t *)pMsg)->pData, 
                                                  ((HciWiFiMsgRxDataStream_t *)pMsg)->len);
            //pData can be NULL if large data packet is missed
            if(((HciWiFiMsgRxDataStream_t *)pMsg)->pData)
                osal_mem_free(((HciWiFiMsgRxDataStream_t *)pMsg)->pData);
        break;

        case HCI_WIFI_MSG_HEAD_TxDataStream:
            HciWiFi_ProcessTxDataStream(((HciWiFiMsgRxDataStream_t *)pMsg)->len);
        break;
        
        default:
            HAL_ASSERT_FORCED();
        break;
    }
}


static void HciWiFi_ProcessHciState(uint16_t state)
{
    void (*pfnVoidArg)(void);
    //update the global var
    curState = state;
    
    switch (state)
    {
        case HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                            HCI_WIFI_STATE_SCKT_Closed,
                            HCI_WIFI_STATE_CONN_Disconnected, 
                            HCI_WIFI_STATE_DEV_Initializing):
            HalTerminalPrintStr("Wi-Fi initializing.\r\n");
            //HAL_ASSERT(pConnInitStruct == NULL);
            HAL_ASSERT(pSocketInitStruct == NULL);
            HAL_ASSERT(scktTxFIFO == NULL);
            
            curState = 0x0000;
            HciWiFiCmdSetMode(HCI_WIFI_MODE_Station);
        break;

        case HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                            HCI_WIFI_STATE_SCKT_Closed,
                            HCI_WIFI_STATE_CONN_Connecting,
                            HCI_WIFI_STATE_DEV_Initialized):
            HalTerminalPrintStr("Wi-Fi connecting.\r\n");
            HAL_ASSERT(pConnInitStruct != NULL);
            HAL_ASSERT(pSocketInitStruct == NULL);
            HAL_ASSERT(scktTxFIFO == NULL);
        break;

        case HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                            HCI_WIFI_STATE_SCKT_Closed,
                            HCI_WIFI_STATE_CONN_Connected,
                            HCI_WIFI_STATE_DEV_Initialized):
            HalTerminalPrintStr("Wi-Fi connected, waiting DHCP to assign IP.\r\n");
            HAL_ASSERT(pConnInitStruct != NULL);
            HAL_ASSERT(pSocketInitStruct == NULL);
            HAL_ASSERT(scktTxFIFO == NULL);
        break;

        case HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                            HCI_WIFI_STATE_SCKT_Closed,
                            HCI_WIFI_STATE_CONN_Disconnected,
                            HCI_WIFI_STATE_DEV_Initialized):
            //come from
            //HciWiFiEvtSetModeSuccess()
            //HciWiFiEvtConnectFailure()
            //HciWiFiEvtDisconnected()
            //HAL_ASSERT(pSocketInitStruct == NULL);
            HAL_ASSERT(scktTxFIFO == NULL);

            //this may be caused by a socket CLOSED event followed by a WIFI DISCONNECT event.
            //in other words, the socket CLOSED event's root cause is WIFI DISCONNECT event and 
            //the application predict the WIFI DISCONNECT and send a socket opening request before knowing WIFI DISCONNECT.
            if(pSocketInitStruct != NULL)
            {
                if(pSocketInitStruct->addr != NULL)
                {
                    osal_mem_free(pSocketInitStruct->addr);
                    //HalTerminalPrintStr("Free RAM for socket ADDR.\r\n");
                }
                pfnVoidArg = pSocketInitStruct->onSocketClosed;
                osal_mem_free(pSocketInitStruct);
                pSocketInitStruct = NULL;
                //HalTerminalPrintStr("Free RAM for socket info.\r\n");

                if(pfnVoidArg != NULL)
                {
                    pfnVoidArg();
                }
            }
            
            if(pConnInitStruct != NULL)
            {
                if(pConnInitStruct->onWiFiConnected != NULL)
                {
                    HalTerminalPrintStr("Wi-Fi connect failure.\r\n");
                }
                else
                {
                    HalTerminalPrintStr("Wi-Fi disconnected.\r\n");
                }
                
                //free memory for SSID
                if(pConnInitStruct->ssid != NULL)
                {
                    osal_mem_free(pConnInitStruct->ssid);
                    pConnInitStruct->ssid = NULL;
                    //HalTerminalPrintStr("Free RAM for SSID.\r\n");
                }
                //free memory for Password
                if(pConnInitStruct->passwd != NULL)
                {
                    osal_mem_free(pConnInitStruct->passwd);
                    pConnInitStruct->passwd = NULL;
                    //HalTerminalPrintStr("Free RAM for PASSWD.\r\n");
                }
                //save the function pointer of WiFi disconnect event
                pfnVoidArg = pConnInitStruct->onWiFiDisconnected;
                //free the connection init struct
                osal_mem_free(pConnInitStruct);
                pConnInitStruct = NULL;
                //HalTerminalPrintStr("Free RAM for connection info.\r\n");
                //execute the event callback
                if(pfnVoidArg != NULL)
                {
                    pfnVoidArg();
                }
                
            }
            else
            {
                HalTerminalPrintStr("Wi-Fi initialized.\r\n");
            }

            
        break;

        case HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                            HCI_WIFI_STATE_SCKT_Opening,
                            HCI_WIFI_STATE_CONN_GotIP,
                            HCI_WIFI_STATE_DEV_Initialized):
            HalTerminalPrintStr("Socket connection opening.\r\n");
            //HAL_ASSERT(pConnInitStruct != NULL);
            //HAL_ASSERT(pSocketInitStruct != NULL);
            HAL_ASSERT(scktTxFIFO == NULL);
        break;

        case HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                            HCI_WIFI_STATE_SCKT_Opened,
                            HCI_WIFI_STATE_CONN_GotIP,
                            HCI_WIFI_STATE_DEV_Initialized):
            //come from HciWiFiEvtOpenSocketSuccess()
            HAL_ASSERT(pConnInitStruct != NULL);
            HAL_ASSERT(pSocketInitStruct != NULL);
            HAL_ASSERT(scktTxFIFO == NULL);
            HalTerminalPrintStr("Socket connection opened.\r\n");
            if(pSocketInitStruct->addr != NULL)
            {
                osal_mem_free(pSocketInitStruct->addr);
                pSocketInitStruct->addr = NULL;
                //HalTerminalPrintStr("Free RAM for socket ADDR.\r\n");
            }
            pfnVoidArg = pSocketInitStruct->onSocketOpened;
            pSocketInitStruct->onSocketOpened = NULL;
            if(pfnVoidArg != NULL)
            {
                pfnVoidArg();
            }
            
        break;

        case HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                            HCI_WIFI_STATE_SCKT_Closed,
                            HCI_WIFI_STATE_CONN_GotIP,
                            HCI_WIFI_STATE_DEV_Initialized):
            //come from
            //HciWiFiEvtConnectSuccess()
            //HciWiFiEvtOpenSocketFailure()
            //HciWiFiEvtCloseSocketSuccess()
            //HciWiFiEvtSocketClosed()
            //HciWiFiEvtTxDataStreamFailure()
            if(pConnInitStruct != NULL)
            {
                if(pConnInitStruct->onWiFiConnected != NULL)
                {
                    HalTerminalPrintStr("Wi-Fi connected.\r\n");
                    //free memory for SSID
                    if(pConnInitStruct->ssid != NULL)
                    {
                        osal_mem_free(pConnInitStruct->ssid);
                        pConnInitStruct->ssid = NULL;
                        //HalTerminalPrintStr("Free RAM for SSID.\r\n");
                    }
                    //free memory for Password
                    if(pConnInitStruct->passwd != NULL)
                    {
                        osal_mem_free(pConnInitStruct->passwd);
                        pConnInitStruct->passwd = NULL;
                        //HalTerminalPrintStr("Free RAM for PASSWD.\r\n");
                    }
                    //save the function pointer of WiFi disconnect event
                    pfnVoidArg = pConnInitStruct->onWiFiConnected;
                    pConnInitStruct->onWiFiConnected = NULL;
                    //execute the event callback
                    if(pfnVoidArg != NULL)
                    {
                        pfnVoidArg();
                    }
                }
            }
            
            if(pSocketInitStruct != NULL)
            {
                if(pSocketInitStruct->onSocketOpened != NULL)
                {
                    HalTerminalPrintStr("Socket connection open failure.\r\n");
                }
                else
                {
                    HalTerminalPrintStr("Socket connection closed.\r\n");
                }
                
                if(pSocketInitStruct->addr != NULL)
                {
                    osal_mem_free(pSocketInitStruct->addr);
                    //HalTerminalPrintStr("Free RAM for socket ADDR.\r\n");
                }
                pfnVoidArg = pSocketInitStruct->onSocketClosed;
                osal_mem_free(pSocketInitStruct);
                pSocketInitStruct = NULL;
                //HalTerminalPrintStr("Free RAM for socket info.\r\n");
                
                if(pfnVoidArg != NULL)
                    pfnVoidArg();

                
            }

            //free the TX fifo
            if(scktTxFIFO != NULL)
            {
                osal_fifo_delete(scktTxFIFO);
                scktTxFIFO = NULL;
                //HalTerminalPrintStr("Free RAM for TX FIFO.\r\n");
            }
            
        break;

        case HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                            HCI_WIFI_STATE_SCKT_Closing,
                            HCI_WIFI_STATE_CONN_GotIP,
                            HCI_WIFI_STATE_DEV_Initialized):
            HalTerminalPrintStr("Closing socket connection.\r\n");
            HAL_ASSERT(pConnInitStruct != NULL);
            HAL_ASSERT(pSocketInitStruct != NULL);
            //free the TX fifo ASAP
            if(scktTxFIFO != NULL)
            {
                osal_fifo_delete(scktTxFIFO);
                scktTxFIFO = NULL;
                //HalTerminalPrintStr("Free RAM for TX FIFO.\r\n");
            }
        break;

        case HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                            HCI_WIFI_STATE_SCKT_Closed,
                            HCI_WIFI_STATE_CONN_PairingWaiting,
                            HCI_WIFI_STATE_DEV_Initialized):
            HalTerminalPrintStr("Wi-Fi pairing waiting.\r\n");
            HAL_ASSERT(pPairingInitStruct != NULL);
            if(pPairingInitStruct->onWiFiPairingWaiting)
                pPairingInitStruct->onWiFiPairingWaiting();
//            if(osal_get_timeoutEx(HciWiFiTaskID, HCI_WIFI_EVT_PairingTimeout) != 0)
//                osal_stop_timerEx(HciWiFiTaskID, HCI_WIFI_EVT_PairingTimeout);
//            osal_start_timerEx(HciWiFiTaskID, HCI_WIFI_EVT_PairingTimeout, HCI_WIFI_CONFIG_PairingTimeout);
        break;

        case HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                            HCI_WIFI_STATE_SCKT_Closed,
                            HCI_WIFI_STATE_CONN_PairingStarted,
                            HCI_WIFI_STATE_DEV_Initialized):
            HalTerminalPrintStr("Wi-Fi pairing started.\r\n");
        break;

        case HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                            HCI_WIFI_STATE_SCKT_Closed,
                            HCI_WIFI_STATE_CONN_PairingSuccess,
                            HCI_WIFI_STATE_DEV_Initialized):
            HalTerminalPrintStr("Wi-Fi pairing success.\r\n");
            //stop Wi-Fi smart config to release RAM resources
            /*
            tarState = HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                                      HCI_WIFI_STATE_SCKT_Closed,
                                      HCI_WIFI_STATE_CONN_PairingStopped,
                                      HCI_WIFI_STATE_DEV_Initialized);
            */
            curState = 0x0000;
            HciWiFiCmdSmartConfigStop();
            HAL_ASSERT(pPairingInitStruct != NULL);
            pPairingInitStruct->onWiFiPairingFailure = NULL;
//            if(osal_get_timeoutEx(HciWiFiTaskID, HCI_WIFI_EVT_PairingTimeout) != 0)
//                osal_stop_timerEx(HciWiFiTaskID, HCI_WIFI_EVT_PairingTimeout);
        break;

        case HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                            HCI_WIFI_STATE_SCKT_Closed,
                            HCI_WIFI_STATE_CONN_PairingFailure,
                            HCI_WIFI_STATE_DEV_Initialized):
            HalTerminalPrintStr("Wi-Fi pairing failure.\r\n");
            //stop Wi-Fi smart config to release RAM resources
            /*
            tarState = HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                                      HCI_WIFI_STATE_SCKT_Closed,
                                      HCI_WIFI_STATE_CONN_PairingStopped,
                                      HCI_WIFI_STATE_DEV_Initialized);
            */
            curState = 0x0000;
            HciWiFiCmdSmartConfigStop();
            HAL_ASSERT(pPairingInitStruct != NULL);
            pPairingInitStruct->onWiFiPairingSuccess = NULL;
//            if(osal_get_timeoutEx(HciWiFiTaskID, HCI_WIFI_EVT_PairingTimeout) != 0)
//                osal_stop_timerEx(HciWiFiTaskID, HCI_WIFI_EVT_PairingTimeout);
        break;

        case HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                            HCI_WIFI_STATE_SCKT_Closed,
                            HCI_WIFI_STATE_CONN_PairingStopped,
                            HCI_WIFI_STATE_DEV_Initialized):
            HalTerminalPrintStr("Wi-Fi pairing stopped.\r\n");
            
            if(pPairingInitStruct->onWiFiPairingSuccess != NULL && 
               pPairingInitStruct->onWiFiPairingFailure == NULL)
            {
                //pairing success stop
                pfnVoidArg = pPairingInitStruct->onWiFiPairingSuccess;
                //fix the current Wi-Fi state, should be Wi-Fi connected
                curState = 0x0000;
                HciWiFiCmdDisconnect();
            }
            else if(pPairingInitStruct->onWiFiPairingFailure != NULL &&
                    pPairingInitStruct->onWiFiPairingSuccess == NULL)
            {
                //pairing failure stop
                pfnVoidArg = pPairingInitStruct->onWiFiPairingFailure;
            }
            else if(pPairingInitStruct->onWiFiPairingFailure != NULL &&
                    pPairingInitStruct->onWiFiPairingSuccess != NULL)
            {
                //pairing timeout stop
                pfnVoidArg = pPairingInitStruct->onWiFiPairingFailure;
            }
            else
            {
                HAL_ASSERT_FORCED();
            }
            
            
            osal_mem_free(pPairingInitStruct);
            pPairingInitStruct = NULL;
            //HalTerminalPrintStr("Free RAM for pairing info.\r\n");
            if(pfnVoidArg != NULL)
            {
                pfnVoidArg();
            }
        break;

        case HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                            HCI_WIFI_STATE_SCKT_Closed,
                            HCI_WIFI_STATE_CONN_Disconnecting,
                            HCI_WIFI_STATE_DEV_Initialized):
            HalTerminalPrintStr("Wi-Fi disconnecting.\r\n");
        break;

        case HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                            HCI_WIFI_STATE_SCKT_Opening, 
                            HCI_WIFI_STATE_CONN_Disconnected, 
                            HCI_WIFI_STATE_DEV_Initialized):
            HalTerminalPrintStr("Wi-Fi disconnected when opening socket.\r\n");
        break;
        
        default:
            //unknown Hci WiFi State must be handled
            HAL_ASSERT_FORCED();
        break;
    }

    
    if(curState != tarState)
    {
        //if not achieved target state and current state is confirmed (not 0x0000)
        if(tarState != 0x0000 && curState != 0x0000)
            osal_set_event(HciWiFiTaskID, HCI_WIFI_EVT_CheckHciWiFiState);
    }
    else
    {
        //job done, clear target state
        tarState = 0x0000;
        osal_clear_event(HciWiFiTaskID, HCI_WIFI_EVT_CheckHciWiFiState);
    }
    
}

static void HciWiFi_StateController(uint16_t *cur_state, uint16_t *tar_state)
{
    if(*cur_state == *tar_state)
        return;
    
    switch (*tar_state)
    {
        //Set Wi-Fi to connect to AP. Or,
        //Set socket to disconnet from server
        case HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                            HCI_WIFI_STATE_SCKT_Closed, 
                            HCI_WIFI_STATE_CONN_GotIP, 
                            HCI_WIFI_STATE_DEV_Initialized):
            //Check device state
            if(HCI_WIFI_STATE_DEV(*cur_state) == HCI_WIFI_STATE_DEV_Initialized)
            {
                //check Wi-Fi connection state
                if(HCI_WIFI_STATE_CONN(*cur_state) == HCI_WIFI_STATE_CONN_Disconnected ||
                   HCI_WIFI_STATE_CONN(*cur_state) == HCI_WIFI_STATE_CONN_PairingStopped)
                {
                    //start Wi-Fi connection under these states
                    if(pConnInitStruct != NULL)
                    {
                        *cur_state = 0x0000;
                        HciWiFiCmdConnect(pConnInitStruct->ssid, pConnInitStruct->passwd);
                    }
                }
                else if(HCI_WIFI_STATE_CONN(*cur_state) == HCI_WIFI_STATE_CONN_GotIP)
                {
                    //check socket state
                    if(HCI_WIFI_STATE_SCKT(*cur_state) == HCI_WIFI_STATE_SCKT_Opened)
                    {
                        //check communication state
                        if(HCI_WIFI_STATE_COMM(*cur_state) == HCI_WIFI_STATE_COMM_Idle)
                        {
                            //close socket connection in this state
                            *cur_state = 0x0000;
                            HciWiFiCmdSocketClose();
                        }
                    }
                    else
                    {
                        //just wait, do nothing
                    }
                }
                else
                {
                    
                }
            }
            else
            {
                //just wait, do nothing
            }
        break;

        //Start Wi-Fi pairing mode
        case HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                            HCI_WIFI_STATE_SCKT_Closed, 
                            HCI_WIFI_STATE_CONN_PairingWaiting, 
                            HCI_WIFI_STATE_DEV_Initialized):
            //check device state
            if(HCI_WIFI_STATE_DEV(*cur_state) == HCI_WIFI_STATE_DEV_Initialized)
            {
                //check Wi-Fi connection state
                if(HCI_WIFI_STATE_CONN(*cur_state) == HCI_WIFI_STATE_CONN_Disconnected ||
                   HCI_WIFI_STATE_CONN(*cur_state) == HCI_WIFI_STATE_CONN_PairingStopped)
                {
                    *cur_state = 0x0000;
                    HciWiFiCmdSmartConfigStart();
                }
                else if(HCI_WIFI_STATE_CONN(*cur_state) == HCI_WIFI_STATE_CONN_GotIP)
                {
                    //check socket connection state
                    if(HCI_WIFI_STATE_SCKT(*cur_state) == HCI_WIFI_STATE_SCKT_Closed)
                    {
                        *cur_state = 0x0000;
                        HciWiFiCmdDisconnect();
                    }
                    else if(HCI_WIFI_STATE_SCKT(*cur_state) == HCI_WIFI_STATE_SCKT_Opened)
                    {
                        //check communication state
                        if(HCI_WIFI_STATE_COMM(*cur_state) == HCI_WIFI_STATE_COMM_Idle)
                        {
                            //close socket connection
                            *cur_state = 0x0000;
                            HciWiFiCmdSocketClose();
                        }
                        else
                        {
                            //just ignore and wait
                        }
                    }
                    else
                    {
                        //Wi-Fi state can be
                        //HCI_WIFI_STATE_SCKT_Closing
                        //HCI_WIFI_STATE_SCKT_Opening
                        //just ignore and wait
                    }
                }
                else
                {
                    //just ignore and wait
                }
            }
            else
            {
                //just ignore and wait
            }
        break;

        //Open socket
        case HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                            HCI_WIFI_STATE_SCKT_Opened, 
                            HCI_WIFI_STATE_CONN_GotIP, 
                            HCI_WIFI_STATE_DEV_Initialized):
            if(*cur_state == HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                                          HCI_WIFI_STATE_SCKT_Closed, 
                                          HCI_WIFI_STATE_CONN_GotIP, 
                                          HCI_WIFI_STATE_DEV_Initialized))
            {
                if(pSocketInitStruct != NULL)
                {
                    *cur_state = 0x0000;
                    HciWiFiCmdSocketOpen(HCI_WIFI_CONN_TYPE_TCP, pSocketInitStruct->addr, pSocketInitStruct->port);
                }
            }
             
        break;

        //Disconnect Wi-Fi from AP
        case HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                            HCI_WIFI_STATE_SCKT_Closed, 
                            HCI_WIFI_STATE_CONN_Disconnected, 
                            HCI_WIFI_STATE_DEV_Initialized):
            if(HCI_WIFI_STATE_DEV(*cur_state) == HCI_WIFI_STATE_DEV_Initialized)
            {
                if(HCI_WIFI_STATE_CONN(*cur_state) == HCI_WIFI_STATE_CONN_GotIP)
                {
                    if(HCI_WIFI_STATE_SCKT(*cur_state) == HCI_WIFI_STATE_SCKT_Closed)
                    {
                        *cur_state = 0x0000;
                        HciWiFiCmdDisconnect();
                    }
                    else if(HCI_WIFI_STATE_SCKT(*cur_state) == HCI_WIFI_STATE_SCKT_Opened)
                    {
                        if(HCI_WIFI_STATE_COMM(*cur_state) == HCI_WIFI_STATE_COMM_Idle)
                        {
                            *cur_state = 0x0000;
                            HciWiFiCmdSocketClose();
                        }
                        else
                        {
                            //just ignore and wait
                        }
                    }
                    else
                    {
                        //just ignore and wait
                    }
                }
                else
                {
                    //just ignore and wait
                }
            }
            else
            {
                //do nothing just wait
            }
        break;
        
        default:
            HAL_ASSERT_FORCED();
        break;
    }
}

static void HciWiFi_ProcessTxDataStream(uint16_t len)
{
    uint16_t i;
    uint8_t u8tmp;
    
    HAL_ASSERT(scktTxFIFO != NULL);
    HAL_ASSERT((uint16_t)osal_fifo_len(scktTxFIFO) >= len);

    for(i = 0; i < len; i++)
    {
        u8tmp = osal_fifo_get(scktTxFIFO);
        HalATCmdSIOSendByte(u8tmp);
    }

    //free the FIFO buffer ASAP
    if((uint16_t)osal_fifo_len(scktTxFIFO) == 0)
    {
        osal_fifo_delete(scktTxFIFO);
        scktTxFIFO = NULL;
        //HalTerminalPrintStr("Free RAM for TX FIFO.\r\n");
    }
}

static void HciWiFi_SocketSendKernel(uint8_t byte)
{
    if(scktTxFIFO == NULL)
        scktTxFIFO = osal_fifo_create();
    HAL_ASSERT(scktTxFIFO != NULL);
    HAL_ASSERT(osal_fifo_put(scktTxFIFO, byte) != NULL);
}


