#ifndef OBJECTUART_H
#define OBJECTUART_H

#include <QObject>
#include <QFile>
#include <QDebug>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>

#include <QThread>
#include <QProcess>
#include <QTime>
#include <QTimer>
#include <syslog.h>
#include "objecttcpdumpkiller.h"
#include "boxv3.h"


#define UART_TRANSCEIVER_SELECT_CNT 1
#define FILE_PATH_UARTSTM8  "/dev/ttyS2"
#define FILE_PATH_UART232 "/dev/ttyS3"
#define FILE_PATH_UART485 "/dev/ttyS4"
#define FILE_PATH_LENGTH 32

#define UART_SEND_CHECKEDWELL 0x01
#define UART_RECV_CHECKEDWELL 0x10
#define UART_RECVSEND_CHECKEDWELL 0x11

/*BOXV3 LAN PORTS:
    | Port2 | Port4 |
    | Port1 | Port3 |
*/
#define LINKMASK_PHY          (0x1<<0)
#define LINKMASK_WAN_PORT0     (0x1<<1)
#define LINKMASK_LAN_PORT1    (0x1<<2)
#define LINKMASK_LAN_PORT2    (0x1<<5)
#define LINKMASK_LAN_PORT3    (0x1<<6)
#define LINKMASK_LAN_PORT4    (0x1<<7)
#define LINKMASK_LAN_PORTALL_CHECKED    (LINKMASK_LAN_PORT1|LINKMASK_LAN_PORT2|LINKMASK_LAN_PORT3|LINKMASK_LAN_PORT4)
#define LINKMASK_LAN_WORKWELL LINKMASK_LAN_PORTALL_CHECKED
#define LINKMASK_WAN_WORKWELL LINKMASK_WAN_PORT0
#define STM8_PROTOCOL_HEAD0  0x55
#define STM8_PROTOCOL_HEAD1  0xAA
#define STM8_REGDATA_LENGTH_MAX 32
#define STM8_CMD_AND_DATA_LENGTH_MAX (32+9)

#define ETHNET_TEST_ETH0_HOSTADDR "192.168.1.100"
#define ETHNET_TEST_ETH1_HOSTADDR "192.168.2.100"


typedef struct _netCheckFlag
{
    char checkedPort;
    char sendWorkWellPort;
    char recvWorkWellPort;
    _netCheckFlag()
    {
        checkedPort = 0;
        sendWorkWellPort = 0;
        recvWorkWellPort = 0;
    }
}netCheckFlag_t;

class objectUart : public QObject
{
    Q_OBJECT
public:
    objectUart(char* p);
    ~objectUart();

    void blockedMsecDelay(unsigned int msec);
    void showBuf(char* buf, int len);
    void setUartSpeed(int fd, int speed);
    int setUartParity(int fd, int databits, int stopbits, int parity);

    void calcStm8CmdParity(QByteArray& cmdArray);
    char beginWanAndLanTest(char port);

    char sendNetworkCheckInfo(netCheckFlag_t *netCheckFlag);
signals:
    void signalSendUartCheckResult(char admin, char recvSend);//recvSend: 0x10: recv ok, 0x01: send ok
    void signalTcpdumpTimerKiller(unsigned int msec);
    void signalSendNetworkCheckInfo(QByteArray netCheckFlagArray);
    void signalBarCodeGunSanDone(char codeCnt);

public slots:
    int uartTransceiver(void);
    int uartCommunicateWithStm8(char* buf, int len);
    int checkUartStm8(void);
    char slotReadStm8Uart2ForNetStatus(void);
    int slotGetEthLinkedPortNum(void);
    void slotGetStm8CmdBuf(char* buf, char *bufLen, char rdwr, char mainCmd, char subCmd, char dataLen, char* data);
    void slotLanPortCtrlAndCheck(void);
    void slotTouchBarCodeGunRecorder(void);
    int slotStartBarCodeGun(void);
    int slotWriteCharToFile(char* code, char* filePath);
    char slotSelfTransceiverSelf(void);
private:
    int fd;
    char filePath[FILE_PATH_LENGTH];
    QByteArray logArray;
};

#endif // OBJECTUART_H
