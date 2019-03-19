#include <stdio.h>
#include <string.h>
#include "osal.h"
#include "osal_clock.h"

#include "hal_terminal.h"
#include "hal_assert.h"
#include "stringx.h"
#include "hci_wifi.h"
#include "websocket.h"

#define WS_HDR_BYTE0_FIN                  0x80
#define WS_HDR_BYTE0_OPCODE_TEXT          0x01
#define WS_HDR_BYTE0_OPCODE_CLOSE         0x08
#define WS_HDR_BYTE1_MASK                 0x80
#define WS_HDR_BYTE1_PAYLOAD_LEN          0x7F

static uint8_t wsState = WEBSOCKET_STATE_Closed;
static WebSocketInitStruct_t *pwsInit = NULL;

static void onSocketOpened(void)
{
    //Send handshake request
    HciWiFiSocketSendStr("GET ");
    HciWiFiSocketSendStr(pwsInit->path);
    HciWiFiSocketSendStr(" HTTP/1.1\r\n");

    HciWiFiSocketSendStr("Host:");
    HciWiFiSocketSendStr(pwsInit->host);
    HciWiFiSocketSendChar(':');
    HciWiFiSocketSendUint(pwsInit->port);
    HciWiFiSocketSendStr("\r\n");

    HciWiFiSocketSendStr("Connection: ");
    HciWiFiSocketSendStr("Upgrade\r\n");
    
    HciWiFiSocketSendStr("Upgrade: websocket\r\n");
    
    HciWiFiSocketSendStr("Sec-WebSocket-Key: ");
    HciWiFiSocketSendStr("dGhlIHNhbXBsZSBub25jZQ==");
    HciWiFiSocketSendStr("\r\n");

    HciWiFiSocketSendStr("Sec-WebSocket-Version: 13\r\n");

    HciWiFiSocketSendStr("\r\n");

    //Free host and path's RAM resource, will not be used in future
    if(pwsInit->host != NULL)
        osal_mem_free(pwsInit->host);
    if(pwsInit->path != NULL)
        osal_mem_free(pwsInit->path);
    pwsInit->host = NULL;
    pwsInit->path = NULL;

    wsState = WEBSOCKET_STATE_Handshaking;
    
}

static void onSocketClosed(void)
{
    void (*pfn)(void);
    
    if(pwsInit->host != NULL)
        osal_mem_free(pwsInit->host);
    if(pwsInit->path != NULL)
        osal_mem_free(pwsInit->path);

    pfn = pwsInit->onWebSocketClosed;
    osal_mem_free(pwsInit);
    pwsInit = NULL;
    wsState = WEBSOCKET_STATE_Closed;
    
    pfn();
}

