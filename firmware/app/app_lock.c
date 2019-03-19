#include <stdio.h>
#include <string.h>
#include "osal.h"
#include "osal_clock.h"
#include "app_lock.h"
#include "hal_terminal.h"
#include "hal_assert.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_buzzer.h"
#include "hal_devid.h"
#include "hal_flash.h"
#include "stringx.h"
#include "md5.h"
#include "hci_wifi.h"
#include "hal_atcmdsio.h"
#include "websocket.h"

uint8_t AppLockTaskID;
static char *pSSID;
static char *pPASSWD;
static bool isDoorOpen;
static bool isOpenLockEnable;
static bool isPairingEnable;

#define FLASH_ADDR_SSID         (HAL_FLASH_PAGE_SIZE*0)
#define FLASH_ADDR_PASSWD       (HAL_FLASH_PAGE_SIZE*1)
#define FLASH_ADDR_SERVER       (HAL_FLASH_PAGE_SIZE*2)

#define MAX_LENGTH_SSID         32
#define MAX_LENGTH_PASSWD       32
#define MAX_LENGTH_SERVER       64
#define MAX_LENGTH_MSG          125
#define MAX_LENGTH_DEVID        45 

#define TEST_SERVER             "www.fatdragonhk.com:8080/myHandler"//"123.207.167.163:9010/ajaxchattest"

#define DELAY_OPEN_WEBSOCKET    3000
#define DELAY_START_WIFI_CONN   1000
#define DELAY_RELAY_OFF         1000
#define DELAY_OPENLOCK_ENABLE   5000
#define DELAY_CLOSE_WEBSOCKET   60000
#define DELAY_SEND_HBP          30000
#define DELAY_WEBSOCKET_OPENED_BEEP     3000

#define HAL_OPEN_LOCK()         HalLedBlink(HAL_LED_Relay, 1, 50, 1000)

#if 0
static void appLock_HandleServerMsg(char *pSvrMsg)
{

}
#endif

static uint16_t flashReadSSID(char *ssid)
{
    uint16_t len;
    HAL_ASSERT(ssid != NULL);
    HalFlashRead(FLASH_ADDR_SSID, (uint32_t *)ssid, MAX_LENGTH_SSID/sizeof(uint32_t));

    len = 0;
    while(len < MAX_LENGTH_SSID)
    {
        if(ssid[len] == 0x00)
        {
            break;
        }
        else if((uint8_t)ssid[len] == 0xFF)
        {
            len = 0;
            break;
        }
        else
        {
            len++;
        }
    }
    
    return len;
}
static void flashWriteSSID(const char *ssid)
{
    char pBuf[MAX_LENGTH_SSID];
    uint16_t len = 0;
    if(ssid != NULL)
        len = strlen(ssid);
    if(len <= MAX_LENGTH_SSID)
    {
        osal_memset(pBuf, 0x00, sizeof(pBuf));
        if(len > 0)
            osal_memcpy(pBuf, ssid, len);
        HalFlashErase(FLASH_ADDR_SSID);
        HalFlashWrite(FLASH_ADDR_SSID, (uint32_t *)pBuf, sizeof(pBuf)/sizeof(uint32_t));
    }
}

static uint16_t flashReadPASSWD(char *passwd)
{
    uint16_t len;
    HAL_ASSERT(passwd != NULL);
    HalFlashRead(FLASH_ADDR_PASSWD, (uint32_t *)passwd, MAX_LENGTH_PASSWD/sizeof(uint32_t));

    len = 0;
    while(len < MAX_LENGTH_PASSWD)
    {
        if(passwd[len] == 0x00)
        {
            break;
        }
        else if((uint8_t)passwd[len] == 0xFF)
        {
            len = 0;
            break;
        }
        else
        {
            len++;
        }
    }
    
    return len;
}

static void flashWritePASSWD(const char *passwd)
{
    char pBuf[MAX_LENGTH_PASSWD];
    uint16_t len;

    len = 0;
    if(passwd != NULL)
        len = strlen(passwd);
    if(len <= MAX_LENGTH_PASSWD)
    {
        osal_memset(pBuf, 0x00, sizeof(pBuf));
        if(len > 0)
            osal_memcpy(pBuf, passwd, len);
        HalFlashErase(FLASH_ADDR_PASSWD);
        HalFlashWrite(FLASH_ADDR_PASSWD, (uint32_t *)pBuf, sizeof(pBuf)/sizeof(uint32_t));
    }
}

