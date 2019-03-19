#ifndef __WEBSOCKET_H__
#define __WEBSOCKET_H__

#include <stdint.h>


#define WEBSOCKET_ERR_None               0
#define WEBSOCKET_ERR_WiFiNA             1
#define WEBSOCKET_ERR_ReOpen             2
#define WEBSOCKET_ERR_ReClose            3
#define WEBSOCKET_ERR_InvalidArg         4

#define WEBSOCKET_STATE_Closed           0x00
#define WEBSOCKET_STATE_Opening          0x01
#define WEBSOCKET_STATE_Handshaking      0x02
#define WEBSOCKET_STATE_HandshakePass    0x03
#define WEBSOCKET_STATE_Closing          0x04

#define WEBSOCKET_CLOSE_CODE_NC          1000   //Normal closure
#define WEBSOCKET_CLOSE_CODE_GA          1001   //Going away
#define WEBSOCKET_CLOSE_CODE_PE          1002   //Protocol error
#define WEBSOCKET_CLOSE_CODE_NA          1003   //Not accepted data
#define WEBSOCKET_CLOSE_CODE_ID          1007   //Invalid UTF-8 data
#define WEBSOCKET_CLOSE_CODE_VP          1008   //Voilate policy
#define WEBSOCKET_CLOSE_CODE_BM          1009   //Big message, can not be processed
#define WEBSOCKET_CLOSE_CODE_NE          1010   //Need extension
#define WEBSOCKET_CLOSE_CODE_UC          1011   //Unexpected condition (from server)
#define WEBSOCKET_CLOSE_CODE_TLS         1015   //TLS handshake verification error

typedef struct {
    char *path;
    char *host;
    uint16_t port;
    void (*onWebSocketOpened)(void);
    void (*onWebSocketRxData)(char *p);
    void (*onWebSocketClosed)(void);
} WebSocketInitStruct_t;

extern uint8_t  WebSocketState(void);
extern uint8_t  WebSocketOpen(WebSocketInitStruct_t *pInit);
extern void     WebSocketSend(const char *p);
extern void     WebSocketClose(uint16_t code, const char *reason);

#endif

