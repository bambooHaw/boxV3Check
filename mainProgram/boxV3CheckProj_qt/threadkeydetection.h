#ifndef THREADKEYDETECTION_H
#define THREADKEYDETECTION_H

#include <QByteArray>
#include <QFileInfo>
#include <QThread>
#include "boxv3.h"
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include "objectat88sc104c.h"


class threadKeyDetection : public QThread
{
    Q_OBJECT
public:
    threadKeyDetection();

    void tryGetLatestTime();
private:
    key_boxv3_status_t currentStatus;
    QString logStr;
    QByteArray currentBarCodeArray;
signals:
    void signalDisplayBarCode(char status, QByteArray barArry);
    void signalRecorder(char level, QString key, QString data="");
public slots:
    int slotStartKeyDect(void);
    void slotKeyHasBeenPressed(void);
    void slotGetScanedBarCode(char res, QByteArray barCodeArray);
protected:
    void run();
};

#endif // THREADKEYDETECTION_H