static uint16_t flashReadServer(char *pServer)
{
    uint16_t len;
    
    HAL_ASSERT(pServer != NULL);
    
    HalFlashRead(FLASH_ADDR_SERVER, (uint32_t *)pServer, MAX_LENGTH_SERVER/sizeof(uint32_t));
    
    len = 0;
    while(len < MAX_LENGTH_PASSWD)
    {
        if(pServer[len] == 0x00)
        {
            break;
        }
        else if((uint8_t)pServer[len] == 0xFF)
        {
            len = 0;
            break;
        }
        else
        {
            len++;
        }
    }
    
    return len;
    
}

static void flashWriteServer(const char *pServer)
{
    char pBuf[MAX_LENGTH_SERVER];
    uint16_t len;
    
    len = 0;
    if(pServer != NULL)
        len = strlen(pServer);

    if(len + 1 <= MAX_LENGTH_SERVER)
    {
        osal_memset(pBuf, 0x00, sizeof(pBuf));
        osal_memcpy(pBuf, pServer, len);
        HalFlashErase(FLASH_ADDR_SERVER);
        HalFlashWrite(FLASH_ADDR_SERVER, (uint32_t *)pBuf, sizeof(pBuf)/sizeof(uint32_t));
    }
}

static void appLock_ProcessOsalMsgKey(HalKeyMsg_t keyMsg)
{   
    uint16_t hciWiFiState;
    HciWiFiPairingInitStruct_t pairingInitStruct;
    
    if(keyMsg.keyId  == HAL_KEY_1)
    {
        if(keyMsg.keyEvt == HAL_KEY_EVT_Enter)
        {
            isDoorOpen = FALSE;
            HalTerminalPrintStr("Door closed.\r\n");
            return;
        }
        
        if(keyMsg.keyEvt == HAL_KEY_EVT_Leave)
        {

            isDoorOpen = TRUE;
            HalTerminalPrintStr("Door opened.\r\n");

            hciWiFiState = HciWiFiGetState();
            if(HCI_WIFI_STATE_DEV(hciWiFiState) == HCI_WIFI_STATE_DEV_Initialized)
            {
                if(HCI_WIFI_STATE_CONN(hciWiFiState) != HCI_WIFI_STATE_CONN_GotIP)
                {
                    pairingInitStruct.onWiFiPairingWaiting = onWiFiPairingWaiting;
                    pairingInitStruct.onWiFiPairingFailure = onWiFiPairingFailure;
                    pairingInitStruct.onWiFiPairingSuccess = onWiFiPairingSuccess;
                    pairingInitStruct.onWiFiPairingUpdatePASSWD = onWiFiPairingUpdatePASSWD;
                    pairingInitStruct.onWiFiPairingUpdateSSID = onWiFiPairingUpdateSSID;
                    HciWiFiPairingStart(&pairingInitStruct);
                }
            }
            
            return;
        }

        return;
    }

    if(keyMsg.keyId  == HAL_KEY_0 &&
       keyMsg.keyEvt == HAL_KEY_EVT_Long)
    {
        //WebSocketSend("HBP:Key Pressed.");
        HalFlashErase(FLASH_ADDR_SSID);
        HalFlashErase(FLASH_ADDR_PASSWD);
        pairingInitStruct.onWiFiPairingWaiting = onWiFiPairingWaiting;
        pairingInitStruct.onWiFiPairingSuccess = onWiFiPairingSuccess;
        pairingInitStruct.onWiFiPairingFailure = onWiFiPairingFailure;
        pairingInitStruct.onWiFiPairingUpdatePASSWD = onWiFiPairingUpdatePASSWD;
        pairingInitStruct.onWiFiPairingUpdateSSID = onWiFiPairingUpdateSSID;
        HciWiFiPairingStart(&pairingInitStruct);
        return;
    }

    if(keyMsg.keyId  == HAL_KEY_0 &&
       keyMsg.keyEvt == HAL_KEY_EVT_Short)
    {
        //WebSocketSend("Open lock.");
        if(isOpenLockEnable)
        {
            HAL_OPEN_LOCK();
            isOpenLockEnable = FALSE;
            osal_start_timerEx(AppLockTaskID, APP_LOCK_EVT_OpenLockEnable, DELAY_OPENLOCK_ENABLE);
        }
        return;
    }
    
}

