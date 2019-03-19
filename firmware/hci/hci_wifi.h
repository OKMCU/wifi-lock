#ifndef __HCI_WIFI_H__
#define __HCI_WIFI_H__

#include <stdint.h>

extern uint8_t HciWiFiTaskID;

#define HCI_WIFI_MSG_HEAD_ATCmd                 0x00        //char *
#define HCI_WIFI_MSG_HEAD_SSID                  0x01        //char *
#define HCI_WIFI_MSG_HEAD_PASSWD                0x02        //char *
#define HCI_WIFI_MSG_HEAD_HciWiFiState          0x03        //HciWiFiMsgBodyHciState_t *
#define HCI_WIFI_MSG_HEAD_RxDataStream          0x04        //HciWiFiMsgBodyRxDataStream_t *
#define HCI_WIFI_MSG_HEAD_TxDataStream          0x05        //HciWiFiMsgBodyTxDataStream_t *

#define HCI_WIFI_EVT_TxPoll                     0x0020
#define HCI_WIFI_EVT_HciEnable                  0x0010
#define HCI_WIFI_EVT_PairingTimeout             0x0008
#define HCI_WIFI_EVT_Transmitted                0x0004
#define HCI_WIFI_EVT_Receiving                  0X0002
#define HCI_WIFI_EVT_CheckHciWiFiState          0x0001

#define HCI_WIFI_STATE_DEV_Off                  0
#define HCI_WIFI_STATE_DEV_Initializing         1
#define HCI_WIFI_STATE_DEV_Initialized          2
#define HCI_WIFI_STATE_CONN_Disconnected        0
#define HCI_WIFI_STATE_CONN_Disconnecting       1
#define HCI_WIFI_STATE_CONN_Connected           2
#define HCI_WIFI_STATE_CONN_Connecting          3
#define HCI_WIFI_STATE_CONN_GotIP               4
#define HCI_WIFI_STATE_CONN_PairingWaiting      5
#define HCI_WIFI_STATE_CONN_PairingStarted      6
#define HCI_WIFI_STATE_CONN_PairingSuccess      7
#define HCI_WIFI_STATE_CONN_PairingFailure      8
#define HCI_WIFI_STATE_CONN_PairingStopped      9
#define HCI_WIFI_STATE_SCKT_Closed              0
#define HCI_WIFI_STATE_SCKT_Opening             1
#define HCI_WIFI_STATE_SCKT_Opened              2
#define HCI_WIFI_STATE_SCKT_Closing             3
#define HCI_WIFI_STATE_COMM_Idle                0           
#define HCI_WIFI_STATE_COMM_Tx                  1
#define HCI_WIFI_STATE_COMM_Rx                  2
#define HCI_WIFI_STATE_COMM_TxRx                3

#define HCI_WIFI_CONN_ERR_None                  0
#define HCI_WIFI_CONN_ERR_InvalidArg            1
#define HCI_WIFI_CONN_ERR_WiFiNA                2           //Wi-Fi connecting or connected or initializing
#define HCI_WIFI_CONN_ERR_Busy                  3

#define HCI_WIFI_SCKT_ERR_None                  0
#define HCI_WIFI_SCKT_ERR_WiFiNA                1
#define HCI_WIFI_SCKT_ERR_ReOpen                2
#define HCI_WIFI_SCKT_ERR_ReClose               3
#define HCI_WIFI_SCKT_ERR_InvalidArg            4
#define HCI_WIFI_SCKT_ERR_Busy                  5

#define HCI_WIFI_CONFIG_HciEnableDelay          1000
#define HCI_WIFI_CONFIG_PairingTimeout          60000                

volatile typedef struct {
    uint8_t head;
} HciWiFiMsg_t;

volatile typedef struct {
    uint8_t head;
    char *pATCmd;
} HciWiFiMsgATCmd_t;

volatile typedef struct {
    uint8_t head;
    char *ssid;
} HciWiFiMsgSSID_t;

volatile typedef struct {
    uint8_t head;
    char *passwd;
} HciWiFiMsgPASSWD_t;

volatile typedef struct {
    uint8_t head;
    uint16_t hciWiFiState;
} HciWiFiMsgHciWiFiState_t;

volatile typedef struct {
    uint8_t head;
    uint16_t len;
    uint8_t *pData;
} HciWiFiMsgRxDataStream_t;

volatile typedef struct {
    uint8_t head;
    uint16_t len;
} HciWiFiMsgTxDataStream_t;

typedef struct {
    char *ssid;
    char *passwd;
    void (*onWiFiDisconnected)(void);
    void (*onWiFiConnected)(void);
} HciWiFiConnInitStrcuct_t;

typedef struct {
    void (*onWiFiPairingWaiting)(void);
    void (*onWiFiPairingUpdateSSID)(const char *ssid);
    void (*onWiFiPairingUpdatePASSWD)(const char *passwd);
    void (*onWiFiPairingSuccess)(void);
    void (*onWiFiPairingFailure)(void);
} HciWiFiPairingInitStruct_t;

typedef struct {
    char *addr;
    uint16_t port;
    void (*onSocketOpened)(void);
    void (*onSocketClosed)(void);
    void (*onSocketRxData)(uint8_t *pData, uint16_t len);
} HciWiFiSocketInitStruct_t;

#define HCI_WIFI_STATE(comm, sckt, conn, dev)       (uint16_t)((comm<<12) | (sckt<<8) | (conn<<4) | dev)
#define HCI_WIFI_STATE_COMM(state)                  (state>>12)
#define HCI_WIFI_STATE_SCKT(state)                  ((state>>8) & 0x000F)
#define HCI_WIFI_STATE_CONN(state)                  ((state>>4) & 0x000F)
#define HCI_WIFI_STATE_DEV(state)                   (state & 0x000F)

extern void HciWiFi_Init( uint8_t task_id );
extern uint16_t HciWiFi_ProcessEvent ( uint8_t task_id, uint16_t events );

extern void HciWiFiReset(void);
extern uint16_t HciWiFiGetState(void);
extern uint8_t HciWiFiSetConnection(HciWiFiConnInitStrcuct_t *p_arg);
extern void HciWiFiPairingStart(HciWiFiPairingInitStruct_t *p_arg);

extern uint8_t HciWiFiSocketOpen(HciWiFiSocketInitStruct_t *p_arg);
extern uint8_t HciWiFiSocketClose(void);
extern void HciWiFiSocketSend(uint8_t *pData, uint16_t len);
extern void HciWiFiSocketSendByte(uint8_t byte);
extern void HciWiFiSocketSendChar(char c);
extern void HciWiFiSocketSendStr(char *str);
extern void HciWiFiSocketSendSint(int32_t num);
extern void HciWiFiSocketSendUint(uint32_t num);
extern void HciWiFiSocketSendHex8(uint8_t num);
extern void HciWiFiSocketSendHex16(uint16_t num);
extern void HciWiFiSocketSendHex32(uint32_t num);


#endif /* __HCI_WIFI_H__ */

