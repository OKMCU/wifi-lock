#include <stdio.h>
#include <string.h>
#include "stringx.h"
#include "osal.h"

#include "hal_assert.h"
#include "hal_terminal.h"

#include "hci_wifi.h"
#include "hci_wifiatcop.h"


typedef char *(*ATCmdProcessor_t)(char *pMsg);
static ATCmdProcessor_t pfnATCmdProcessor = NULL;
static uint16_t txLen = 0;
static uint16_t hciWiFiState = 0x0000;

#define HCI_WIFI_SET_STATE(comm, link, conn, dev)       st(hciWiFiState = HCI_WIFI_STATE(comm, link, conn, dev);)
#define HCI_WIFI_SET_STATE_DEV(state)                   st(hciWiFiState &= 0xFFF0; hciWiFiState |= state;)
#define HCI_WIFI_SET_STATE_CONN(state)                  st(hciWiFiState &= 0xFF0F; hciWiFiState |= (state<<4);)
#define HCI_WIFI_SET_STATE_SCKT(state)                  st(hciWiFiState &= 0xF0FF; hciWiFiState |= (state<<8);)
#define HCI_WIFI_SET_STATE_COMM(state)                  st(hciWiFiState &= 0x0FFF; hciWiFiState |= (state<<12);)

static char *ATCmdProcessor_Generic         (char *pMsg);
static char *ATCmdProcessor_CWMODE_CUR      (char *pMsg);
static char *ATCmdProcessor_CWJAP_CUR       (char *pMsg);
static char *ATCmdProcessor_CWSTARTSMART    (char *pMsg);
static char *ATCmdProcessor_CWSTOPSMART     (char *pMsg);
static char *ATCmdProcessor_CIPSTART        (char *pMsg);
static char *ATCmdProcessor_CIPSEND         (char *pMsg);
static char *ATCmdProcessor_CIPCLOSE        (char *pMsg);
static char *ATCmdProcessor_CWQAP           (char *pMsg);
static char *ATCmdProcessor_SmartConfig     (char *pMsg);
static char *ATCmdProcessor_TxDataStream    (char *pMsg);

static void HciWiFiEvtSmartConfigUpdateSSID(const char *ssid);
static void HciWiFiEvtSmartConfigUpdatePASSWD(const char *passwd);
static void HciWiFiEvtTxDataStreamStart(uint16_t txLen);
static void HciWiFiEvtTxDataStreamSuccess(void);
static void HciWiFiSendState(uint16_t state);


extern void HciWiFiATCoP(char *pATCmdMsg)
{
    char *processedMsg = NULL;
    
    //check if there's a specified processor
    if(pfnATCmdProcessor != NULL)
        processedMsg = pfnATCmdProcessor(pATCmdMsg);

    //if the message is not processed by the specified processor, try the generic one
    if(processedMsg == NULL)
        processedMsg = ATCmdProcessor_Generic(pATCmdMsg);

    //all message must be processed
    HAL_ASSERT(processedMsg != NULL);
}

