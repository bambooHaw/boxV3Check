#ifndef TCPSERVERTHREAD_H
#define TCPSERVERTHREAD_H
#include <QThread>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <syslog.h>

#include "tcpzhitongbox.h"
#include "boxv3.h"
#include "objectat88sc104c.h"
//#include <arpa/inet.h> //htonl

/*Attention: Please do not modified this size rishly. TCP_PVALUE_LENGTH_SIZE depends on user-defied tcp packets*/
#define TCP_PVALUE_LENGTH_SIZE          4
#define MAC_ADDR_LENGTH                 6
#define TCP_PVALUE_P2P_ID_OFFSET        0
#define TCP_PVALUE_LAN_MAC_OFFSET   (TCP_PVALUE_P2P_ID_OFFSET + P2P_ID_LENGTH)
#define TCP_PVALUE_WAN_MAC_OFFSET   (TCP_PVALUE_LAN_MAC_OFFSET + MAC_ADDR_LENGTH)
#define TCP_PVALUE_SERIAL_NUMBER_OFFSET (TCP_PVALUE_WAN_MAC_OFFSET + MAC_ADDR_LENGTH)
#define TCP_PVALUE_4G_MAC_OFFSET    (TCP_PVALUE_SERIAL_NUMBER_OFFSET + SERIAL_NUMBER_LENGTH)
#define TCP_PVALUE_WIFI_MAC_OFFSET  (TCP_PVALUE_4G_MAC_OFFSET + MAC_ADDR_LENGTH)

#define TCP_MAC_SERIAL_LENGTH (P2P_ID_LENGTH + MAC_ADDR_LENGTH*4 + SERIAL_NUMBER_LENGTH)

typedef struct _TCP_PVALUE_MAIN_INFO{
    unsigned char p2pid[P2P_ID_LENGTH];
    unsigned char macLan[MAC_ADDR_LENGTH];
    unsigned char macWan[MAC_ADDR_LENGTH];
    unsigned char serianNum[SERIAL_NUMBER_LENGTH];
    unsigned char mac4G[MAC_ADDR_LENGTH];
    unsigned char macWifi[MAC_ADDR_LENGTH];
}tcp_pvalue_main_info_t;

class threadTcpServer : public QThread
{
    Q_OBJECT

public:
    threadTcpServer();
    ~threadTcpServer();

    QByteArray currentLogFileName;

    QByteArray changeCharMacToStdMac(char* mac);
    void setLanMac(char *mac);
    void setWanMac(char *mac);
signals:
    void signalStopUdpBroadcast(void);
    void signalTcpTransceiverLogDone(void);
    void signalRecorder(char level, QString key, QString data="");
public slots:
    int slotEnableWifi();
    void showBuf(char *buf, int len);
    void showTcpMsg(void* packet);
    void showTcpLogMsg(void* packet);
    char parseHeaderfindLength(tcpMsgGetLen_t* hvl);
    void fillAppointedData(void* recvMsg, void* p, char *data, unsigned long dataLen, unsigned short feedBck);
    void fillAppointedDataForLog(void *recvMsg, void* p, char *data, unsigned int dataLen, unsigned short feedBck);
    unsigned char calcVerifyCode(void* packet);
    void* createTmpPacketAndRecvToEnd(unsigned short* ui16Length);
    void* createNewPacketAndFillIt(void* recvMsg, char* data, unsigned short dataLen, unsigned short fedBck);
    void* createLogPacketAndFillIt(void* recvMsg, char* log, unsigned short logLen, unsigned short fedBck);
    void trasHardwareSoftwareVersion(void* recvMsg);
    void trasHeartbeat(void* recvMsg);
    void setMacAndSerialNum(tcpMsgHaveData_t* packet);
    void getMacAndSerialNum(void* recvMsg);
    void trasLogFile(void* recvMsg);

    void analyzeTcpPacket(void *pack);
    void revcMessage(void);
    void transceiverMsg();
    void slotSendLogName(QByteArray logFileName);


    void slotStartTcpTrasceiver(void);
    void slotTcpTimeout(void);
protected:
    bool startedFlag;
    QTimer tcpTimeoutKeeper;
    QTcpServer* tcpServer;
    QTcpSocket* clientCon;
    void run();
};

#endif // TCPSERVERTHREAD_H
