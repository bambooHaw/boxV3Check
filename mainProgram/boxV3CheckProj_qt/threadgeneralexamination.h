#ifndef GENERALEXAMINATIONTHREAD_H
#define GENERALEXAMINATIONTHREAD_H
#include <QThread>
#include <QByteArray>
#include <QProcess>
#include <QNetworkInterface>
#include <QDateTime>
#include "objectuart.h"
#include "objectat88sc104c.h"
#include "udpzhitongbox.h"
#include "boxv3.h"

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>


#define TIME_BUF_LENGTH 32

class threadGeneralExamination : public QThread
{
    Q_OBJECT
public:

    threadGeneralExamination();
    ~threadGeneralExamination();

    char haveBarCode;
    results_info_t checkInfo;
    checkflag_enum currentStatus;

    void displayCheckoutInfo(results_info_t* result);
    void displayAgingMonitorResult(results_info_t* result, QByteArray msg);
    char parseCheckResult(results_info_t* result);

    int tryEnableWifi(void);
    void doHWfinalCheckTask(bool timeoutFlag);
    void generateCheckoutForUserspace(results_info_t info);


    device_status_enum checkIp4G(void);
    void fdiskForTheNewBox(void);
    void clearAgingLog(void);
    QString getSysUptimeMax(void);
    void doNTPandSyncSystime(void);
signals:
    void signalRecorder(char level, QString key, QString data="");
    void signalStartUdpTrasceiver(void);
    void signalStartTcpTrasceiver(void);
    void signalSelfTransceiverSelf(void);
    void signalReadStm8Uart2ForNetStatus(void);
    void signalTcpdumpTimerKiller(uint msec);
    void signalDisplayCheckout(QByteArray checkout);
    void signalDisplayAgingMonitorResult(QByteArray checkout, QByteArray msg);
    void signalAllPhyCheckedDone(char result);
public slots:
    void slotHandlerDisplayAndNetCheckTimer(void);
    QString slotUpdateAgingCheckOut(void);
    bool slotGetNativeNetworkInfo(QString& netName, QByteArray& macArray, QByteArray& ipArray);
    bool slotGetNativeNetworkInfo(char *deviceName, QByteArray &macArray, QByteArray &ipArray);
    char* slotGetCurrentTime(void);
    char slotCheckWifi(QString& logStr, QByteArray& macArray, QByteArray& ipArray);
    char slotCheckNtpAndSysDate(void);
    char slotCheckRtcAndSysDate(void);
    char slotUpdateSysOrRtcDate(results_info_t* checkInfo);
    int  slotGetUartCheckResult(char admin, char recvSend);
    char slotCheckSpi0Flash(void);
    char slotCheckSpi0Flashby_spidev(void);
    char slotCheckSpi0Flashby_mtd(void);
    char slotCheckSpi1Lora(void);
    void slotGetNetworkCheckInfo(QByteArray netCheckFlagArray);
    char slotCheck4G(QString& logStr);

    void slotKeyHasBeenPressed(void);
    void slotDisplayBarCode(char barCodeStatus, QString str);
    void slotStartProductionCheck(void);
    void slotStartAfterAgingCheck();
protected:
    QTimer displayAndNetCheckTimer;
    void run();
};

#endif // GENERALEXAMINATIONTHREAD_H