static char *ATCmdProcessor_Generic(char *pMsg)
{
    HciWiFiMsgRxDataStream_t *msg;
    char *str;
    uint8_t *pu8tmp;
    uint32_t u32tmp;
    
    if(strcmp(pMsg, "\r\n") == 0)
    {
        //ignore
        return pMsg;
    }
    
    str = strStartsWith(pMsg, "+IPD,");
    if(str)
    {
        msg = (HciWiFiMsgRxDataStream_t *)osal_msg_allocate(sizeof(HciWiFiMsgRxDataStream_t));
        HAL_ASSERT(msg != NULL);
        msg->head = HCI_WIFI_MSG_HEAD_RxDataStream;
        
        
        pu8tmp = (uint8_t *)strchr(str, ':');
        *pu8tmp = (uint8_t)'\0';
        pu8tmp++;
        HAL_ASSERT(decstr2uint(str, &u32tmp) != 0);
        msg->len = (uint16_t)u32tmp;

        u32tmp = *pu8tmp++;
        u32tmp <<= 8;
        u32tmp |= *pu8tmp++;
        u32tmp <<= 8;
        u32tmp |= *pu8tmp++;
        u32tmp <<= 8;
        u32tmp |= *pu8tmp++;
        
        msg->pData = (uint8_t *)u32tmp;
        osal_msg_send(HciWiFiTaskID, (uint8_t *)msg);
        //HciWiFiEvtRxDataStream((uint8_t *)ipdData, (uint16_t)u32tmp);
        
        return pMsg;
    }
    
    if(strcmp(pMsg, "rl") == 0)
    {
        return pMsg;
    }

    if(strcmp(pMsg, "ready\r\n") == 0)
    {
        HCI_WIFI_SET_STATE_DEV(HCI_WIFI_STATE_DEV_Initializing);
        HciWiFiSendState(hciWiFiState);
        return pMsg;
    }

    if(strcmp(pMsg, "WIFI CONNECTED\r\n") == 0)
    {
        HCI_WIFI_SET_STATE_CONN(HCI_WIFI_STATE_CONN_Connected);
        HciWiFiSendState(hciWiFiState);
        return pMsg;
    }

    
    if(strcmp(pMsg, "WIFI GOT IP\r\n") == 0)
    {
        HCI_WIFI_SET_STATE_CONN(HCI_WIFI_STATE_CONN_GotIP);
        HciWiFiSendState(hciWiFiState);
        return pMsg;
    }
    
    if(strcmp(pMsg, "WIFI DISCONNECT\r\n") == 0)
    {
        HCI_WIFI_SET_STATE_CONN(HCI_WIFI_STATE_CONN_Disconnected);
        HciWiFiSendState(hciWiFiState);
        return pMsg;
    }

    if(strcmp(pMsg, "CLOSED\r\n") == 0)
    {
        HCI_WIFI_SET_STATE_SCKT(HCI_WIFI_STATE_SCKT_Closed);
        HciWiFiSendState(hciWiFiState);
        return pMsg;
    }

    if(strcmp(pMsg, "busy p...\r\n") == 0)
    {
        return pMsg;
    }

    if(pfnATCmdProcessor == NULL)
    {
        if(strStartsWith(pMsg, "AT+CWMODE_CUR="))
        {
            pfnATCmdProcessor = ATCmdProcessor_CWMODE_CUR;
            return pMsg;
        }

        if(strStartsWith(pMsg, "AT+CWJAP_CUR="))
        {
            pfnATCmdProcessor = ATCmdProcessor_CWJAP_CUR;
            HCI_WIFI_SET_STATE_CONN(HCI_WIFI_STATE_CONN_Connecting);
            HciWiFiSendState(hciWiFiState);
            return pMsg;
        }

        if(strStartsWith(pMsg, "AT+CWSTARTSMART="))
        {
            pfnATCmdProcessor = ATCmdProcessor_CWSTARTSMART;
            return pMsg;
        }

        if(strStartsWith(pMsg, "AT+CIPSTART="))
        {
            pfnATCmdProcessor = ATCmdProcessor_CIPSTART;
            HCI_WIFI_SET_STATE_SCKT(HCI_WIFI_STATE_SCKT_Opening);
            HciWiFiSendState(hciWiFiState);
            return pMsg;
        }

        str = strStartsWith(pMsg, "AT+CIPSEND=");
        if(str)
        {
            pfnATCmdProcessor = ATCmdProcessor_CIPSEND;
            //replace the '\r' to '\0'
            *strchr(str, '\r') = '\0';
            HAL_ASSERT(decstr2uint(str, &u32tmp) != 0);
            txLen = (uint16_t)u32tmp;
            return pMsg;
        }

        if(strStartsWith(pMsg, "smartconfig type:"))
        {
            pfnATCmdProcessor = ATCmdProcessor_SmartConfig;
            HCI_WIFI_SET_STATE_CONN(HCI_WIFI_STATE_CONN_PairingStarted);
            HciWiFiSendState(hciWiFiState);
            return pMsg;
        }

        if(strcmp(pMsg, "AT+CIPCLOSE\r\n") == 0)
        {
            pfnATCmdProcessor = ATCmdProcessor_CIPCLOSE;
            HCI_WIFI_SET_STATE_SCKT(HCI_WIFI_STATE_SCKT_Closing);
            HciWiFiSendState(hciWiFiState);
            return pMsg;
        }

        if(strcmp(pMsg, "AT+CWSTOPSMART\r\n") == 0)
        {
            pfnATCmdProcessor = ATCmdProcessor_CWSTOPSMART;
            return pMsg;
        }

        if(strcmp(pMsg, "AT+CWQAP\r\n") == 0)
        {
            pfnATCmdProcessor = ATCmdProcessor_CWQAP;
            HCI_WIFI_SET_STATE_CONN(HCI_WIFI_STATE_CONN_Disconnecting);
            HciWiFiSendState(hciWiFiState);
            return pMsg;
        }
        
    }
    
    return NULL;
}