static void appLock_ProcessOsalMsgTerminal(char *pCmd)
{
    WebSocketSend(pCmd); 
    
    //HalATCmdSIOPrintStr(pCmd);
    //HalATCmdSIOPrintStr("\r\n");
}

static void appLock_ProcessOsalMsg(uint8_t *pMsg)
{
    switch (((AppLockMsg_t *)pMsg)->head)
    {
        case APP_LOCK_MSG_HEAD_KeyMsg:
            appLock_ProcessOsalMsgKey(((AppLockMsgKey_t *)pMsg)->keyMsg);
        break;

        case APP_LOCK_MSG_HEAD_Terminal:
            appLock_ProcessOsalMsgTerminal(((AppLockMsgTerminal_t *)pMsg)->pCmd);
            osal_mem_free(((AppLockMsgTerminal_t *)pMsg)->pCmd);
        break;
        
        default:
            HAL_ASSERT_FORCED();
        break;
    }
}

extern void AppLock_Init( uint8_t task_id )
{
    uint8_t i;
    
    AppLockTaskID = task_id;
    pSSID = NULL;
    pPASSWD = NULL;

    isDoorOpen = (HalKeyRead() & (1<<HAL_KEY_1)) ? FALSE : TRUE;
    isOpenLockEnable = TRUE;
    isPairingEnable = FALSE;

    if(isDoorOpen)
    {
        HalTerminalPrintStr("Door opened.\r\n");
    }
    else
    {
        HalTerminalPrintStr("Door closed.\r\n");
    }

    //Print device ID
    HalTerminalPrintStr("[DEVID]:");
    for(i = 0; i < HAL_DEVID_SIZE; i++)
    {
        HalTerminalPrintHex32(HalDevIdRead(i));
        if(i < HAL_DEVID_SIZE-1)
        {
            HalTerminalPrintChar('-');
        }
    }
    HalTerminalPrintStr("\r\n");

    HalLedBlink(HAL_LED_System, 0, 50, 1000);
    osal_start_timerEx(task_id, APP_LOCK_EVT_StartWiFiConn, DELAY_START_WIFI_CONN);
}