static void onSocketRxData(uint8_t *pData, uint16_t len)
{
    char *pResponse = NULL;
    char *pRemain = NULL;
    char *pKey = NULL;
    uint8_t *pDataEnd;
    uint8_t u8tmp;
    //uint16_t i;
    uint16_t statusCode;
    uint16_t lenProc = 0;
    bool forceClose;
#if 0
    uint16_t i;
    HalTerminalPrintStr("RX ");
    HalTerminalPrintUint(len);
    HalTerminalPrintStr(" bytes:");
    if(pData != NULL)
    {
        for(i = 0; i < len; i++)
        {
            HalTerminalPrintChar(pData[i]);
        }
        HalTerminalPrintStr("\r\n");
    }
    else
    {
        HalTerminalPrintStr("NULL\r\n");
    }
#endif

    //return if data body is missing
    //this can happen when data length is too long
    if(pData == NULL)
        return;

    if(wsState == WEBSOCKET_STATE_Handshaking)
    {
        pResponse = strtok_r((char *)pData, "\r\n", &pRemain);
        if(strcmp(pResponse, "HTTP/1.1 101 ") != 0)
        {
            //HAL_ASSERT(HciWiFiSocketClose() == HCI_WIFI_SCKT_ERR_None);
            wsState = WEBSOCKET_STATE_Closing;
            return;
        }

        pResponse = strtok_r(NULL, "\r\n", &pRemain);
        if(strcmp(pResponse, "Upgrade: websocket") != 0)
        {
            //HAL_ASSERT(HciWiFiSocketClose() == HCI_WIFI_SCKT_ERR_None);
            wsState = WEBSOCKET_STATE_Closing;
            return;
        }

        pResponse = strtok_r(NULL, "\r\n", &pRemain);
        if(strcmp(pResponse, "Connection: upgrade") != 0)
        {
            //HAL_ASSERT(HciWiFiSocketClose() == HCI_WIFI_SCKT_ERR_None);
            wsState = WEBSOCKET_STATE_Closing;
            return;
        }

        pResponse = strtok_r(NULL, "\r\n", &pRemain);
        pKey = strStartsWith(pResponse, "Sec-WebSocket-Accept: ");
        if(pKey == NULL)
        {
            //HAL_ASSERT(HciWiFiSocketClose() == HCI_WIFI_SCKT_ERR_None);
            wsState = WEBSOCKET_STATE_Closing;
            return;
        }

        if(strcmp(pKey, "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=") != 0)
        {
            //HAL_ASSERT(HciWiFiSocketClose() == HCI_WIFI_SCKT_ERR_None);
            wsState = WEBSOCKET_STATE_Closing;
            return;
        }

        pResponse = strtok_r(NULL, "\r\n", &pRemain);
        if(strStartsWith(pResponse, "Date:") == NULL)
        {
            //HAL_ASSERT(HciWiFiSocketClose() == HCI_WIFI_SCKT_ERR_None);
            wsState = WEBSOCKET_STATE_Closing;
            return;
        }
        
        pResponse = strtok_r(NULL, "\r\n", &pRemain);
        if(pResponse != NULL)
        {
            //HAL_ASSERT(HciWiFiSocketClose() == HCI_WIFI_SCKT_ERR_None);
            wsState = WEBSOCKET_STATE_Closing;
            return;
        }
        
        wsState = WEBSOCKET_STATE_HandshakePass;
        pwsInit->onWebSocketOpened();
        return;
    }
    
    if(wsState == WEBSOCKET_STATE_HandshakePass)
    {
        //Websocket data, length must be >= 2, ignore otherwise
        while(lenProc < len)
        {
            if(len - lenProc >= 2)
            {
                //decode websocket data and call onWebSocketRxData()
                //FIN must be set, receive tail frame only, otherwise ignore
                //If this is a tail text frame
                if(pData[lenProc + 0] == (WS_HDR_BYTE0_FIN | WS_HDR_BYTE0_OPCODE_TEXT))
                {
                    //Text from server should not be masked. Ignore otherwise
                    if((pData[lenProc + 1] & WS_HDR_BYTE1_MASK) == 0)
                    {
                        //only accept data packets with length <= 125, ignore otherwise
                        if((pData[lenProc + 1] & WS_HDR_BYTE1_PAYLOAD_LEN) <= 125)
                        {
                            pDataEnd = pData + 2 + (pData[lenProc + 1] & WS_HDR_BYTE1_PAYLOAD_LEN);
                            u8tmp = *pDataEnd;
                            *pDataEnd = 0;
                            pwsInit->onWebSocketRxData((char *)&pData[lenProc + 2]);
                            *pDataEnd = u8tmp;
                            lenProc += 2 + (pData[lenProc + 1] & WS_HDR_BYTE1_PAYLOAD_LEN);
                        }
                        else
                        {
                            WebSocketClose(WEBSOCKET_CLOSE_CODE_BM, "Data length should be less than 125 bytes.");
                            break;
                        }
                    }
                    else
                    {
                        WebSocketClose(WEBSOCKET_CLOSE_CODE_PE, "Data from server should not be masked.");
                        break;
                    }
                }
                else if(pData[0 + lenProc] == (WS_HDR_BYTE0_FIN | WS_HDR_BYTE0_OPCODE_CLOSE))
                {
                    HAL_ASSERT(HciWiFiSocketClose() == HCI_WIFI_SCKT_ERR_None);
                    wsState = WEBSOCKET_STATE_Closing;
                    break;
                }
                else
                {
                    WebSocketClose(WEBSOCKET_CLOSE_CODE_NA, "Client accept text data ONLY.");
                    break;
                }
            }
            else
            {
                WebSocketClose(WEBSOCKET_CLOSE_CODE_PE, "Illegal websocket data packet.");
                break;
            }
        }
        /*
        {
            HalTerminalPrintStr("Unknown:");
            for(i = 0; i < len; i++)
            {
                HalTerminalPrintHex8(pData[i]);
                HalTerminalPrintChar(' ');
            }
            HalTerminalPrintStr("\r\n");
        }
        */
        return;
    }
    
    if(wsState == WEBSOCKET_STATE_Closing)
    {
        forceClose = TRUE;
        if(pData[0] == (WS_HDR_BYTE0_FIN | WS_HDR_BYTE0_OPCODE_CLOSE))
        {
            if((pData[1] & WS_HDR_BYTE1_MASK) == 0)
            {
                //pay load should be 2, storing status code
                if(pData[1] <= 125 && pData[1] >= 2)
                {
                    forceClose = FALSE;
                    statusCode = (pData[2]<<8) | pData[3];
                    
                    HalTerminalPrintStr("RX server response for closing. Code: ");
                    HalTerminalPrintUint(statusCode);
                    HalTerminalPrintStr(". Reason:");
                    if(pData[1] > 2)
                        HalTerminalPrintStr((char *)&pData[4]);
                    else
                        HalTerminalPrintStr("NULL");
                    HalTerminalPrintStr("\r\n");
                }
            }
        }
        
        if(forceClose == TRUE)
        {
            HalTerminalPrintStr("Do force close!\r\n");
            HAL_ASSERT(HciWiFiSocketClose() == HCI_WIFI_SCKT_ERR_None);
            wsState = WEBSOCKET_STATE_Closing;
        }
        return;
    }
    
}