static char *ATCmdProcessor_CWMODE_CUR(char *pMsg)
{
    if(strcmp(pMsg, "\r\n") == 0)
    {
        return pMsg;
    }
    
    if(strcmp(pMsg, "OK\r\n") == 0)
    {
        pfnATCmdProcessor = NULL;
        HCI_WIFI_SET_STATE_DEV(HCI_WIFI_STATE_DEV_Initialized);
        HciWiFiSendState(hciWiFiState);
        return pMsg;
    }
    
    return NULL;
}

static char *ATCmdProcessor_CWJAP_CUR(char *pMsg)
{
    if(strcmp(pMsg, "\r\n") == 0)
    {
        return pMsg;
    }
    
    if(strcmp(pMsg, "WIFI CONNECTED\r\n") == 0)
    {
        HCI_WIFI_SET_STATE_CONN(HCI_WIFI_STATE_CONN_Connected);
        return pMsg;
    }

    if(strcmp(pMsg, "WIFI GOT IP\r\n") == 0)
    {
        HCI_WIFI_SET_STATE_CONN(HCI_WIFI_STATE_CONN_GotIP);
        return pMsg;
    }

    if(strcmp(pMsg, "OK\r\n") == 0)
    {
        pfnATCmdProcessor = NULL;
        HciWiFiSendState(hciWiFiState);
        return pMsg;
    }

    if(strcmp(pMsg, "WIFI DISCONNECT\r\n") == 0)
    {
        //the ESP8266 Wi-Fi module does not reply this message
        HCI_WIFI_SET_STATE_CONN(HCI_WIFI_STATE_CONN_Disconnected);
        return pMsg;
    }

    if(strStartsWith(pMsg, "+CWJAP:"))
    {
        HCI_WIFI_SET_STATE_CONN(HCI_WIFI_STATE_CONN_Disconnected);
        return pMsg;
    }

    if(strcmp(pMsg, "FAIL\r\n") == 0)
    {
        pfnATCmdProcessor = NULL;
        HCI_WIFI_SET_STATE_CONN(HCI_WIFI_STATE_CONN_Disconnected);
        HciWiFiSendState(hciWiFiState);
        return pMsg;
    }
    
    return NULL;
}

static char *ATCmdProcessor_CWSTARTSMART(char *pMsg)
{
    
    if(strcmp(pMsg, "\r\n") == 0)
    {
        return pMsg;
    }
    
    if(strcmp(pMsg, "OK\r\n") == 0)
    {
        pfnATCmdProcessor = ATCmdProcessor_SmartConfig;
        HCI_WIFI_SET_STATE_CONN(HCI_WIFI_STATE_CONN_PairingWaiting);
        HciWiFiSendState(hciWiFiState);
        //Start the pairing timeout timer
        osal_start_timerEx(HciWiFiTaskID, HCI_WIFI_EVT_PairingTimeout, HCI_WIFI_CONFIG_PairingTimeout);
        return pMsg;
    }
    
    return NULL;
}