extern uint16_t AppLock_ProcessEvent ( uint8_t task_id, uint16_t events )
{
    uint8_t *pMsg;
    char *pServer;
    char *pHost;
    char *pPath;
    char *pPort;
    uint32_t u32tmp;
    HciWiFiConnInitStrcuct_t connInitStruct;
    HciWiFiPairingInitStruct_t pairingInitStruct;
    WebSocketInitStruct_t wsInitStruct;
    uint16_t u16tmp;
    
    (void)task_id;  // Intentionally unreferenced parameter
    
    
    if ( events & SYS_EVENT_MSG )
    {
        pMsg = osal_msg_receive(task_id);

        while (pMsg)
        {
          /* Process msg */
          appLock_ProcessOsalMsg( pMsg );
          
          /* De-allocate */
          osal_msg_deallocate( pMsg );
          /* Next */
          pMsg = osal_msg_receive( task_id );
        }
        return events ^ SYS_EVENT_MSG;
    }

    if( events & APP_LOCK_EVT_SendHBP )
    {
        WebSocketSend("@heart");
        return events ^ APP_LOCK_EVT_SendHBP;
    }

    if( events & APP_LOCK_EVT_OpenLockEnable )
    {
        isOpenLockEnable = TRUE;
        return events ^ APP_LOCK_EVT_OpenLockEnable;
    }

    if( events & APP_LOCK_EVT_OpenWebSocket )
    {
        if(HciWiFiGetState() == HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                                               HCI_WIFI_STATE_SCKT_Closed, 
                                               HCI_WIFI_STATE_CONN_GotIP, 
                                               HCI_WIFI_STATE_DEV_Initialized))
        {
            pServer = (char *)osal_mem_alloc(MAX_LENGTH_SERVER);
            HAL_ASSERT(pServer != NULL);

            while(1)
            {
                pHost = NULL;
                pPath = NULL;
                pPort = NULL;
                u16tmp = flashReadServer(pServer);
                if(u16tmp == 0)
                {
                    flashWriteServer(TEST_SERVER);
                    continue;
                }
                pHost = pServer;
                wsInitStruct.host = pHost;
                strtok_r(pHost, ":", &pPort);
                strtok_r(pPort, "/", &pPath);
                if(decstr2uint(pPort, &u32tmp) == 0)
                {
                    flashWriteServer(TEST_SERVER);
                    continue;
                }
                wsInitStruct.port = (uint16_t)u32tmp;
                pPath--;
                *pPath = '/';
                wsInitStruct.path = pPath;
                wsInitStruct.onWebSocketOpened = onWebSocketOpened;
                wsInitStruct.onWebSocketClosed = onWebSocketClosed;
                wsInitStruct.onWebSocketRxData = onWebSocketRxData;
                WebSocketOpen(&wsInitStruct);
                osal_mem_free(pServer);

                HalLedBlink(HAL_LED_WiFi_B, 0, 50, 500);
                HalLedBlink(HAL_LED_WiFi_R, 0, 50, 500);
                break;
            }
        }
        else
        {
            osal_start_timerEx(task_id, APP_LOCK_EVT_OpenWebSocket, DELAY_OPEN_WEBSOCKET);
        }
        return events ^ APP_LOCK_EVT_OpenWebSocket;
    }

    if( events & APP_LOCK_EVT_StartWiFiConn )
    {
        if(HciWiFiGetState() == HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                                               HCI_WIFI_STATE_SCKT_Closed, 
                                               HCI_WIFI_STATE_CONN_Disconnected, 
                                               HCI_WIFI_STATE_DEV_Initialized) ||
           HciWiFiGetState() == HCI_WIFI_STATE(HCI_WIFI_STATE_COMM_Idle, 
                                               HCI_WIFI_STATE_SCKT_Closed, 
                                               HCI_WIFI_STATE_CONN_PairingStopped, 
                                               HCI_WIFI_STATE_DEV_Initialized)                                   )
        {
            connInitStruct.ssid = (char *)osal_mem_alloc(MAX_LENGTH_SSID);
            HAL_ASSERT(connInitStruct.ssid != NULL);
            u16tmp = flashReadSSID(connInitStruct.ssid);
            if(u16tmp != 0)
            {
                connInitStruct.passwd = (char *)osal_mem_alloc(MAX_LENGTH_PASSWD);
                HAL_ASSERT(connInitStruct.passwd != NULL);
                u16tmp = flashReadPASSWD(connInitStruct.passwd);
                connInitStruct.passwd[u16tmp] = '\0';
                connInitStruct.onWiFiConnected = onWiFiConnected;
                connInitStruct.onWiFiDisconnected = onWiFiDisconnected;
                HAL_ASSERT(HciWiFiSetConnection(&connInitStruct) == HCI_WIFI_CONN_ERR_None);
                osal_mem_free(connInitStruct.passwd);
                osal_mem_free(connInitStruct.ssid);
                HalLedBlink(HAL_LED_WiFi_B, 0, 50, 500);
                HalLedSet(HAL_LED_WiFi_R, HAL_LED_MODE_OFF);
            }
            else
            {
                osal_mem_free(connInitStruct.ssid);
                pairingInitStruct.onWiFiPairingWaiting = onWiFiPairingWaiting;
                pairingInitStruct.onWiFiPairingSuccess = onWiFiPairingSuccess;
                pairingInitStruct.onWiFiPairingFailure = onWiFiPairingFailure;
                pairingInitStruct.onWiFiPairingUpdatePASSWD = onWiFiPairingUpdatePASSWD;
                pairingInitStruct.onWiFiPairingUpdateSSID = onWiFiPairingUpdateSSID;
                HciWiFiPairingStart(&pairingInitStruct);
            }
        }
        else
        {
            osal_start_timerEx(task_id, APP_LOCK_EVT_StartWiFiConn, DELAY_START_WIFI_CONN);
        }
        return events ^ APP_LOCK_EVT_StartWiFiConn;
    }


    if( events & APP_LOCK_EVT_CloseWebSocket )
    {
        WebSocketClose(WEBSOCKET_CLOSE_CODE_NC, "No communication for 60 sec.");
        return events ^ APP_LOCK_EVT_CloseWebSocket;
    }

    if( events & APP_LOCK_EVT_WsOpenedBeep )
    {
        //Websocket connection stable
        HalBuzzerBeep(HAL_BUZZER_TIME_SHORT, 0, 1);
        return events ^ APP_LOCK_EVT_WsOpenedBeep;
    }
    #if 0
    if( events & APP_LOCK_EVT_DevStart )
    {
        HalLedBlink(HAL_LED_System, 0, 50, 1000);

        HalTerminalPrintStr("[EVENT]:");
        HalTerminalPrintStr("Device Started. ");
        HalTerminalPrintStr("Device ID: ");
        for(u8tmp = 0; u8tmp < HAL_DEVID_SIZE; u8tmp++)
        {
            HalTerminalPrintHex32(HalDevIdRead(u8tmp));
            if(u8tmp < HAL_DEVID_SIZE-1)
            {
                HalTerminalPrintChar('-');
            }
        }
        HalTerminalPrintStr(".\r\n");
        
        return events ^ APP_LOCK_EVT_DevStart;
    }
    #endif
    
    return 0;
}