extern uint8_t  WebSocketState(void)
{
    return wsState;
}

extern uint8_t  WebSocketOpen(WebSocketInitStruct_t *pInit)
{
    uint16_t len;
    uint16_t hciWiFiState;
    HciWiFiSocketInitStruct_t pSocketInit;
    
    if(pInit == NULL)
        return WEBSOCKET_ERR_InvalidArg;
    if(pInit->path == NULL ||
       pInit->host == NULL ||
       pInit->port == 0    ||
       pInit->onWebSocketOpened == NULL ||
       pInit->onWebSocketRxData == NULL ||
       pInit->onWebSocketClosed == NULL)
        return WEBSOCKET_ERR_InvalidArg;
    
    hciWiFiState = HciWiFiGetState();
    if(HCI_WIFI_STATE_DEV(hciWiFiState)  != HCI_WIFI_STATE_DEV_Initialized ||
       HCI_WIFI_STATE_CONN(hciWiFiState) != HCI_WIFI_STATE_CONN_GotIP)
        return WEBSOCKET_ERR_WiFiNA;

    if(HCI_WIFI_STATE_SCKT(hciWiFiState) != HCI_WIFI_STATE_SCKT_Closed)
        return WEBSOCKET_ERR_ReOpen;

    HAL_ASSERT(pwsInit == NULL);
    
    pwsInit = (WebSocketInitStruct_t *)osal_mem_alloc(sizeof(WebSocketInitStruct_t));
    HAL_ASSERT(pwsInit != NULL);

    len = strlen(pInit->path)+1; //+1 for '\0'
    pwsInit->path = (char *)osal_mem_alloc(len);
    HAL_ASSERT(pwsInit->path != NULL);
    osal_memcpy(pwsInit->path, pInit->path, len);

    len = strlen(pInit->host)+1;//+1 for '\0'
    pwsInit->host = (char *)osal_mem_alloc(len);
    HAL_ASSERT(pwsInit->host != NULL);
    osal_memcpy(pwsInit->host, pInit->host, len);
    
    pwsInit->port = pInit->port;
    pwsInit->onWebSocketOpened = pInit->onWebSocketOpened;
    pwsInit->onWebSocketClosed = pInit->onWebSocketClosed;
    pwsInit->onWebSocketRxData = pInit->onWebSocketRxData;
    
    pSocketInit.addr = pwsInit->host;
    pSocketInit.port = pwsInit->port;
    pSocketInit.onSocketClosed = onSocketClosed;
    pSocketInit.onSocketOpened = onSocketOpened;
    pSocketInit.onSocketRxData = onSocketRxData;
    
    HAL_ASSERT(HciWiFiSocketOpen(&pSocketInit) == HCI_WIFI_SCKT_ERR_None);

    wsState = WEBSOCKET_STATE_Opening;
    
    return WEBSOCKET_ERR_None;
}

