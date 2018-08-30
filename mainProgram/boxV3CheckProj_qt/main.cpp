#include <QApplication>
#include <QTextCodec>
#include <QFont>
#include <QFontDatabase>
#include <QMessageBox>
#include <QCommandLineOption>
#include <QCommandLineParser>

#include "mainwindow.h"
#include "threadgeneralexamination.h"
#include "threadudpbroadcast.h"
#include "threadtcpserver.h"
#include "threadbarcodegun.h"
#include "threadkeydetection.h"
#include "threadlogrecorder.h"
#include "threadagingmonitor.h"
#include "threadled.h"
#include "boxv3.h"



//get argument
char getAccessToAt88(void)
{
    //Verifying access rights
    QMutexLocker lockerAt88(&mutexAT88);
    objectAT88SC104C at88((char*)AT88SC104_NODE_NAME);
    return at88.verifyCheckAccess();
}

static checkflag_enum cmdLineParser(QApplication& a)
{
    checkflag_enum checkStage = SELFCHECK_WAIT;

    QCommandLineOption optForce("f");
    optForce.setDescription("Dangerous! Force to enter in production/aging/aded check mode. NOTES: Look out! This option will modified the check flags in crypt memory!");
    optForce.setValueName("check stage");
    QCommandLineOption optJson("j");
    //optJson.setValueName("jsonFilePath");
    //optJson.setDefaultValue(QString(BOXV3_HW_CHECKOUT_FILEDIR)+ QString(BOXV3_HW_CHECKOUT_FILENAME));
    optJson.setDescription(QString("Safety. Just get checkout and write it to a json file(default: ")
                           + QString(BOXV3_HW_CHECKOUT_FILEDIR)
                           + QString(BOXV3_HW_CHECKOUT_FILENAME)
                           + QString("), wouldn't modify the flag memory."));
    QCommandLineOption optAdmin("admin");
    optAdmin.setDescription("Administrator's options(like: pass)");
    optAdmin.setValueName("wishes");

    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.addVersionOption();
    parser.addHelpOption();
    parser.addOption(optForce);
    parser.addOption(optJson);
    parser.addOption(optAdmin);
    parser.process(a);
#ifdef BOXV3_DEBUG_DISPLAY
    qDebug("########################################");
    qDebug() << parser.isSet(optForce);
    qDebug() << parser.value(optForce);
    qDebug() << parser.isSet(optJson);
    qDebug() << parser.value(optJson);
    qDebug() << parser.isSet(optAdmin);
    qDebug() << parser.value(optAdmin);
    qDebug() << parser.helpText();
    qDebug("########################################");
#endif

    QMutexLocker lockerAt88(&mutexAT88);
    objectAT88SC104C at88((char*)AT88SC104_NODE_NAME);
    if(WELLCHECK != at88.verfyPasswd())
    {
        qCritical() << "Can't access the at88sc";
        checkStage = SELFCHECK_UNKNOWN;
    }else
    {
        QString value;
        if(parser.isSet(optJson))
        {
            //1. create json file
            //checkStage = SELFCHECK_JSON;
            at88.setCheckedFlags(SELFCHECK_AGING);
            checkStage = SELFCHECK_AGED;
        }else if(parser.isSet(optForce))
        {
            //2. force to check in some kind of mode
            value = parser.value(optForce);
            if(value.contains("production"))
            {
                checkStage = SELFCHECK_PRODUCTION;
                at88.clearCheckFlags(checkStage);
            }
            else if(value.contains("aging"))
            {
                at88.setCheckedFlags(SELFCHECK_PRODUCTION);
                checkStage = SELFCHECK_AGING;
                at88.clearCheckFlags(checkStage);
                system("rm -rf /opt/private/log/boxv3/agingcheck/*");
                qCritical("rm -rf /opt/private/log/boxv3/agingcheck/*");
            }
            else if(value.contains("aged"))
            {
                at88.setCheckedFlags(SELFCHECK_AGING);
                checkStage = SELFCHECK_AGED;
                at88.clearCheckFlags(checkStage);
            }
            else
            {
                qCritical() << "Error: Unknown check mode:" << value;
                checkStage = SELFCHECK_UNKNOWN;
            }
        }else if(parser.isSet(optAdmin))
        {
            //3. exec admin commands
            value = parser.value(optAdmin);
            if(value.contains("pass"))
            {
                at88.setCheckedFlags(SELFCHECK_ADMIN_PASS);
                checkStage = SELFCHECK_ADMIN_PASS;
            }else
            {
                //other admin's cmd
                qCritical() << "Error: Unknown admin's cmd: " << value;
                checkStage = SELFCHECK_UNKNOWN;
            }

        }else
        {
            //4. Verifying access rights nature
            if(0 == at88.verifyCheckAccess())
                checkStage = SELFCHECK_WAIT;
            else
                checkStage = SELFCHECK_NO_ADMISSION;
        }
    }

    return checkStage;
}

