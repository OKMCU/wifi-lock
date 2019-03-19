#include <stdio.h>
#include <string.h>
#include "hal_atcmdsio.h"
#include "hal_assert.h"
#include "hci_wificmd.h"


extern void HciWiFiCmdSwReset(void)
{
    HalATCmdSIOPrintStr("AT+RST\r\n");
}

extern void HciWiFiCmdSetMode(uint8_t mode)
{
    HalATCmdSIOPrintStr("AT+CWMODE_CUR=");
    HalATCmdSIOPrintUint(mode);
    HalATCmdSIOPrintStr("\r\n");
}

extern void HciWiFiCmdConnect(const char *ssid, const char *passwd)
{
    HalATCmdSIOPrintStr("AT+CWJAP_CUR=\"");
    HalATCmdSIOPrintStr(ssid);
    HalATCmdSIOPrintStr("\",\"");
    if(passwd != NULL)
        HalATCmdSIOPrintStr(passwd);
    HalATCmdSIOPrintStr("\"");
    HalATCmdSIOPrintStr("\r\n");
}

extern void HciWiFiCmdDisconnect(void)
{
    HalATCmdSIOPrintStr("AT+CWQAP\r\n");
}

extern void HciWiFiCmdQryIp(void)
{
    HalATCmdSIOPrintStr("AT+CIFSR\r\n");
}

extern void HciWiFiCmdSocketOpen(uint8_t type, const char *ip_dn, uint16_t port)
{
    const char *connType[] = {
        "TCP",
        "UDP",
        "SSL"
    };
    HalATCmdSIOPrintStr("AT+CIPSTART=\"");
    HalATCmdSIOPrintStr(connType[type]);
    HalATCmdSIOPrintStr("\",\"");
    HalATCmdSIOPrintStr(ip_dn);
    HalATCmdSIOPrintStr("\",");
    HalATCmdSIOPrintUint(port);
    HalATCmdSIOPrintStr("\r\n");
}

extern void HciWiFiCmdSocketClose(void)
{
    HalATCmdSIOPrintStr("AT+CIPCLOSE\r\n");
}

extern void HciWiFiCmdTxDataStreamStart(uint16_t txDataLen)
{
    HalATCmdSIOPrintStr("AT+CIPSEND=");
    HalATCmdSIOPrintUint(txDataLen);
    HalATCmdSIOPrintStr("\r\n");
}

extern void HciWiFiCmdSmartConfigStart(void)
{
    HalATCmdSIOPrintStr("AT+CWSTARTSMART=3\r\n");
}

extern void HciWiFiCmdSmartConfigStop(void)
{
    HalATCmdSIOPrintStr("AT+CWSTOPSMART\r\n");
}