extern void     WebSocketSend(const char *p)
{
    uint16_t len;
    uint16_t i;
    uint8_t byte0;
    uint8_t byte1;
    static const uint8_t mask[4] = {0x00, 0x00, 0x00, 0x00};

    //do not send before handshake success
    if(wsState != WEBSOCKET_STATE_HandshakePass)
        return;
    
    //do not send string with length longer than 125
    len = strlen(p);
    if(len > 125)
        return;
    byte0 = WS_HDR_BYTE0_FIN | WS_HDR_BYTE0_OPCODE_TEXT;
    byte1 = WS_HDR_BYTE1_MASK | (uint8_t)len;
    HciWiFiSocketSendByte(byte0);
    HciWiFiSocketSendByte(byte1);
    HciWiFiSocketSendByte(mask[0]);
    HciWiFiSocketSendByte(mask[1]);
    HciWiFiSocketSendByte(mask[2]);
    HciWiFiSocketSendByte(mask[3]);
    for(i = 0; i < len; i++)
    {
        HciWiFiSocketSendByte((uint8_t)p[i] ^ mask[i%4]);
    }
}

extern void WebSocketClose(uint16_t code, const char *reason)
{
    uint16_t len;
    uint16_t i;
    uint8_t byte0;
    uint8_t byte1;
    static const uint8_t mask[4] = {0x00, 0x00, 0x00, 0x00};

    //do not send before handshake success
    if(wsState != WEBSOCKET_STATE_HandshakePass)
        return;

    len = 0;
    //do not send string with length longer than 125
    if(reason != NULL)
    {
        len = strlen(reason);
        if(len > 125-2)
            len = 0;
    }
        
    
    byte0 = WS_HDR_BYTE0_FIN | WS_HDR_BYTE0_OPCODE_CLOSE;
    byte1 = WS_HDR_BYTE1_MASK | (uint8_t)len+2;
    HciWiFiSocketSendByte(byte0);
    HciWiFiSocketSendByte(byte1);
    HciWiFiSocketSendByte(mask[0]);
    HciWiFiSocketSendByte(mask[1]);
    HciWiFiSocketSendByte(mask[2]);
    HciWiFiSocketSendByte(mask[3]);
    HciWiFiSocketSendByte((uint8_t)((code>>8)&0xFF) ^ mask[0%4]);
    HciWiFiSocketSendByte((uint8_t)((code>>0)&0xFF) ^ mask[1%4]);
    
    for(i = 0; i < len; i++)
    {
        HciWiFiSocketSendByte((uint8_t)reason[i] ^ mask[(i+2)%4]);
    }
    wsState = WEBSOCKET_STATE_Closing;
    return;
}

