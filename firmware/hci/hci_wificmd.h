#ifndef __HCI_WIFI_CMD_H__
#define __HCI_WIFI_CMD_H__

#include <stdint.h>

#define HCI_WIFI_MODE_Station       1
#define HCI_WIFI_MODE_SoftAP        2
#define HCI_WIFI_MODE_SoftAPStation 3

#define HCI_WIFI_CONN_TYPE_TCP      0
#define HCI_WIFI_CONN_TYPE_UDP      1
#define HCI_WIFI_CONN_TYPE_SSL      2

extern void HciWiFiCmdSwReset(void);
extern void HciWiFiCmdSetMode(uint8_t mode);
extern void HciWiFiCmdConnect(const char *ssid, const char *passwd);
extern void HciWiFiCmdDisconnect(void);
extern void HciWiFiCmdQryIp(void);
extern void HciWiFiCmdSocketOpen(uint8_t type, const char *ip_dn, uint16_t port);
extern void HciWiFiCmdSocketClose(void);
extern void HciWiFiCmdTxDataStreamStart(uint16_t txDataLen);
extern void HciWiFiCmdSmartConfigStart(void);
extern void HciWiFiCmdSmartConfigStop(void);


#endif /* __HCI_WIFI_CMD_H__ */

