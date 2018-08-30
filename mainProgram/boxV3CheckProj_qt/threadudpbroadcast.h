#ifndef THREADUDPBROADCAST_H
#define THREADUDPBROADCAST_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QDebug>
#include <syslog.h>
#include "boxv3.h"
#include "udpzhitongbox.h"
#include "objectat88sc104c.h"

class threadUdpBroadcast : public QThread
{
    Q_OBJECT
public:
    threadUdpBroadcast();
    ~threadUdpBroadcast();
    void getNativeNetworkInfo();
    void changeMacToNum(char *dst, char *src);
    void cpImportantMsgFromServer(udpMsg_t *dst, udpMsg_t *src);
    void sendUdpDateToServer();
    void sendMainUdpDateToServer();
    void getSelfSign(unsigned short *sign);
    void fillUdpDataPackage();

signals:
    void signalRecorder(char level, QString key, QString data="");
public slots:
    void slotStartUdpTransceiver();
    void slotStopUdpTransceiver();
    void slotProcessPendingDatagrams();
    void slotMainProcessPendingDatagrams();
    int slotEnableWifi();
    void slotUdpTimeout(void);

protected:
    udpMsg_t udpMsg;
    QUdpSocket* udpsocketRecv;
    QUdpSocket* udpsocketSend;
    QUdpSocket* udpsocketMainRecv;
    QUdpSocket* udpsocketMainSend;
    QHostAddress udpServerHost;
    quint16 udpServerPort;
    QHostAddress currentUdpServerHost;
    quint16 currentUdpServerPort;

    bool startedUdpFlag;
    QTimer udpTimeKeeper;
    void run(void);
};

#endif // THREADUDPBROADCAST_H