extern void onWiFiDisconnected(void)
{
    osal_start_timerEx(AppLockTaskID, APP_LOCK_EVT_StartWiFiConn, 1000);
    HalLedSet(HAL_LED_WiFi_R, HAL_LED_MODE_OFF);
    HalLedBlink(HAL_LED_WiFi_B, 0, 50, 1000);
    //HalBuzzerBeep(HAL_BUZZER_TIME_LONG, 0, 1);
}

extern void onWiFiConnected(void)
{
    osal_start_timerEx(AppLockTaskID, APP_LOCK_EVT_OpenWebSocket, DELAY_OPEN_WEBSOCKET);
    HalLedSet(HAL_LED_WiFi_B, HAL_LED_MODE_ON);
    HalLedSet(HAL_LED_WiFi_R, HAL_LED_MODE_OFF);
    HalBuzzerBeep(HAL_BUZZER_TIME_SHORT, HAL_BUZZER_TIME_SHORT, 3);
}

extern void onWiFiPairingWaiting(void)
{
    HalLedBlink(HAL_LED_WiFi_R, 0, 50, 500);
    HalLedSet(HAL_LED_WiFi_B, HAL_LED_MODE_OFF);
    HalBuzzerBeep(HAL_BUZZER_TIME_VLONG, 0, 1);
}

extern void onWiFiPairingUpdateSSID(const char *ssid)
{
    uint16_t len;

    len = 0;
    if(ssid != NULL)
    {
        len = strlen(ssid);
    }
    if(len > 0)
    {
        pSSID = osal_mem_alloc(len+1);//+1 for '\0'
        HAL_ASSERT(pSSID != NULL);
        osal_memcpy(pSSID, ssid, len+1);
    }
    
}

extern void onWiFiPairingUpdatePASSWD(const char *passwd)
{
    uint16_t len;

    len = 0;
    if(passwd != NULL)
    {
        len = strlen(passwd);
    }
    if(len > 0)
    {
        pPASSWD = osal_mem_alloc(len+1);//+1 for '\0'
        HAL_ASSERT(pPASSWD != NULL);
        osal_memcpy(pPASSWD, passwd, len+1);
    }
}

extern void onWiFiPairingSuccess(void)
{
    HalTerminalPrintStr("Pairing success.\r\n");
    
    HalTerminalPrintStr("SSID = \"");
    if(pSSID != NULL)
        HalTerminalPrintStr(pSSID);
    HalTerminalPrintStr("\"\r\n");
    flashWriteSSID(pSSID);
    osal_mem_free(pSSID);
    pSSID = NULL;
    
    
    HalTerminalPrintStr("PASSWD = \"");
    if(pPASSWD != NULL)
        HalTerminalPrintStr(pPASSWD);
    HalTerminalPrintStr("\"\r\n");
    flashWritePASSWD(pPASSWD);
    osal_mem_free(pPASSWD);
    pPASSWD = NULL;

    HalTerminalPrintStr("Saved to flash.\r\n");
    osal_start_timerEx(AppLockTaskID, APP_LOCK_EVT_StartWiFiConn, 1000);

    HalLedSet(HAL_LED_WiFi_R, HAL_LED_MODE_OFF);
    HalLedBlink(HAL_LED_WiFi_B, 0, 50, 1000);
    
}