static char *ATCmdProcessor_CWSTOPSMART(char *pMsg)
{
    if(strcmp(pMsg, "\r\n") == 0)
    {
        return pMsg;
    }

    
    if(strcmp(pMsg, "OK\r\n") == 0)
    {
        pfnATCmdProcessor = NULL;
        HCI_WIFI_SET_STATE_CONN(HCI_WIFI_STATE_CONN_PairingStopped);
        HciWiFiSendState(hciWiFiState);
        return pMsg;
    }

    return NULL;
}

static char *ATCmdProcessor_CIPSTART(char *pMsg)
{
    static char *errReason = NULL;

    
    if(strcmp(pMsg, "\r\n") == 0)
    {
        return pMsg;
    }
    
    if(strcmp(pMsg, "CONNECT\r\n") == 0)
    {
        HCI_WIFI_SET_STATE_SCKT(HCI_WIFI_STATE_SCKT_Opened);
        return pMsg;
    }

    if(strcmp(pMsg, "OK\r\n") == 0)
    {
        pfnATCmdProcessor = NULL;
        errReason = NULL;
        HciWiFiSendState(hciWiFiState);
        
        return pMsg;
    }
    
    if(strcmp(pMsg, "no ip\r\n") == 0)
    {
        errReason = "no ip\r\n"; 
        return pMsg;
    }

    if(strcmp(pMsg, "DNS Fail\r\n") == 0)
    {
        errReason = "DNS Fail\r\n";
        return pMsg;
    }
    
    if(strcmp(pMsg, "ERROR\r\n") == 0)
    {
        HCI_WIFI_SET_STATE_SCKT(HCI_WIFI_STATE_SCKT_Closed);
        if(errReason != NULL)
        {
            pfnATCmdProcessor = NULL;
            errReason = NULL;
            HciWiFiSendState(hciWiFiState); 
        }
        return pMsg;
    }

    if(strcmp(pMsg, "CLOSED\r\n") == 0)
    {
        pfnATCmdProcessor = NULL;
        errReason = NULL; 
        HCI_WIFI_SET_STATE_SCKT(HCI_WIFI_STATE_SCKT_Closed);
        HciWiFiSendState(hciWiFiState);
        return pMsg;
    }

    return NULL;
}

/*
if(strcmp(pMsg, ) == 0)
{
    
    return pMsg;
}
*/
static char *ATCmdProcessor_CIPSEND(char *pMsg)
{
    if(strcmp(pMsg, "\r\n") == 0)
    {
        return pMsg;
    }
    
    if(strcmp(pMsg, "OK\r\n") == 0)
    {
        pfnATCmdProcessor = ATCmdProcessor_TxDataStream;
        HciWiFiEvtTxDataStreamStart(txLen);
        return pMsg;
    }

    return NULL;
}

static char *ATCmdProcessor_CIPCLOSE(char *pMsg)
{
    if(strcmp(pMsg, "OK\r\n") == 0)
    {
        pfnATCmdProcessor = NULL;
        HCI_WIFI_SET_STATE_SCKT(HCI_WIFI_STATE_SCKT_Closed);
        HciWiFiSendState(hciWiFiState);
        return pMsg;
    }

    /*
    if(strcmp(pMsg, "ERROR\r\n") == 0)
    {
        pfnATCmdProcessor = NULL;
        HAL_ASSERT_FORCED();
        return pMsg;
    }
    */
    
    return NULL;
}

static char *ATCmdProcessor_CWQAP(char *pMsg)
{
    if(strcmp(pMsg, "\r\n") == 0)
    {
        return pMsg;
    }

    if(strcmp(pMsg, "AT+CWQAP\r\n") == 0)
    {
        return pMsg;
    }
    
    if(strcmp(pMsg, "OK\r\n") == 0)
    {
        pfnATCmdProcessor = NULL;
        //expected to received "WIFI DISCONNECT\r\n" later on
        return pMsg;
    }

    return NULL;
}