int main(int argc, char *argv[])
{
    //optional ctrl+c
    signal(SIGINT, quit_sighandler);

    //add lib
    QApplication::addLibraryPath(BOXV3_QT_LIBRARY_PATH);
    //app
    QCoreApplication::setApplicationVersion(BOXV3CHECKAPP_VERSION);
    QApplication app(argc, argv);

    checkflag_enum stage = cmdLineParser(app);
    if(SELFCHECK_UNKNOWN == stage || SELFCHECK_NO_ADMISSION == stage)
    {
        quit_sighandler(SIGUSR1);
        return 0;
    }
    //codec
    QTextCodec* codec = QTextCodec::codecForName("utf-8"); //"GB2312"
    QTextCodec::setCodecForLocale(codec);



    //setFont
    int indexFont = QFontDatabase::addApplicationFont(QString(FONT_DIR_WENQUANYI));
    if(-1 != indexFont)
    {
        QStringList strList(QFontDatabase::applicationFontFamilies(indexFont));
        if(strList.count() > 0)
        {
            QFont thisFont(strList.at(0), 12, QFont::DemiBold);
            qApp->setFont(thisFont);
        }
    }

    /*classes*/
    MainWindow w;
    w.showFullScreen();
    QCoreApplication::processEvents();


    threadBarCodeGun barCodeGun;
    threadKeyDetection key;
    threadLogRecorder logRecorder;
    threadGeneralExamination gCheck;
    threadUdpBroadcast udpBr;
    threadTcpServer tcpST;
    threadAgingmonitor  aging;
    threadLed leds;

    //show cpu temperature
    QTimer timer;
    QObject::connect(&timer, SIGNAL(timeout()), &w, SLOT(slotShowCpuTemperature()), Qt::QueuedConnection);
    timer.start(1000); //1000

    /*connection*/
    //key bar
    QObject::connect(&barCodeGun, SIGNAL(signalSendScanedBarCode(char,QByteArray)), &key, SLOT(slotGetScanedBarCode(char,QByteArray)), Qt::DirectConnection);

    //dispaly
    QObject::connect(&key, SIGNAL(signalDisplayBarCode(char,QByteArray)), &w, SLOT(slotDisplayBarCode(char,QByteArray)), Qt::QueuedConnection);
    QObject::connect(&gCheck, SIGNAL(signalDisplayCheckout(QByteArray)), &w, SLOT(slotDisplayCheckout(QByteArray)), Qt::QueuedConnection);
    //QObject::connect(&gCheck, SIGNAL(signalAllPhyCheckedDone(char)), &w, SLOT(slotAllPhyCheckedDone(char)), Qt::QueuedConnection);
    QObject::connect(&gCheck, SIGNAL(signalDisplayAgingMonitorResult(QByteArray,QByteArray)), &w, SLOT(slotDisplayAgingMonitorResult(QByteArray,QByteArray)), Qt::QueuedConnection);


    //log msg
    //log file slot exec option must be queued
    QObject::connect(&w, SIGNAL(signalRecorder(char,QString,QString)), &logRecorder, SLOT(slotRecorder(char,QString,QString)), Qt::QueuedConnection);
    QObject::connect(&barCodeGun, SIGNAL(signalRecorder(char,QString,QString)), &logRecorder, SLOT(slotRecorder(char,QString,QString)), Qt::QueuedConnection);
    QObject::connect(&key, SIGNAL(signalRecorder(char,QString,QString)), &logRecorder, SLOT(slotRecorder(char,QString,QString)), Qt::QueuedConnection);
    QObject::connect(&gCheck, SIGNAL(signalRecorder(char,QString,QString)), &logRecorder, SLOT(slotRecorder(char,QString,QString)), Qt::QueuedConnection);
    QObject::connect(&tcpST, SIGNAL(signalRecorder(char,QString,QString)), &logRecorder, SLOT(slotRecorder(char,QString,QString)), Qt::QueuedConnection);
    QObject::connect(&aging, SIGNAL(signalRecorder(char,QString,QString)), &logRecorder, SLOT(slotRecorder(char,QString,QString)), Qt::QueuedConnection);
    QObject::connect(&udpBr, SIGNAL(signalRecorder(char,QString,QString)), &logRecorder, SLOT(slotRecorder(char,QString,QString)), Qt::QueuedConnection);

    //log display
    QObject::connect(&logRecorder, SIGNAL(signalDisplayNotes(QString,QString)), &w, SLOT(slotDisplayNotes(QString,QString)), Qt::QueuedConnection);


    //get logname from logrecorder thread
    QObject::connect(&logRecorder, SIGNAL(signalSendLogName(QByteArray)), &tcpST, SLOT(slotSendLogName(QByteArray)), Qt::DirectConnection);
    QObject::connect(&logRecorder, SIGNAL(signalSendLogName(QByteArray)), &aging, SLOT(slotGetLogName(QByteArray)), Qt::DirectConnection);

    //start generanl / aging /aged check, Or stop it
    QObject::connect(&logRecorder, SIGNAL(signalStartProductionCheck()), &gCheck, SLOT(slotStartProductionCheck()), Qt::DirectConnection);
    QObject::connect(&logRecorder, SIGNAL(signalStartAgingCheck()), &aging, SLOT(slotStartAgingThread()), Qt::DirectConnection);
    QObject::connect(&logRecorder, SIGNAL(signalStartAfterAgingCheck()), &gCheck, SLOT(slotStartAfterAgingCheck()), Qt::DirectConnection);
    QObject::connect(&logRecorder, SIGNAL(signalStopAgingCheck()), &aging, SLOT(slotStopAgingThread()), Qt::DirectConnection);

    //start udp/tcp, if all check are ok. Or, stop it.
    //start tcp when all threadGeneralExamination is ok, and keep it running all time
    //close udp service when a new tcp connected success
    QObject::connect(&gCheck, SIGNAL(signalStartUdpTrasceiver()), &udpBr, SLOT(slotStartUdpTransceiver()), Qt::QueuedConnection);
    QObject::connect(&gCheck, SIGNAL(signalStartTcpTrasceiver()), &tcpST, SLOT(slotStartTcpTrasceiver()), Qt::QueuedConnection);
    QObject::connect(&tcpST, SIGNAL(signalStopUdpBroadcast()), &udpBr, SLOT(slotStopUdpTransceiver()), Qt::QueuedConnection);

    //work stage change time
    QObject::connect(&tcpST, SIGNAL(signalTcpTransceiverLogDone()), &w, SLOT(slotTcpTransceiverLogDone()));

    //led
    QObject::connect(&gCheck, SIGNAL(signalDisplayCheckout(QByteArray)), &leds, SLOT(slotDisplayCheckout(QByteArray)));
    QObject::connect(&aging, SIGNAL(signalLedShowAgingResult(QByteArray)), &leds, SLOT(slotLedShowAgingResult(QByteArray)), Qt::QueuedConnection);
    QObject::connect(&aging, SIGNAL(signalSetFlagLed(char)), &leds, SLOT(slotSetFlagLed(char)), Qt::DirectConnection);


    /*start logging first*/
    logRecorder.start();

    /*check access*/
    switch(getAccessToAt88())
    {
    case -EACCES:
    {
        //qCritical() << "error at88 EACCES" << endl;
        //w.slotDisplayNotes("", "error at88 EACCES");
        emit key.signalRecorder(LOG_CRIT, "", "error at88 EACCES");
        QCoreApplication::processEvents();
        sleep(1);
        return EACCES;
    }
    case -ENXIO:
    {
        emit key.signalRecorder(LOG_CRIT, "", "warning at88 ENXIO");
    }
    default:
    {
        emit key.signalRecorder(LOG_CRIT, "", "Success access at88");
        break;
    }
    }

    barCodeGun.start();
    key.start();
    udpBr.start();
    tcpST.start();
    leds.start();

    return app.exec();
}
