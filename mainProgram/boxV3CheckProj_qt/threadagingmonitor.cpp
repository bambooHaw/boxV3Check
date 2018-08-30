#include "threadagingmonitor.h"

threadAgingmonitor::threadAgingmonitor()
{

    //
    uptimeMax = QString("");
    //create dir
    QByteArray cmdArray;
    cmdArray.append("mkdir -p ");
    cmdArray.append((char*)BOXV3_SELFCHECK_LOGDIR_PATH);
    cmdArray.append((char*)BOXV3_SELFCHECK_DIR_PREFIX_AGING);
    cmdArray.append((char*)BOXV3_SELFCHECK_DIR_PREFIX_UPTIME);
    system(cmdArray.data());


    QDir dir(QString(BOXV3_SELFCHECK_LOGDIR_PATH) + QString(BOXV3_SELFCHECK_DIR_PREFIX_AGING) + QString(BOXV3_SELFCHECK_DIR_PREFIX_UPTIME));
    QStringList infolist = dir.entryList(QDir::NoDotAndDotDot | QDir::Files);

    //qDebug("infolist.size(): %d.", infolist.size());
    if(1 != infolist.size())
    {
        //rm all the file
        QString cmdStr("rm -rf ");
        cmdStr += BOXV3_SELFCHECK_LOGDIR_PATH;
        cmdStr += BOXV3_SELFCHECK_DIR_PREFIX_AGING;
        cmdStr += BOXV3_SELFCHECK_DIR_PREFIX_UPTIME;
        cmdStr += "*";
        system(cmdStr.toLocal8Bit().data());

        //name the uptime recorder file
        cmdStr = "touch ";
        maxUptimeLogFile = QString("");
        maxUptimeLogFile.append((char*)BOXV3_SELFCHECK_LOGDIR_PATH);
        maxUptimeLogFile.append((char*)BOXV3_SELFCHECK_DIR_PREFIX_AGING);
        maxUptimeLogFile.append((char*)BOXV3_SELFCHECK_DIR_PREFIX_UPTIME);
        maxUptimeLogFile.append(QDateTime::currentDateTime().toString("yyyy_MM_dd[HH:mm:ss]"));
        maxUptimeLogFile.append((char*)BOXV3_SELFCHECK_LOGNAME_SUFFIX);
        cmdStr += maxUptimeLogFile;
        system(cmdStr.toLocal8Bit().data());
        qDebug("%s", cmdStr.toLocal8Bit().data());
    }else
    {
        QString filename = infolist.at(0);
        maxUptimeLogFile = QString(BOXV3_SELFCHECK_LOGDIR_PATH) + QString(BOXV3_SELFCHECK_DIR_PREFIX_AGING) + QString(BOXV3_SELFCHECK_DIR_PREFIX_UPTIME) + filename;

        //qDebug("dirpath:%s", dir.absolutePath().toLocal8Bit().data());

    }
    //qDebug("maxUptimelogfilepath:%s", maxUptimeLogFile.toLocal8Bit().data());

    moveToThread(this);
}

threadAgingmonitor::~threadAgingmonitor()
{

}

void threadAgingmonitor::slotStartAgingThread()
{
    this->start();
}

void threadAgingmonitor::slotStopAgingThread()
{
    this->terminate();
}

void threadAgingmonitor::slotRecorderSysUptime()
{
    QFile file("/proc/uptime");
    QString maxUptime;
    if(!file.open(QIODevice::ReadOnly))
    {

    }else
    {
        QTextStream io(&file);
        maxUptime = io.readLine().section(" ", 0, 0, QString::SectionSkipEmpty);
        file.close();
    }

    QString cmdStr("echo ");
    cmdStr.append(maxUptime);
    cmdStr.append(" >> ");
    cmdStr.append(maxUptimeLogFile);

    system(cmdStr.toLocal8Bit().data());
    emit signalRecorder(LOG_ALERT, "currentMaxUptime", maxUptime);
}

QString threadAgingmonitor::slotGetMaxSysUptime(void)
{
    static QString tmpUptime;
    QFile file(maxUptimeLogFile);
    if(!file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        qDebug("Error: Can't open updateFile(%s).", maxUptimeLogFile.toLocal8Bit().data());
        uptimeMax = "";
    }else
    {
        QTextStream data(&file);
        while(!data.atEnd())
        {
            QString line;
            line = data.readLine();
            //qDebug("line:%s", line.toLocal8Bit().data());
            if(uptimeMax.length() < line.length())
            {
                uptimeMax = line;
            }else if(uptimeMax.length() == line.length())
            {
                if(uptimeMax < line)
                {
                    uptimeMax = line;
                }
            }
        }
        file.close();
    }
    //qDebug("uptimeMax:%s", uptimeMax.toLocal8Bit().data());
#if 1
    float seconds = uptimeMax.toFloat();
    long min = seconds/60;
    long hour = min/60;
    min = min%60;
    QString uptimeLog("uptimeMax= ");
    uptimeLog += QString::number(hour, 10);
    uptimeLog += "h";
    uptimeLog += QString::number(min, 10);
    uptimeLog += "m";
    if(hour >= (long)48)
    {
        emit signalSetFlagLed(WELLCHECK);
        uptimeLog += " (agingCheck time is ok)";
        emit signalRecorder(LOG_ALERT, "uptimeMax", uptimeLog);
    }else
    {
        emit signalSetFlagLed(ONCHECK);
        uptimeLog += " (agingCheck is going on...)";
    }

    if(0 != uptimeLog.compare(tmpUptime))
    {
        tmpUptime = uptimeLog;
        emit signalRecorder(LOG_NOTICE, "", uptimeLog);
    }
#endif

    return uptimeMax;
}

void threadAgingmonitor::slotGetLogName(QByteArray currentLogFileName)
{
    //get the aging log File name from log recorder thread
    agingLogPath = QString(currentLogFileName);
    qDebug("Get aing log File name:%s", agingLogPath.toLocal8Bit().data());
}

void threadAgingmonitor::run()
{
    qDebug() << "threadAgingmonitor...";

    if(1)
    {
        QByteArray deviceCode;
        deviceCode.resize(12);
        objectAT88SC104C at88((char*)AT88SC104_NODE_NAME);
        at88.getDeviceCode(deviceCode.data());
        emit signalRecorder(LOG_CRIT, "serialNum", QString(deviceCode));
    }


    slotRecorderSysUptime();
    slotGetMaxSysUptime();
    QTimer uptimeTimer;
    QObject::connect(&uptimeTimer, SIGNAL(timeout()), this, SLOT(slotRecorderSysUptime()), Qt::QueuedConnection);
    QObject::connect(&uptimeTimer, SIGNAL(timeout()), this, SLOT(slotGetMaxSysUptime()), Qt::QueuedConnection);

    qDebug() << __func__<<  __LINE__ << endl;
    uptimeTimer.setInterval(60000);
    QTimer::singleShot(0, &uptimeTimer,SLOT(start())); //60000
    qDebug() << __func__<<  __LINE__ << endl;

    system("killall -9 agingNetCheckApp");
    system("chmod 777 /opt/agingNetCheckApp");
    system("/opt/agingNetCheckApp &");

    QByteArray checkout;
    emit signalLedShowAgingResult(checkout);

    exec();
}