static char *ATCmdProcessor_SmartConfig(char *pMsg)
{
    char *str;
    
    if(strStartsWith(pMsg, "smartconfig type:"))
    {
        osal_stop_timerEx(HciWiFiTaskID, HCI_WIFI_EVT_PairingTimeout);
        osal_start_timerEx(HciWiFiTaskID, HCI_WIFI_EVT_PairingTimeout, HCI_WIFI_CONFIG_PairingTimeout);
        return pMsg;
    }

    if(strcmp(pMsg, "Smart get wifi info\r\n") == 0)
    {
        osal_stop_timerEx(HciWiFiTaskID, HCI_WIFI_EVT_PairingTimeout);
        osal_start_timerEx(HciWiFiTaskID, HCI_WIFI_EVT_PairingTimeout, HCI_WIFI_CONFIG_PairingTimeout);
        return pMsg;
    }

    str = strStartsWith(pMsg, "ssid:");
    if(str)
    {
        *strchr(str, '\r') = '\0';
        HciWiFiEvtSmartConfigUpdateSSID(str);
        osal_stop_timerEx(HciWiFiTaskID, HCI_WIFI_EVT_PairingTimeout);
        osal_start_timerEx(HciWiFiTaskID, HCI_WIFI_EVT_PairingTimeout, HCI_WIFI_CONFIG_PairingTimeout);
        return pMsg;
    }

    str = strStartsWith(pMsg, "password:");
    if(str)
    {
        *strchr(str, '\r') = '\0';
        HciWiFiEvtSmartConfigUpdatePASSWD(str);
        osal_stop_timerEx(HciWiFiTaskID, HCI_WIFI_EVT_PairingTimeout);
        osal_start_timerEx(HciWiFiTaskID, HCI_WIFI_EVT_PairingTimeout, HCI_WIFI_CONFIG_PairingTimeout);
        return pMsg;
    }

    if(strcmp(pMsg, "WIFI CONNECTED\r\n") == 0)
    {
        osal_stop_timerEx(HciWiFiTaskID, HCI_WIFI_EVT_PairingTimeout);
        osal_start_timerEx(HciWiFiTaskID, HCI_WIFI_EVT_PairingTimeout, HCI_WIFI_CONFIG_PairingTimeout);
        return pMsg;
    }

    if(strcmp(pMsg, "WIFI GOT IP\r\n") == 0)
    {
        osal_stop_timerEx(HciWiFiTaskID, HCI_WIFI_EVT_PairingTimeout);
        osal_start_timerEx(HciWiFiTaskID, HCI_WIFI_EVT_PairingTimeout, HCI_WIFI_CONFIG_PairingTimeout);
        return pMsg;
    }

    if(strcmp(pMsg, "smartconfig connected wifi\r\n") == 0)
    {
        pfnATCmdProcessor = NULL;
        HCI_WIFI_SET_STATE_CONN(HCI_WIFI_STATE_CONN_PairingSuccess);
        HciWiFiSendState(hciWiFiState);
        osal_stop_timerEx(HciWiFiTaskID, HCI_WIFI_EVT_PairingTimeout);
        return pMsg;
    }

    if(strStartsWith(pMsg, "DHCP"))
    {
        osal_stop_timerEx(HciWiFiTaskID, HCI_WIFI_EVT_PairingTimeout);
        osal_start_timerEx(HciWiFiTaskID, HCI_WIFI_EVT_PairingTimeout, HCI_WIFI_CONFIG_PairingTimeout);
        return pMsg;
    }
    
    if(strcmp(pMsg, "WIFI DISCONNECT\r\n") == 0)
    {
        pfnATCmdProcessor = NULL;
        HCI_WIFI_SET_STATE_CONN(HCI_WIFI_STATE_CONN_PairingFailure);
        HciWiFiSendState(hciWiFiState);
        osal_stop_timerEx(HciWiFiTaskID, HCI_WIFI_EVT_PairingTimeout);
        return pMsg;
    }

    if(strcmp(pMsg, "AT+CWSTOPSMART\r\n") == 0)
    {
        //this message can be received when pairing is timeout
        //just clear the AT command processor and returns NULL
        //then, a new AT command processor will be assigned in the generic handler.
        pfnATCmdProcessor = NULL;
        return NULL;
    }
    
    //HAL_ASSERT_FORCED();
    //pfnATCmdProcessor = NULL;
    
    return NULL;
}

