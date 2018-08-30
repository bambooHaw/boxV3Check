#ifndef THREADAGINGMONITOR_H
#define THREADAGINGMONITOR_H

#include <QObject>
#include <QThread>
#include <QDateTime>
#include <QTimer>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <syslog.h>
#include <objectat88sc104c.h>
#include "boxv3.h"

class threadAgingmonitor : public QThread
{
    Q_OBJECT
public:
    threadAgingmonitor();
    ~threadAgingmonitor();

    QString maxUptimeLogFile;
    QString agingLogPath;
    QString uptimeMax;
signals:
    void signalRecorder(char level, QString key, QString data);
    //void signalSpecialDisplay(QString currentUptime);
    void signalSetFlagLed(char flag);
    void signalLedShowAgingResult(QByteArray checkout);
public slots:
    void slotStartAgingThread(void);
    void slotStopAgingThread(void);
    void slotRecorderSysUptime(void);
    QString slotGetMaxSysUptime(void);
    void slotGetLogName(QByteArray currentLogFileName);
protected:
    void run();
};

#endif // THREADAGINGMONITOR_H