extern void onWiFiPairingFailure(void)
{
    //HciWiFiPairingInitStruct_t pairingInitStruct;
    HalTerminalPrintStr("Pairing failure.\r\n");

    if(pSSID != NULL)
    {
        osal_mem_free(pSSID);
        pSSID = NULL;
    }

    if(pPASSWD != NULL)
    {
        osal_mem_free(pPASSWD);
        pPASSWD = NULL;
    }

    osal_start_timerEx(AppLockTaskID, APP_LOCK_EVT_StartWiFiConn, DELAY_START_WIFI_CONN);
    HalLedSet(HAL_LED_WiFi_R, HAL_LED_MODE_OFF);
    HalLedBlink(HAL_LED_WiFi_B, 0, 50, 1000);
}

extern void onWebSocketOpened(void)
{
    HalTerminalPrintStr("Handshake passed.\r\n");
    HalLedSet(HAL_LED_WiFi_B, HAL_LED_MODE_ON);
    HalLedSet(HAL_LED_WiFi_R, HAL_LED_MODE_ON);
    osal_start_timerEx(AppLockTaskID, APP_LOCK_EVT_CloseWebSocket, DELAY_CLOSE_WEBSOCKET);
    osal_start_timerEx(AppLockTaskID, APP_LOCK_EVT_SendHBP, DELAY_SEND_HBP);

    osal_start_timerEx(AppLockTaskID, APP_LOCK_EVT_WsOpenedBeep, DELAY_WEBSOCKET_OPENED_BEEP);
}

