#ifndef __APP_LOCK_H__
#define __APP_LOCK_H__

#include <stdint.h>
#include "hal_key.h"

#define APP_LOCK_MSG_HEAD_KeyMsg        0x00
#define APP_LOCK_MSG_HEAD_Terminal      0x01

#define APP_LOCK_EVT_DoorOpenAlarm      0x0040
#define APP_LOCK_EVT_WsOpenedBeep       0x0020
#define APP_LOCK_EVT_SendHBP            0x0010
#define APP_LOCK_EVT_CloseWebSocket     0x0008
#define APP_LOCK_EVT_OpenLockEnable     0x0004
#define APP_LOCK_EVT_OpenWebSocket      0x0002
#define APP_LOCK_EVT_StartWiFiConn      0x0001

volatile typedef struct {
    uint8_t head;
} AppLockMsg_t;

volatile typedef struct {
    uint8_t head;
    HalKeyMsg_t keyMsg;
} AppLockMsgKey_t;

volatile typedef struct {
    uint8_t head;
    char *pCmd;
} AppLockMsgTerminal_t;

extern uint8_t AppLockTaskID;

extern void AppLock_Init( uint8_t task_id );
extern uint16_t AppLock_ProcessEvent ( uint8_t task_id, uint16_t events );

extern void onWiFiDisconnected(void);
extern void onWiFiConnected(void);
extern void onWiFiPairingWaiting(void);
extern void onWiFiPairingUpdateSSID(const char *ssid);
extern void onWiFiPairingUpdatePASSWD(const char *passwd);
extern void onWiFiPairingSuccess(void);
extern void onWiFiPairingFailure(void);
extern void onWebSocketOpened(void);
extern void onWebSocketRxData(char *pSvrMsg);
extern void onWebSocketClosed(void);

#endif

