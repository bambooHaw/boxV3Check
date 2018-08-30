#ifndef THREADLOGRECORDER_H
#define THREADLOGRECORDER_H

#include <QThread>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QMutex>
#include <QMutexLocker>
#include <syslog.h>
#include <QDebug>
#include "boxv3.h"

#define TIME_BUF_LENGTH 32

class threadLogRecorder : public QThread
{
    Q_OBJECT
public:
    threadLogRecorder();
    checkflag_enum currentCheckStage;
    QMutex logMutex;
    QByteArray currentLogFile;

    char *getCurrentTime();
    QString createLogFile(checkflag_enum stage);
    void deleteCurrentAgingLogFile(void);
    void initNoteMap();
signals:
    void signalSendLogName(QByteArray currentLogFileName);
    void signalStopAgingCheck(void);
    void signalStartProductionCheck(void);
    void signalStartAgingCheck(void);
    void signalStartAfterAgingCheck(void);
    void signalDisplayNotes(QString key, QString data);
public slots:
    void slotGetLogFile(void);
    void slotWriteLogItem(QString logArray);
    void slotRecorder(char level, QString key, QString data);
    void slotGetCheckStageAndCreateNewLogFile(checkflag_enum status);
private:
    //key_boxv3_status_t checkStatus;
    QMap<QString, QString> noteMap;
protected:
    bool proLogCreated;
    bool agingLogCreated;
    bool agedLogCreated;
    void run(void);
};

#endif // THREADLOGRECORDER_H