extern void onWebSocketRxData(char *pSvrMsg)
{
    uint8_t *pMd5;
    char *pDevId;
    char *pClientMsg;
    char *pStr;
    
    uint32_t u32tmp;
    int32_t len;
    uint8_t i, j;

    HalTerminalPrintStr("[RXMSG]:");
    HalTerminalPrintStr(pSvrMsg);
    HalTerminalPrintStr("\r\n");

    //reset the timeout timer
    osal_stop_timerEx(AppLockTaskID, APP_LOCK_EVT_CloseWebSocket);
    osal_start_timerEx(AppLockTaskID, APP_LOCK_EVT_CloseWebSocket, DELAY_CLOSE_WEBSOCKET);
    osal_stop_timerEx(AppLockTaskID, APP_LOCK_EVT_SendHBP);
    osal_start_timerEx(AppLockTaskID, APP_LOCK_EVT_SendHBP, DELAY_SEND_HBP);

    /*
    if(strcmp(pSvrMsg, "uuid") == 0)
    {
        WebSocketSend("uuid0");
        return;
    }
    */
    
    if(strcmp(pSvrMsg, "UUID?") == 0)
    {   
        pDevId = osal_mem_alloc( MAX_LENGTH_DEVID );
        HAL_ASSERT(pDevId != NULL);

        //Read device id and store in byte, totally 20 bytes
        len = 0;
        for(i = 0; i < HAL_DEVID_SIZE; i++)
        {
            len += sprintf( pDevId + len, "%.8X", HalDevIdRead(i) );
            if(i < HAL_DEVID_SIZE-1)
            {
                len += sprintf( pDevId + len, "-" );
            }
        }
        
        pMd5 = osal_mem_alloc(MD5_SIZE);
        HAL_ASSERT(pMd5 != NULL);
        md5( pMd5, (uint8_t *)pDevId, MAX_LENGTH_DEVID-1 );
        osal_mem_free(pDevId);
        
        //HalWiFiSendStr("UUID:");
        //HalWiFiSendStr("00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00");//length = 2*16 + 15
        pClientMsg = (char *)osal_mem_alloc(MAX_LENGTH_MSG);
        HAL_ASSERT(pClientMsg != NULL);
        len = 0;
        len += sprintf( pClientMsg + len, "UUID:" );
        for( i = 0; i < MD5_SIZE; i++ )
        {
            len += sprintf( pClientMsg + len, "%.2X", pMd5[i] );
            if( i < MD5_SIZE - 1 )
            {
                pClientMsg[len++] = '-';
            }
        }
        pClientMsg[len] = '\0';
        WebSocketSend(pClientMsg);
        
        HalTerminalPrintStr("[TXMSG]:");
        HalTerminalPrintStr(pClientMsg);
        HalTerminalPrintStr("\r\n");
        
        osal_mem_free(pClientMsg);
        osal_mem_free(pMd5);
        
        return;
    }

    if(strcmp(pSvrMsg, "DEVID?") == 0)
    {
        pClientMsg = (char *)osal_mem_alloc(MAX_LENGTH_MSG);
        HAL_ASSERT(pClientMsg != NULL);
        len = 0;
        len += sprintf( pClientMsg + len, "DEVID:" );
        for(i = 0; i < HAL_DEVID_SIZE; i++)
        {
            len += sprintf( pClientMsg + len, "%.8X", HalDevIdRead(i) );
            if(i < HAL_DEVID_SIZE-1)
            {
                len += sprintf( pClientMsg + len, "-" );
            }
        }
        pClientMsg[len] = '\0';
        WebSocketSend(pClientMsg);

        HalTerminalPrintStr("[TXMSG]:");
        HalTerminalPrintStr(pClientMsg);
        HalTerminalPrintStr("\r\n");
        
        osal_mem_free(pClientMsg);
        return;
    }

    if(strcmp(pSvrMsg, "STATE?") == 0)
    {
        if(isDoorOpen)
            pClientMsg = "STATE:OPENED";
        else
            pClientMsg = "STATE:CLOSED";

        WebSocketSend(pClientMsg);
        
        HalTerminalPrintStr("[TXMSG]:");
        HalTerminalPrintStr(pClientMsg);
        HalTerminalPrintStr("\r\n");
        
        return;
    }
    
    if(strcmp(pSvrMsg, "OPEN!") == 0)
    {
        if(isOpenLockEnable)
        {
            HAL_OPEN_LOCK();
            isOpenLockEnable = FALSE;
            osal_start_timerEx(AppLockTaskID, APP_LOCK_EVT_OpenLockEnable, DELAY_OPENLOCK_ENABLE);
        }
        pClientMsg = "OPEN:OK";
        WebSocketSend(pClientMsg);
        
        HalTerminalPrintStr("[TXMSG]:");
        HalTerminalPrintStr(pClientMsg);
        HalTerminalPrintStr("\r\n");
        
        return;
    }

    if(strcmp(pSvrMsg, "PAIRING!") == 0)
    {
        //WebSocketSend("PAIRING:OK");
        isPairingEnable = TRUE;
        WebSocketClose(WEBSOCKET_CLOSE_CODE_GA, "Client enable Wi-Fi pairing.");
        return;
    }

    pStr = strStartsWith(pSvrMsg, "VERIFY!");
    if(pStr != NULL)
    {
        pClientMsg = (char *)osal_mem_alloc(MAX_LENGTH_MSG);
        HAL_ASSERT(pClientMsg != NULL);
        strcpy(pClientMsg, "VERIFY:00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00");
        
        u32tmp = (uint32_t)strlen(pStr);
        if(u32tmp == 2*MD5_SIZE + MD5_SIZE - 1)
        {
            j = (uint8_t)u32tmp;
            for(i = 0; i < j; i++)
            {
                if(i % 3 == 2)
                {
                    if(pStr[i] != '-')
                    {
                        break;
                    }
                }
                else
                {
                    if(pStr[i] >= '0' && pStr[i] <= '9')
                    {

                    }
                    else if(pStr[i] >= 'A' && pStr[i] <= 'F')
                    {

                    }
                    else
                    {
                        break;
                    }
                }
            } 

            if( i == j )
            {
                //received a legal VCODE, now calculate the verification answer

                //load the Device ID as string format
                len = 0;
                for(i = 0; i < HAL_DEVID_SIZE; i++)
                {
                    len += sprintf( pClientMsg + len, "%.8X", HalDevIdRead(i) );
                    if(i < HAL_DEVID_SIZE-1)
                    {
                        len += sprintf( pClientMsg + len, "-" );
                    }
                }
                pClientMsg[len] = '\0';

                //calculate the MD5 of this Device ID string
                pMd5 = osal_mem_alloc(MD5_SIZE);
                HAL_ASSERT(pMd5 != NULL);
                md5(pMd5, (uint8_t *)pClientMsg, (uint32_t)len);

                //convert the MD5 to string
                len = 0;
                for( i = 0; i < MD5_SIZE; i++ )
                {
                    len += sprintf( pClientMsg + len, "%.2X", pMd5[i] );
                    if( i < MD5_SIZE - 1 )
                    {
                        pClientMsg[len++] = '-';
                    }
                }
                pClientMsg[len] = '\0';

                //len should be 47
                HAL_ASSERT(len == 2*MD5_SIZE + MD5_SIZE - 1);

                //merge the Device ID's MD5 and VCODE (simple add operation)
                for( i = 0; i < 2*MD5_SIZE + MD5_SIZE - 1; i++ )
                {
                    pClientMsg[i] += pStr[i];
                }

                //calculate the merged data chunk's MD5
                md5(pMd5, (uint8_t *)pClientMsg, (uint32_t)len);

                //now, build the actual client message
                len = 0;
                len += sprintf( pClientMsg + len, "VERIFY:" );
                for( i = 0; i < MD5_SIZE; i++ )
                {
                    len += sprintf( pClientMsg + len, "%.2X", pMd5[i] );
                    if( i < MD5_SIZE - 1 )
                    {
                        pClientMsg[len++] = '-';
                    }
                }
                pClientMsg[len] = '\0';
                osal_mem_free(pMd5);
                
            }
        }

        WebSocketSend(pClientMsg);
        
        
        HalTerminalPrintStr("[TXMSG]:");
        HalTerminalPrintStr(pClientMsg);
        osal_mem_free(pClientMsg);
        HalTerminalPrintStr("\r\n");
        
        return;
    }

    pStr = strStartsWith(pSvrMsg, "WSUPDATE!ws://");
    if(pStr != NULL)
    {
        pClientMsg = "WSUPDATE:OK";
        WebSocketSend(pClientMsg);
        HalTerminalPrintStr("[TXMSG]:");
        HalTerminalPrintStr(pClientMsg);
        HalTerminalPrintStr("\r\n");
        
        flashWriteServer(pStr);
        return;
    }

    if(strcmp(pSvrMsg, "WSCLOSE!") == 0)
    {
        WebSocketClose(WEBSOCKET_CLOSE_CODE_NC, "Server requests to close.");
        return;
    }
    
}

