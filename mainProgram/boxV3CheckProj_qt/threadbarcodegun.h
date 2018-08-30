#ifndef BARCODEGUNTHREAD_H
#define BARCODEGUNTHREAD_H

#include <QObject>
#include <QThread>
#include <QFile>
#include <QDebug>
//#include <QtSerialPort/QSerialPort>
//#include <QtSerialPort/qserialportinfo.h>


#include "objectuart.h"
#include "boxv3.h"

class threadBarCodeGun : public QThread
{
    Q_OBJECT
public:
    threadBarCodeGun();
    ~threadBarCodeGun();

    QByteArray barCodeArray;
    void showBuf(char *buf, int len);
signals:
    void signalSendScanedBarCode(char result, QByteArray barCodeArray);
    void signalRecorder(char level, QString key, QString data="");
public slots:
    //void slotSetTmpSerial(QSerialPort* port);
    //void slotReadData(void);
    barCodeGunScanResult_enum slotAnalysizeBarCode(QString barCodeStr);
    char slotReadScanedBarCode(void);
    void slotBarCodeGunScanDone(char codeCnt);
private:
    //QSerialPort* tmpSerial;
protected:
    void run(void);
};

#endif // BARCODEGUNTHREAD_H