static char *ATCmdProcessor_TxDataStream(char *pMsg)
{
    if(strcmp(pMsg, "\r\n") == 0)
    {
        return pMsg;
    }
    
    if(strcmp(pMsg, "> \r\n") == 0)
    {
        return pMsg;
    }

    if(strStartsWith(pMsg, "Recv "))
    {
        return pMsg;
    }

    if(strcmp(pMsg, "SEND OK\r\n") == 0)
    {
        pfnATCmdProcessor = NULL;
        HciWiFiEvtTxDataStreamSuccess();
        return pMsg;
    }

    if(strcmp(pMsg, "CLOSED\r\n") == 0)
    {
        HCI_WIFI_SET_STATE_SCKT(HCI_WIFI_STATE_SCKT_Closed);
        return pMsg;
    }

    if(strcmp(pMsg, "SEND FAIL\r\n") == 0)
    {
        pfnATCmdProcessor = NULL;
        HciWiFiSendState(hciWiFiState);
        return pMsg;
    }

    return NULL;
    
}

static void HciWiFiEvtTxDataStreamStart(uint16_t txLen)
{
    HciWiFiMsgTxDataStream_t *msg;
    
    msg = (HciWiFiMsgTxDataStream_t *)osal_msg_allocate(sizeof(HciWiFiMsgTxDataStream_t));
    HAL_ASSERT(msg != NULL);
    msg->head = HCI_WIFI_MSG_HEAD_TxDataStream;
    msg->len = txLen;
    osal_msg_send(HciWiFiTaskID, (uint8_t *)msg);
}

static void HciWiFiEvtTxDataStreamSuccess(void)
{
    osal_set_event(HciWiFiTaskID, HCI_WIFI_EVT_Transmitted);
}

static void HciWiFiEvtSmartConfigUpdateSSID(const char *ssid)
{
    HciWiFiMsgSSID_t *msg;
    uint16_t len;

    len = strlen(ssid)+1;
    
    msg = (HciWiFiMsgSSID_t *)osal_msg_allocate(sizeof(HciWiFiMsgSSID_t));
    HAL_ASSERT(msg != NULL);
    msg->head = HCI_WIFI_MSG_HEAD_SSID;
    msg->ssid = osal_mem_alloc(len);
    HAL_ASSERT(msg->ssid != NULL);
    osal_memcpy(msg->ssid, ssid, len);
    osal_msg_send(HciWiFiTaskID, (uint8_t *)msg);
}

static void HciWiFiEvtSmartConfigUpdatePASSWD(const char *passwd)
{
    HciWiFiMsgPASSWD_t *msg;
    uint16_t len;

    len = strlen(passwd)+1;
    
    msg = (HciWiFiMsgPASSWD_t *)osal_msg_allocate(sizeof(HciWiFiMsgPASSWD_t));
    HAL_ASSERT(msg != NULL);
    msg->head = HCI_WIFI_MSG_HEAD_PASSWD;
    msg->passwd = osal_mem_alloc(len);
    HAL_ASSERT(msg->passwd != NULL);
    osal_memcpy(msg->passwd, passwd, len);
    osal_msg_send(HciWiFiTaskID, (uint8_t *)msg);
}



static void HciWiFiSendState(uint16_t state)
{
    HciWiFiMsgHciWiFiState_t *msg;
    
    msg = (HciWiFiMsgHciWiFiState_t *)osal_msg_allocate(sizeof(HciWiFiMsgHciWiFiState_t));
    HAL_ASSERT(msg != NULL);
    msg->head = HCI_WIFI_MSG_HEAD_HciWiFiState;
    msg->hciWiFiState = state;
    osal_msg_send(HciWiFiTaskID, (uint8_t *)msg);
}