extern void onWebSocketClosed(void)
{
    HciWiFiPairingInitStruct_t pairingInitStruct;

    osal_stop_timerEx(AppLockTaskID, APP_LOCK_EVT_SendHBP);
    osal_stop_timerEx(AppLockTaskID, APP_LOCK_EVT_WsOpenedBeep);
    //HalBuzzerBeep(HAL_BUZZER_TIME_SHORT, HAL_BUZZER_TIME_SHORT, 2);
    
    if(isPairingEnable == FALSE)
    {
        osal_stop_timerEx(AppLockTaskID, APP_LOCK_EVT_CloseWebSocket);
        osal_start_timerEx(AppLockTaskID, APP_LOCK_EVT_OpenWebSocket, DELAY_OPEN_WEBSOCKET);
        HalLedSet(HAL_LED_WiFi_R, HAL_LED_MODE_OFF);
        HalLedSet(HAL_LED_WiFi_B, HAL_LED_MODE_ON);
    }
    else
    {
        isPairingEnable = FALSE;
        pairingInitStruct.onWiFiPairingWaiting = onWiFiPairingWaiting;
        pairingInitStruct.onWiFiPairingSuccess = onWiFiPairingSuccess;
        pairingInitStruct.onWiFiPairingFailure = onWiFiPairingFailure;
        pairingInitStruct.onWiFiPairingUpdatePASSWD = onWiFiPairingUpdatePASSWD;
        pairingInitStruct.onWiFiPairingUpdateSSID = onWiFiPairingUpdateSSID;
        HciWiFiPairingStart(&pairingInitStruct);
    }
}



