#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <QString>
#include <QPalette>
#include <QDateTime>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QTextCodec>
#include <QDir>
#include <QNetworkInterface>
#include <qpalette.h>
#include <QDateTime>
#include <QMovie>

#include "boxv3.h"
#include "objectuart.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void initNoteMap(void);
    void initDisplayScreen(void);
    void showWifiMac();

public slots:
    void slotDisplayBarCode(char status, QByteArray barArray);
    void slotDisplayCheckout(QByteArray checkout);
    void slotDisplayAgingMonitorResult(QByteArray checkout, QByteArray msg);
    void slotAllPhyCheckedDone(char result);
    void slotTcpTransceiverLogDone(void);

    void slotShowSystemDataAndTime();
    void slotShowCpuTemperature();
    void slotDisplayNotes(QString key, QString data);
    void slotShowWifiIp();

    void slotShowAlive();
signals:
    void signalRecorder(char level, QString key, QString data="");

private:
    Ui::MainWindow *ui;
    QMovie* movie;

    QTimer aliveTimer;


    char tcpDone;
    QString softVersion;

    //QString barCodeLastLineStr;
};

#endif // MAINWINDOW_H
