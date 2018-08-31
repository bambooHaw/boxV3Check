#include "threadgeneralexamination.h"

#if 1
extern "C" {
#include "ztboxc_deprecated.h"
}
#endif

threadGeneralExamination::threadGeneralExamination()
{
    haveBarCode = 0;
    currentStatus = SELFCHECK_WAIT;


    //clear checkout json file history
    QByteArray cmdArray("rm -rf ");
    cmdArray.append(BOXV3_HW_CHECKOUT_FILEDIR);
    cmdArray.append(BOXV3_HW_CHECKOUT_FILENAME);
    system(cmdArray.data());

    moveToThread(this);
}

threadGeneralExamination::~threadGeneralExamination()
{
}


device_status_enum threadGeneralExamination::checkIp4G()
{
    device_status_enum check;

    QString netName("usb0");
    QString ipStr;
    if(getIfconfigIp(netName, ipStr) == 0){
        if(ipStr.isEmpty()){
            check =  FAILEDCHECK;
        }else
        {
            check =  WELLCHECK;
        }
    }else{
        check = FAILEDCHECK;
    }

    return check;
}

void threadGeneralExamination::fdiskForTheNewBox()
{
    emit signalRecorder(LOG_CRIT, "", "<span style=\"color:red\">正在初始化Flash，请稍后...</span>");
    char ret = 0;
    ret = system(BOXV3_SHELL_FLASH_INIT_PART0);
    if(!ret)
    {
        emit signalRecorder(LOG_CRIT, "", "Flash part0 init success!");
    }else
    {
        QString info("Error: Flash part0 init failed! code:");
        info += QString::number(ret, 10);
        emit signalRecorder(LOG_CRIT,"", info);
    }
    ret = system(BOXV3_SHELL_FLASH_INIT_PART1);
    if(!ret)
    {
        emit signalRecorder(LOG_CRIT, "", "Flash part1 init success!");
    }else
    {
        QString info("Error: Flash part1 init failed! code:");
        info += QString::number(ret, 10);
        emit signalRecorder(LOG_CRIT,"", info);
    }
}

void threadGeneralExamination::clearAgingLog()
{
    QByteArray cmdArray("rm -rf ");
    cmdArray.append(BOXV3_SELFCHECK_LOGDIR_PATH);
    cmdArray.append(BOXV3_SELFCHECK_DIR_PREFIX_AGING);
    cmdArray.append("*");
    system(cmdArray.data());
}

QString threadGeneralExamination::getSysUptimeMax(void)
{
    QString maxUptime;

    QDir dir(QString(BOXV3_SELFCHECK_LOGDIR_PATH) + QString(BOXV3_SELFCHECK_DIR_PREFIX_AGING) + QString(BOXV3_SELFCHECK_DIR_PREFIX_UPTIME));
    QString filename;

    QStringList list = dir.entryList(QDir::NoDotAndDotDot | QDir::Files | QDir::NoSymLinks);
    if(list.size() > 0)
    {
        filename = dir.absolutePath();
        filename += "/";
        filename += list.at(0);
        QFile file(filename);
        if(!file.open(QIODevice::ReadOnly))
        {
            maxUptime = "read error";
        }else
        {
            QTextStream io(&file);
            float uptimeMaxFTmp = 0.0;
            while(!io.atEnd())
            {
                QString line = io.readLine();
                if(uptimeMaxFTmp < line.toFloat())
                {
                    uptimeMaxFTmp = line.toFloat();
                }
            }
            file.close();
            maxUptime = QString::number(uptimeMaxFTmp);
        }

    }else
    {
        maxUptime = "no record";
    }

    return maxUptime;
}

void threadGeneralExamination::doNTPandSyncSystime()
{
    QString logStr;

    //2.1 check ntpclient, again
    if(WELLCHECK != checkInfo.ntpdate)
        checkInfo.ntpdate = slotCheckNtpAndSysDate();

    if(WELLCHECK == checkInfo.ntpdate)
    {
        logStr = "Success: NtpClient was checked ok! Date: ";
        logStr += QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss");
        //qDebug() << logStr.toLocal8Bit().data();
    }else
    {
        logStr = "Error  : NTP client check failed! Connect the World NTP server failed!";
        //qDebug() << logStr.toLocal8Bit().data();
    }
    emit signalRecorder(LOG_CRIT, "", logStr);

    //4. update systime or rtc
    switch(slotUpdateSysOrRtcDate(&checkInfo))
    {
    case WELLCHECK:
    {
        checkInfo.sysTime = WELLCHECK;
        logStr = "Success: sysTime and rtcTime both have been updated from internet.";
        logStr += "\nRtcTime: ";
        system("rm -rf /tmp/rtcChecked.txt");
        system("hwclock > /tmp/rtcChecked.txt");
        QFile file("/tmp/rtcChecked.txt");
        if(!file.open(QIODevice::ReadOnly|QIODevice::Text))
        {
            logStr += "rtcTime tmpFile open failed!";
        }else
        {
            QTextStream io(&file);
            logStr += io.readAll();
            file.close();
        }
        //qDebug() << logStr.toLocal8Bit().data();
        break;
    }
    case FAILEDEVENT1:
    {
        checkInfo.sysTime = WELLCHECK;
        logStr = "Warning: RTC does not work, only sysTime was updated from internet!";
        //qDebug() << logStr.toLocal8Bit().data();
        break;
    }
    case FAILEDEVENT2:
    case FAILEDEVENT3:
    default:
    {
        checkInfo.sysTime = FAILEDCHECK;
        logStr = "Error: Can't get time from internet. System time may not correct!";
        //qDebug() << logStr.toLocal8Bit().data();
        break;
    }
    }
    emit signalRecorder(LOG_CRIT, "", logStr);
}

void threadGeneralExamination::displayCheckoutInfo(results_info_t *result)
{
    QByteArray arrayCheckInfo;
    arrayCheckInfo.resize(sizeof(results_info_t));

    if(!result) return;

    memcpy(arrayCheckInfo.data(), result, sizeof(results_info_t));
    emit signalDisplayCheckout(arrayCheckInfo);
}

void threadGeneralExamination::displayAgingMonitorResult(results_info_t *result, QByteArray msg)
{
    QByteArray arrayCheckInfo;
    arrayCheckInfo.resize(sizeof(results_info_t));

    if(!result) return;

    memcpy(arrayCheckInfo.data(), result, sizeof(results_info_t));
    emit signalDisplayAgingMonitorResult(arrayCheckInfo, msg);
}

char threadGeneralExamination::parseCheckResult(results_info_t *result)
{
    if(WELLCHECK != result->wifi) return FAILEDCHECK;
    if(WELLCHECK != result->ntpdate) return FAILEDCHECK;
    if(WELLCHECK != result->rtc) return FAILEDCHECK;
    if((WELLEVENT1|WELLEVENT2) != result->uart232) return FAILEDCHECK;
    if(WELLCHECK != result->spi0Flash) return FAILEDCHECK;
    if(WELLCHECK != result->spi1Lora) return FAILEDCHECK;
    if(WELLCHECK != result->iicAt88sc104c) return FAILEDCHECK;
    if(WELLCHECK != result->uart2Stm8) return FAILEDCHECK;
    if(WELLCHECK != result->network4G) return FAILEDCHECK;

    if(WELLCHECK != result->networkLanPort1) return FAILEDCHECK;
    if(WELLCHECK != result->networkLanPort2) return FAILEDCHECK;
    if(WELLCHECK != result->networkLanPort3) return FAILEDCHECK;
    if(WELLCHECK != result->networkLanPort4) return FAILEDCHECK;
    if(WELLCHECK != result->networkWan) return FAILEDCHECK;
    if(SELFCHECK_AGED == currentStatus)
        if(WELLCHECK != result->agingResult) return FAILEDCHECK;

    return WELLCHECK;
}

int threadGeneralExamination::tryEnableWifi()
{
    int ret = 0;

    ret = system("ifconfig wlan0 up");
    if(ret)return ret;
    system("killall -9 hostapd");
    system("killall -9 wpa_supplicant");
    ret = system("wpa_supplicant -i wlan0 -c /etc/wpa_supplicant.conf &");
    if(ret)return ret;
    ret = system("udhcpc -i wlan0 -b -q -t 1");

    return ret;
}

void threadGeneralExamination::doHWfinalCheckTask(bool timeoutFlag)
{

    //13. do wifi check in the end
    QByteArray wifiMac;
    QByteArray wifiIp;
    QString logStr;
    checkInfo.wifi = slotCheckWifi(logStr, wifiMac, wifiIp);
    if(WELLCHECK == checkInfo.wifi)
    {
        emit signalRecorder(LOG_CRIT, "success wifiCheck");
    }else
    {
        qDebug("checkInfo.wifi:%d", checkInfo.wifi);
        emit signalRecorder(LOG_CRIT, "error wifiCheck");
    }
    emit signalRecorder(LOG_CRIT, "", logStr);

    //display check info
    displayCheckoutInfo(&checkInfo);

#ifndef BOXV3_DEBUG_NET
    //14. sync time
    if(WELLCHECK == checkInfo.wifi)
    {
        doNTPandSyncSystime();
        emit signalRecorder(LOG_CRIT, "", QDateTime::currentDateTime().toString("yyyy/MM/dd-HH:mm:ss"));
    }else
    {
        emit signalRecorder(LOG_CRIT, "", "WIFI network is poor, ntpclient can't work.");
    }
    //15. cp logfile


    //generate json txt for other user to use checkout
    generateCheckoutForUserspace(checkInfo);

    checkInfo.whole = parseCheckResult(&checkInfo);
#else
    checkInfo.whole = WELLCHECK;
#endif

    //display check info
    displayCheckoutInfo(&checkInfo);

    //13. if net all ok , start the udp service
    if(WELLCHECK == checkInfo.whole)
    {
        qDebug() << "all generalExaminationThread is ok, start udp broadcast thread.";
        emit signalRecorder(LOG_NOTICE, "success0");
        emit signalRecorder(LOG_NOTICE, "", "prepareCommunicationEnv...");
        prepareCommunicationEnv();
        //emit signalAllPhyCheckedDone((char)WELLCHECK);
        emit signalStartUdpTrasceiver();
        emit signalStartTcpTrasceiver();
    }else
    {
        qDebug() << "all generalExaminationThreads have been done, but something is wrong!";
        //emit signalAllPhyCheckedDone((char)FAILEDCHECK);
        if(timeoutFlag)
        {
            signalRecorder(LOG_NOTICE, "failed1");
        }else
        {
            signalRecorder(LOG_NOTICE, "failed0");
        }
    }

    //stop generation check immediately
    //this->terminate();
    //this->quit();
}

void threadGeneralExamination::generateCheckoutForUserspace(results_info_t info)
{

    QDir dir(BOXV3_HW_CHECKOUT_FILEDIR);
    if(!dir.exists())
    {
        dir.mkpath(BOXV3_HW_CHECKOUT_FILEDIR);
    }
    //touch checkout file
    QByteArray cmdArray("touch ");
    cmdArray.append(BOXV3_HW_CHECKOUT_FILEDIR);
    cmdArray.append(BOXV3_HW_CHECKOUT_FILENAME);
    system(cmdArray.data());

    QString line;
    QFile file(QString(BOXV3_HW_CHECKOUT_FILEDIR) + QString(BOXV3_HW_CHECKOUT_FILENAME));
    file.open(QIODevice::ReadWrite|QIODevice::Text);
    QTextStream io(&file);
    io << "{\n";
    io << "\"jsonline\":\"start\",\n";

    //time:
    line = "\"sysTime\":";
    line += "\"";
    line += QDateTime::currentDateTime().toString("yyyy_MM_dd[HH-mm-ss]");
    line += "\",";
    io << line << endl;

    //serialNum:
    line = "\"serialNum\":";
    line += "\"";
    if(1)
    {
        QMutexLocker lockerAt88(&mutexAT88);
        objectAT88SC104C at88((char*)AT88SC104_NODE_NAME);
        QByteArray deviceCode;
        deviceCode.resize(AT88SC104_BUF_LEGTH);
        if(at88.getDeviceCode(deviceCode.data()))
            line += QString(deviceCode);
        else
            line += QString("at88sc can't access.");
    }
    line += "\",";
    io << line << endl;

    //1. write txt for wifi
    line = "\"wifi\":";
    if(WELLCHECK == info.wifi) line += "\"ok\",";
    else if(NOTCHECK == info.wifi) line += "\"unchecked\",";
    else line += "\"failed\",";
    io << line << endl;
    //2. ntpclient
    line = "\"ntpClient\":";
    if(WELLCHECK == info.ntpdate) line += "\"ok\",";
    else if(NOTCHECK == info.ntpdate) line += "\"unchecked\",";
    else line += "\"failed\",";
    io << line << endl;
    //3. rtc
    line = "\"rtc\":";
    if(WELLCHECK == info.rtc) line += "\"ok\",";
    else if(NOTCHECK == info.rtc) line += "\"unchecked\",";
    else line += "\"failed\",";
    io << line << endl;
    //4. systime
        //agnore
    //5. uart232
    line = "\"uart232\":";
    if((WELLEVENT1 | WELLEVENT2) == info.uart232) line += "\"ok\",";
    else if(NOTCHECK == info.uart232) line += "\"unchecked\",";
    else line += "\"failed\",";
    io << line << endl;
    //6. spi-flash
    line = "\"spi0Flash\":";
    if(WELLCHECK == info.spi0Flash) line += "\"ok\",";
    else if(NOTCHECK == info.spi0Flash) line += "\"unchecked\",";
    else line += "\"failed\",";
    io << line << endl;
    //7. spi-lora
    line = "\"spi1Lora\":";
    if(WELLCHECK == info.spi1Lora) line += "\"ok\",";
    else if(NOTCHECK == info.spi1Lora) line += "\"unchecked\",";
    else line += "\"failed\",";
    io << line << endl;
    //8. at88sc104
    line = "\"iicAt88sc104c\":";
    if(WELLCHECK == info.iicAt88sc104c) line += "\"ok\",";
    else if(NOTCHECK == info.iicAt88sc104c) line += "\"unchecked\",";
    else line += "\"failed\",";
    io << line << endl;
    //9. stm8
    line = "\"uart2Stm8\":";
    if(WELLCHECK == info.uart2Stm8) line += "\"ok\",";
    else if(NOTCHECK == info.uart2Stm8) line += "\"unchecked\",";
    else line += "\"failed\",";
    io << line << endl;
    //10. 4g
    line = "\"network4G\":";
    if(WELLCHECK == info.network4G) line += "\"ok\",";
    else if(NOTCHECK == info.network4G) line += "\"unchecked\",";
    else line += "\"failed\",";
    io << line << endl;
    //11. Wan0
    line = "\"networkWan\":";
    if(WELLCHECK == info.networkWan) line += "\"ok\",";
    else if(NOTCHECK == info.networkWan) line += "\"unchecked\",";
    else line += "\"failed\",";
    io << line << endl;
    //Lan1
    line = "\"networkLanPort1\":";
    if(WELLCHECK == info.networkLanPort1) line += "\"ok\",";
    else if(NOTCHECK == info.networkLanPort1) line += "\"unchecked\",";
    else line += "\"failed\",";
    io << line << endl;
    //Lan2
    line = "\"networkLanPort2\":";
    if(WELLCHECK == info.networkLanPort2) line += "\"ok\",";
    else if(NOTCHECK == info.networkLanPort2) line += "\"unchecked\",";
    else line += "\"failed\",";
    io << line << endl;
    //Lan3
    line = "\"networkLanPort3\":";
    if(WELLCHECK == info.networkLanPort3) line += "\"ok\",";
    else if(NOTCHECK == info.networkLanPort3) line += "\"unchecked\",";
    else line += "\"failed\",";
    io << line << endl;
    //Lan4
    line = "\"networkLanPort4\":";
    if(WELLCHECK == info.networkLanPort4) line += "\"ok\",";
    else if(NOTCHECK == info.networkLanPort4) line += "\"unchecked\",";
    else line += "\"failed\",";
    io << line << endl;

    io << "\"jsonline\":\"end\"\n";
    io << "}\n";
    file.close();

}


void threadGeneralExamination::slotHandlerDisplayAndNetCheckTimer()
{
#ifndef BOXV3_DEBUG_NET
    static char checkedPort = 0;
    static char timerTrigCnt = 0;
    static bool emitted4GCheckout = false;
    static bool emittedWireNetCheckout = false;
    static bool flagStartedWireCheck = false;
    static QString log4GStr;
    if(NOTCHECK == checkInfo.uart2Stm8)
    {
        ;
    }else
    {
        timerTrigCnt++;
        //wait 4g check thread done
        if((WELLCHECK != checkInfo.network4G) &&\
                (FAILEDCHECK != checkInfo.network4G) &&\
                (OVERTIMECHECK != checkInfo.network4G))
        {
            //overtime 60s
            if(timerTrigCnt > 60)
            {
                checkInfo.network4G = OVERTIMECHECK;
                //goto start check wirenet
            }
            else
            {
                if(0 == timerTrigCnt%4) //11. check 4g every 4s, before wan check
                {
                    checkInfo.network4G = slotCheck4G(log4GStr);
                }
            }
        }else
        {
            if(WELLCHECK == checkInfo.network4G)
            {
                //2.0 check ntp first time when 4G works well
                checkInfo.ntpdate = slotCheckNtpAndSysDate();
                //if failed at here, will be check again after wifi check
                if(WELLCHECK != checkInfo.ntpdate) checkInfo.ntpdate = NOTCHECK;
            }

            //12. wait all wirenet check done
            if((0xf0 == checkedPort) || (emittedWireNetCheckout))
            {
                if(NOTCHECK == checkInfo.whole)
                {
                    doHWfinalCheckTask(emittedWireNetCheckout);
                }else
                {
                    ;
                }
            }else
            {
                //send 4g checkout once
                if(!emitted4GCheckout)
                {
                    emitted4GCheckout = true;
                    emit signalRecorder(LOG_CRIT, "", log4GStr);
                }

                //wait wan&Lan check thread done
                if(timerTrigCnt >= 180)
                {
                    emittedWireNetCheckout = true;
                }
                //10. net check(WAN and LAN) thread wait for check
                if(!flagStartedWireCheck)
                {
                    emit signalReadStm8Uart2ForNetStatus();
                    flagStartedWireCheck = true;
                }
                if(NOTCHECK != checkInfo.networkLanPort4) checkedPort |= 0x80;
                if(NOTCHECK != checkInfo.networkLanPort3) checkedPort |= 0x40;
                if(NOTCHECK != checkInfo.networkLanPort2) checkedPort |= 0x20;
                if(NOTCHECK != checkInfo.networkLanPort1) checkedPort |= 0x10;

            }
        }
    }
#endif
    //display check info
    displayCheckoutInfo(&checkInfo);
}

QString threadGeneralExamination::slotUpdateAgingCheckOut()
{
    QString uptimeMax = getSysUptimeMax();

    float seconds = uptimeMax.toFloat();
    long min = seconds/60;
    long hour = min/60;
    min = min%60;

    if(hour < BOXV3_AGING_MIN_TIME)
    {
        checkInfo.agingResult = FAILEDCHECK;
    }else
    {
        checkInfo.agingResult = WELLCHECK;
    }

    QString msg = QString::number(hour);
    msg += "h";
    msg += QString::number(min);
    msg += "m";
    displayAgingMonitorResult(&checkInfo, msg.toLocal8Bit());

    return msg;
}


bool threadGeneralExamination::slotGetNativeNetworkInfo(QString& netName, QByteArray& macArray, QByteArray& ipArray){
    QList<QNetworkInterface> list = QNetworkInterface::allInterfaces();
    QNetworkInterface interface;
    QList<QNetworkAddressEntry> entryList;
    int i=0, j = 0;

    ipArray.remove(0, ipArray.length());

    for(i = 0; i<list.count(); i++)
    {
        interface = list.at(i);
        entryList= interface.addressEntries();

        if(strcmp(interface.name().toLatin1().data(), netName.toLocal8Bit().data())) continue;
        //qDebug() << "DevName: " << interface.name().toLatin1().data();
        //qDebug() << "Mac: " << interface.hardwareAddress();

        macArray.append(interface.hardwareAddress());

        //20180324log: There's error about it'll has a double scan for same name if you use the entryList.count(), What a fuck?
        if(entryList.isEmpty())
        {
            ipArray.append("isEmpty!");
            return false;
        }
        else
        {
            QNetworkAddressEntry entry = entryList.at(j);
            in_addr in;
            uint32_t ui32Ip = htonl(entry.ip().toIPv4Address());
            memcpy(&in, &ui32Ip, sizeof(ui32Ip));
            ipArray.append(inet_ntoa(in));
            //if there has some ip address
            if(entry.ip().toIPv4Address()){
                return true;
            }
            //qDebug() << "IP: " << QString::number(udpMsg.ui32IP, 10).toUpper();
            //qDebug() << "j"<<j<< "Netmask: " << entry.netmask().toString();
            //qDebug() << "j"<<j<< "Broadcast: " << entry.broadcast().toString();
        }
    }

    return false;
}

bool threadGeneralExamination::slotGetNativeNetworkInfo(char* deviceName, QByteArray& macArray, QByteArray& ipArray){
    QList<QNetworkInterface> list = QNetworkInterface::allInterfaces();
    QNetworkInterface interface;
    QList<QNetworkAddressEntry> entryList;
    int i=0, j = 0;

    for(i = 0; i<list.count(); i++)
    {
        interface = list.at(i);
        entryList= interface.addressEntries();

        if(strcmp(interface.name().toLatin1().data(), deviceName)) continue;
        //qDebug() << "DevName: " << interface.name();
        //qDebug() << "Mac: " << interface.hardwareAddress();

        macArray.append(interface.hardwareAddress());

        //20180324log: There's error about it'll has a double scan for same name if you use the entryList.count(), What a fuck?
        if(entryList.isEmpty())
        {
            ipArray.append("isEmpty!");
            return false;
        }
        else
        {
            QNetworkAddressEntry entry = entryList.at(j);
            in_addr in;
            uint32_t ui32Ip = htonl(entry.ip().toIPv4Address());
            memcpy(&in, &ui32Ip, sizeof(ui32Ip));
            ipArray.append(inet_ntoa(in));
            //if there has some ip address
            if(entry.ip().toIPv4Address()){
                //qDebug() << "ip:" << entry.ip().toIPv4Address();
                return true;
            }
            //qDebug() << "IP: " << QString::number(udpMsg.ui32IP, 10).toUpper();
            //qDebug() << "j"<<j<< "Netmask: " << entry.netmask().toString();
            //qDebug() << "j"<<j<< "Broadcast: " << entry.broadcast().toString();
        }
    }

    return false;
}


char* threadGeneralExamination::slotGetCurrentTime()
{
    static char buf[TIME_BUF_LENGTH] = {};

    QString qStr = QDateTime::currentDateTime().toString();

    bzero(buf, TIME_BUF_LENGTH);
    strncpy(buf, qStr.toStdString().c_str(), qStr.length());
    //qDebug() << "Get time: " << qStr.toStdString().c_str() << "len:" << qStr.length();
    if(strlen(buf) >= TIME_BUF_LENGTH)
        buf[sizeof(buf) - 1] = '\0';
    else
        buf[strlen(buf)] = '\0';

    return buf;
}

char threadGeneralExamination::slotCheckWifi(QString& logStr, QByteArray& macArray, QByteArray& ipArray)
{
    int tryCnt=0;
    char ret = NOTCHECK;

    system("killall -9 wlanMaster.sh");
    system("rm -rf /tmp/wlan*");
    system("chmod 777 /opt/wlanMaster.sh");
    system("/opt/wlanMaster.sh");
    //sleep(15);

    for(tryCnt=0; tryCnt<1; tryCnt++)
    {
        logStr = "WIFI STATUS:";
        QFile file;

        //5. ip
        //check wifi network
        QString netName(WIFI_NAME_V3BOX);
        bool wifiCheck = slotGetNativeNetworkInfo(netName, macArray, ipArray);
        logStr += "/ip(";
        logStr += QString(ipArray);
        logStr += ")";
        if(wifiCheck)
        {
            ret = WELLCHECK;
            break;
        }else
        {
            ret = FAILEDEVENT5;
        }

        //1. module
        file.setFileName("/tmp/wlan0/module");
        if(!file.open(QIODevice::ReadOnly))
        {
            qDebug("Error: wlan0 no module");
            logStr += "no module.";
            ret = FAILEDEVENT1;
        }else
        {
            QTextStream io(&file);
            QString str;
            while(!io.atEnd()) str = io.readLine();
            file.close();
            if(str.contains("OK"))
            {
                str += "module ok.";
            }else
            {
                ret = FAILEDEVENT1;
                str += "module failed.";
            }
        }


        //2. link
        file.setFileName("/tmp/wlan0/link");
        if(!file.open(QIODevice::ReadOnly))
        {
            qDebug("Error: wlan0 no link");
            logStr += "/no link";
            ret = FAILEDEVENT2;
        }else
        {
            QTextStream io(&file);
            QString str;
            while(!io.atEnd()) str = io.readLine();
            file.close();
            if(str.contains("OK"))
            {
                logStr += "/link ok";
            }else
            {
                ret = FAILEDEVENT2;
                logStr += "/link failed";
            }
        }

        //3. quality
        file.setFileName("/tmp/wlan0/linkinfo");
        if(!file.open(QIODevice::ReadOnly))
        {
            qDebug("Error: wlan0 no link quality.");
            logStr += "/no quality";
            ret = FAILEDEVENT3;
        }else
        {
            QTextStream io(&file);
            QString str;
            while(!io.atEnd()) str = io.readLine();
            file.close();
            QString quality = str.section('/', 0, 0, QString::SectionSkipEmpty);
            logStr += "/quality(";
            logStr += quality;
            logStr += ")";
            qDebug("quality:%s", quality.toLocal8Bit().data());
            if(quality.toInt() < (int)5)
            {
                ret = FAILEDEVENT3;
            }
        }

        //4. level
        file.setFileName("/tmp/wlan0/siginfo");
        if(!file.open(QIODevice::ReadOnly))
        {
            qDebug("Error: wlan0 no signal level!");
            logStr += "/no level";
            ret = FAILEDEVENT4;
        }else
        {
            QTextStream io(&file);
            QString level;
            while(!io.atEnd()) level = io.readLine();
            file.close();
            logStr += "/level(";
            logStr += level;
            logStr += ")";
            qDebug("signal level:%s", level.toLocal8Bit().data());
            if(level.toInt() < (int)-90)
            {
                ret = FAILEDEVENT4;
            }
        }
        if((WELLCHECK == ret)) break;
    }
    system("killall -9 wlanMaster.sh");

    return ret;
}

char threadGeneralExamination::slotCheckNtpAndSysDate()
{
    char ret = NOTCHECK;

    for(int i=0; i<2; i++)
    {
        system("rm -rf /tmp/ntpCheckTmp.txt");
        system("killall -9 ntpd");
        system("killall -9 ntpdate");
        //NTP 服务器, 最常见、熟知的就是 www.pool.ntp.org/zone/cn，国内地址为：cn.pool.ntp.org
        system("ntpdate -d cn.pool.ntp.org >> /tmp/ntpCheckTmp.txt ");

        QFile file("/tmp/ntpCheckTmp.txt");
        if(!file.open(QIODevice::ReadOnly|QIODevice::Text))
        {
            ret = FAILEDEVENT1;
            sleep(1);
            continue;
        }else
        {
            QTextStream io(&file);
            QString txt = io.readAll();
            file.close();

            if(txt.contains("adjust time") || txt.contains("synchronization"))
            {
                ret = WELLCHECK;
                break;
            }else if(txt.contains("Can't find"))
            {
                ret = FAILEDEVENT2;
            }else
            {
                ret = FAILEDEVENT3;
                continue;
            }
        }
    }
    system("rm -rf /tmp/ntpCheckTmp.txt");
    system("killall -9 ntpdate");

    //restart ntpd
    system("ntpd");

    return ret;
}

char threadGeneralExamination::slotCheckRtcAndSysDate()
{
    int ret = 0;

    ret = system("hwclock");
    qDebug() << "hwclock ret:" << ret;
    if(ret)
        return FAILEDCHECK;

    return WELLCHECK;
}

char threadGeneralExamination::slotUpdateSysOrRtcDate(results_info_t* checkInfo)
{
    char ret = NOTCHECK;

    if(WELLCHECK == checkInfo->ntpdate)
    {
        if(WELLCHECK == checkInfo->rtc)
        {
            //sysTime -> rtc
            ret = WELLCHECK;
            system("hwclock -w -u");
        }else
        {
            ret = FAILEDEVENT1;
        }
    }else
    {
        /* Because of poor network or somethig else reasons,
         * only one choice we have is to update date from rtc to system
         * in the base of rtc working well
        */
        if(WELLCHECK == checkInfo->rtc)
        {
            ret = FAILEDEVENT2;
        }else
        {
            ret = FAILEDEVENT3;
        }
    }

    return ret;
}

int threadGeneralExamination::slotGetUartCheckResult(char admin, char recvSend)
{
    QString logStr;


    switch(admin)
    {
    case 0x2:
    {
        switch(recvSend)
        {
        case 0x11:
            break;
        case 0x10:
            checkInfo.uart232 |= WELLEVENT1;
            logStr = "Success: uart232(recv works well).";
            qDebug() << logStr.toLocal8Bit().data();
            emit signalRecorder(LOG_CRIT, "", logStr);
            break;
        case 0x01:
            checkInfo.uart232 |= WELLEVENT2;
            logStr = "Success: uart232(send works well).";
            qDebug() << logStr.toLocal8Bit().data();
            emit signalRecorder(LOG_CRIT, "", logStr);
        case 0x00:
            break;
        default:
            break;
        }
        break;
    }
    default:
        break;
    }

    return 0;
}

char threadGeneralExamination::slotCheckSpi0Flash()
{
#ifndef LINUX_NODE_PROC_MTD
    return slotCheckSpi0Flashby_spidev();
#else
    return slotCheckSpi0Flashby_mtd();
#endif
}

char threadGeneralExamination::slotCheckSpi0Flashby_spidev()
{
    char ret = NOTCHECK;
    int fd = -1;
    uint8_t tx[2][4] = {{0x90, 0xff, 0xff, 0x00},{}};
    uint8_t rx[2][4] = {};
    struct spi_ioc_transfer trans;
    trans.tx_buf = (unsigned long)tx;
    trans.rx_buf = (unsigned long)rx;
    trans.len = 8;
    trans.delay_usecs = 0;
    trans.speed_hz = 152000;
    trans.bits_per_word = 8;


    fd = open(BOXV3_SPI_FLASH_DEVNODE, O_RDWR);
    if (fd < 0)
        return FAILEDEVENT1;

    //qDebug("Write standard spi cmd 0x90(read mf's id) to flash.\n");
    ret = ioctl(fd, SPI_IOC_MESSAGE(1), &trans);
    if(ret < 1) return FAILEDEVENT2;

    if((rx[1][0] == (uint8_t)(BOXV3_FLASH_MF_ID>>8)) && (rx[1][1] == (uint8_t)(BOXV3_FLASH_MF_ID&0xff)))
    {
        close(fd);
        return WELLCHECK;
    }

//    if(rx[1][0] || rx[1][1])
//    {
//    }
    qDebug("Flash MF's id : %#x, %#x not match with boxv3:%#x\n", rx[1][0], rx[1][1], BOXV3_FLASH_MF_ID);
    close(fd);
    return WELLEVENT1;
}

char threadGeneralExamination::slotCheckSpi0Flashby_mtd()
{
    char ret = NOTCHECK;

    QFile file(LINUX_NODE_PROC_MTD);
    if(!file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        ret = FAILEDEVENT1;
    }else
    {
        QTextStream io(&file);
        QString txt = io.readAll();
        file.close();

        if(txt.contains(BOXV3_MTD_FLASH_PART1_NAME) || txt.contains(BOXV3_MTD_FLASH_PART1_NAME1))
        {
            system("mount | grep '/mnt/mtdblock' > tmp_flash_test.txt");
            QFile tmp_file("tmp_flash_test.txt");
            if(!tmp_file.open(QIODevice::ReadOnly|QIODevice::Text))
            {
                ret = FAILEDEVENT3;
            }else
            {
                QTextStream tmp_io(&tmp_file);
                QString tmp_txt = tmp_io.readAll(); 
                tmp_file.close();

                if(tmp_txt.contains("/mnt/mtdblock"))
                {
                    ret = WELLCHECK;
                }else
                {
                    ret = FAILEDEVENT4;
                }
            }
            system("rm -rf tmp_flash_test.txt");
        }else
        {
            ret = FAILEDEVENT2;
        }
    }

    return ret;
}

char threadGeneralExamination::slotCheckSpi1Lora()
{
    int fd = -1, ret = 0;
    uint8_t tx[2] = {};
    uint8_t rx[2] = {};
    struct spi_ioc_transfer trans;

    trans.tx_buf = (unsigned long)tx,
    trans.rx_buf = (unsigned long)rx,
    trans.len = 2,
    trans.delay_usecs = 0,
    trans.speed_hz = 152000,
    trans.bits_per_word = 8,

    fd = open(BOXV3_SPI_LORA_DEVNODE, O_RDWR);
    if (fd < 0)
        return FAILEDEVENT1;

    //set
    bzero(tx, sizeof(tx));
    tx[0] = BOXV3_LORA_PHYADDR_FORTEST | 0x80;
    tx[1] = BOXV3_LORA_VALUE_FORTEST;
    ret = ioctl(fd, SPI_IOC_MESSAGE(1), &trans);
    if(ret < 1){
        close(fd);
        return FAILEDEVENT2;
    }
//    qDebug("before set: addr = 0x%08X, value = 0x%08X\n", BOXV3_LORA_PHYADDR_FORTEST, rx[1]);
//    qDebug("set       : addr = 0x%08X, value = 0x%08X\n", BOXV3_LORA_PHYADDR_FORTEST, tx[1]);
   //get
    bzero(tx, sizeof(tx));
    tx[0] = (unsigned char)BOXV3_LORA_PHYADDR_FORTEST & 0x7f;
    ret = ioctl(fd, SPI_IOC_MESSAGE(1), &trans);
    if(ret < 1){
        close(fd);
        return FAILEDEVENT3;
    }
//    qDebug("get: addr = 0x%08X, value = 0x%08X\n", BOXV3_LORA_PHYADDR_FORTEST, rx[1]);
    if((uint8_t)BOXV3_LORA_VALUE_FORTEST == rx[1]){
        close(fd);
        return WELLCHECK;
    }else{
        close(fd);
        return FAILEDEVENT4;
    }

    return WELLEVENT1;
}

void threadGeneralExamination::slotGetNetworkCheckInfo(QByteArray netCheckFlagArray)
{
    static bool emittedLan1 = false;
    static bool emittedLan2 = false;
    static bool emittedLan3 = false;
    static bool emittedLan4 = false;
    static bool emittedWan0 = false;

    QString logStr;
    netCheckFlag_t netCheckFlag;
    memcpy(&netCheckFlag, netCheckFlagArray.data(), netCheckFlagArray.size());

    //WanPort0 , the special port
    if((LINKMASK_LAN_PORTALL_CHECKED == netCheckFlag.checkedPort) && (!emittedWan0))
    {
        logStr = "WanPort0:";
        if(netCheckFlag.recvWorkWellPort & LINKMASK_WAN_PORT0)
        {
            logStr += "recv ok.";
            if(netCheckFlag.sendWorkWellPort & LINKMASK_WAN_PORT0)
            {
                checkInfo.networkWan = WELLCHECK;
                logStr += "send ok.";
            }else
            {
                checkInfo.networkWan = FAILEDEVENT2;
                logStr += "send failed.";
            }
        }else
        {
            logStr += "recv failed.";
            if(netCheckFlag.sendWorkWellPort & LINKMASK_WAN_PORT0)
            {
                checkInfo.networkWan = FAILEDEVENT1;
                logStr += "send ok.";
            }else
            {
                checkInfo.networkWan = FAILEDEVENT3;
                logStr += "send failed.";
            }
        }
        emittedWan0 = true;
        emit signalRecorder(LOG_CRIT, "", logStr);
    }

    //Lan Port1
    if((netCheckFlag.checkedPort & LINKMASK_LAN_PORT1) && (!emittedLan1))
    {
        logStr = "LanPort1:";
        if(netCheckFlag.recvWorkWellPort & LINKMASK_LAN_PORT1)
        {
            //qDebug("---------test---port1---%#x", checkInfo.networkLanPort1);
            logStr += "recv ok.";
            if(netCheckFlag.sendWorkWellPort & LINKMASK_LAN_PORT1)
            {
                checkInfo.networkLanPort1 = WELLCHECK;
                logStr += "send ok.";
            }else
            {
                checkInfo.networkLanPort1 = FAILEDEVENT2;
                logStr += "send failed.";
            }
        }else
        {
            logStr += "recv failed.";
            if(netCheckFlag.sendWorkWellPort & LINKMASK_LAN_PORT1)
            {
                checkInfo.networkLanPort1 = FAILEDEVENT1;
                logStr += "send ok.";
            }else
            {
                checkInfo.networkLanPort1 = FAILEDEVENT3;
                logStr += "send failed.";
            }
        }
        emittedLan1 = true;
        emit signalRecorder(LOG_CRIT, "", logStr);
    }

    //Lan port2
    if((netCheckFlag.checkedPort & LINKMASK_LAN_PORT2) && (!emittedLan2))
    {
        logStr = "LanPort2:";
        if(netCheckFlag.recvWorkWellPort & LINKMASK_LAN_PORT2)
        {
            logStr += "recv ok.";
            if(netCheckFlag.sendWorkWellPort & LINKMASK_LAN_PORT2)
            {
                checkInfo.networkLanPort2 = WELLCHECK;
                logStr += "send ok.";
            }else
            {
                checkInfo.networkLanPort2 = FAILEDEVENT2;
                logStr += "send failed.";
            }
        }else
        {
            logStr += "recv failed.";
            if(netCheckFlag.sendWorkWellPort & LINKMASK_LAN_PORT2)
            {
                checkInfo.networkLanPort2 = FAILEDEVENT1;
                logStr += "send ok.";
            }else
            {
                checkInfo.networkLanPort2 = FAILEDEVENT3;
                logStr += "send failed.";
            }
        }
        emittedLan2 = true;
        emit signalRecorder(LOG_CRIT, "", logStr);
    }
    //Lan port3
    if((netCheckFlag.checkedPort & LINKMASK_LAN_PORT3) && (!emittedLan3))
    {
        logStr = "LanPort3:";
        if(netCheckFlag.recvWorkWellPort & LINKMASK_LAN_PORT3)
        {
            logStr += "recv ok.";
            if(netCheckFlag.sendWorkWellPort & LINKMASK_LAN_PORT3)
            {
                checkInfo.networkLanPort3 = WELLCHECK;
                logStr += "send ok.";
            }else
            {
                checkInfo.networkLanPort3 = FAILEDEVENT2;
                logStr += "send failed.";
            }
        }else
        {
            logStr += "recv failed.";
            if(netCheckFlag.sendWorkWellPort & LINKMASK_LAN_PORT3)
            {
                checkInfo.networkLanPort3 = FAILEDEVENT1;
                logStr += "send ok.";
            }else
            {
                checkInfo.networkLanPort3 = FAILEDEVENT3;
                logStr += "send failed.";
            }
        }
        emittedLan3 = true;
        emit signalRecorder(LOG_CRIT, "", logStr);
    }
    //Lan port4
    if((netCheckFlag.checkedPort & LINKMASK_LAN_PORT4) && (!emittedLan4))
    {
        logStr = "LanPort4:";
        if(netCheckFlag.recvWorkWellPort & LINKMASK_LAN_PORT4)
        {
            logStr += "recv ok.";
            if(netCheckFlag.sendWorkWellPort & LINKMASK_LAN_PORT4)
            {
                checkInfo.networkLanPort4 = WELLCHECK;
                logStr += "send ok.";
            }else
            {
                checkInfo.networkLanPort4 = FAILEDEVENT2;
                logStr += "send failed.";
            }
        }else
        {
            logStr += "recv failed.";
            if(netCheckFlag.sendWorkWellPort & LINKMASK_LAN_PORT4)
            {
                checkInfo.networkLanPort4 = FAILEDEVENT1;
                logStr += "send ok.";
            }else
            {
                checkInfo.networkLanPort4 = FAILEDEVENT3;
                logStr += "send failed.";
            }
        }
        emittedLan4 = true;
        emit signalRecorder(LOG_CRIT, "", logStr);
    }
}

char threadGeneralExamination::slotCheck4G(QString& logStr)
{
    char ret = NOTCHECK;
    QFile file;

    //0. check ip
    file.setFileName(BOXV3_CHECK_4G_TMPFILE_IP);
    if(!file.open(QIODevice::ReadOnly))
    {
        qDebug("Error: Cant't open %s", file.fileName().toLocal8Bit().data());
        logStr = "Error: Cant't open";
        logStr += file.fileName();
        ret = FAILEDCHECK;
    }else
    {
        QTextStream io(&file);
        QString str = io.readAll();
        file.close();

        logStr = "4G IP(";
        logStr += str;
        logStr += ").";
        if(str.contains("."))
        {
            ret = WELLCHECK;
        }else
        {
            ret = FAILEDCHECK;
        }

    }

    //if not good
    if(WELLCHECK != ret)
    {
        //1. devicenode
        file.setFileName(BOXV3_CHECK_4G_TMPFILE_DEVICENODE);
        if(!file.open(QIODevice::ReadOnly))
        {
            qDebug("Error: Cant't open %s", file.fileName().toLocal8Bit().data());
            logStr = "Error: Cant't open";
            logStr += file.fileName();
            ret = FAILEDCHECK;
        }else
        {
            QTextStream io(&file);
            QString str = io.readAll();
            file.close();

            logStr += str;
            logStr += ".";
            if(!str.contains("/"))
            {
                ret = FAILEDEVENT1;
            }

        }
        //2. module
        file.setFileName(BOXV3_CHECK_4G_TMPFILE_MODULE);
        if(!file.open(QIODevice::ReadOnly))
        {
            qDebug("Error: Cant't open %s", file.fileName().toLocal8Bit().data());
            logStr = "Error: Cant't open";
            logStr += file.fileName();
            ret = FAILEDCHECK;
        }else
        {
            QTextStream io(&file);
            QString str = io.readAll();
            file.close();

            logStr += "Module(";
            logStr += str;
            logStr += ").";
            if(!str.contains("OK"))
            {
                ret = FAILEDEVENT2;
            }
        }

        //3. access
        file.setFileName(BOXV3_CHECK_4G_TMPFILE_ACCESS);
        if(!file.open(QIODevice::ReadOnly))
        {
            qDebug("Error: Cant't open %s", file.fileName().toLocal8Bit().data());
            logStr = "Error: Cant't open";
            logStr += file.fileName();
            ret = FAILEDCHECK;
        }else
        {
            QTextStream io(&file);
            QString str = io.readAll();
            file.close();

            logStr += "ACCESS(";
            logStr += str;
            logStr += ").";
            if(!str.contains("R"))
            {
                ret = FAILEDEVENT3;
            }
        }

        //4. dataService
        file.setFileName(BOXV3_CHECK_4G_TMPFILE_DATASERVICE);
        if(!file.open(QIODevice::ReadOnly))
        {
            qDebug("Error: Cant't open %s", file.fileName().toLocal8Bit().data());
            logStr = "Error: Cant't open";
            logStr += file.fileName();
            ret = FAILEDCHECK;
        }else
        {
            QTextStream io(&file);
            QString str = io.readAll();
            file.close();

            logStr += "DATASERVICE(";
            logStr += str;
            logStr += ").";
            if(str.contains("0"))
            {
                ret = FAILEDEVENT4;
            }
        }
        //5. SIMST
        file.setFileName(BOXV3_CHECK_4G_TMPFILE_SIMST);
        if(!file.open(QIODevice::ReadOnly))
        {
            qDebug("Error: Cant't open %s", file.fileName().toLocal8Bit().data());
            logStr = "Error: Cant't open";
            logStr += file.fileName();
            ret = FAILEDCHECK;
        }else
        {
            QTextStream io(&file);
            QString str = io.readAll();
            file.close();

            logStr += "SIMST(";
            logStr += str;
            logStr += ").";
            if(str.contains("255"))
            {
                ret = FAILEDEVENT5;
            }
        }

        //6. GET err MSG
        file.setFileName(BOXV3_CHECK_4G_TMPFILE_CME_ERROR);
        if(!file.open(QIODevice::ReadOnly))
        {
            qDebug("Error: Cant't open %s", file.fileName().toLocal8Bit().data());
            logStr = "Error: Cant't open";
            logStr += file.fileName();
        }else
        {
            QTextStream io(&file);
            QString str = io.readAll();
            file.close();

            logStr += "CME_ERR(";
            logStr += str;
            logStr += ").";
        }

    }

    logStr.remove('\n');

    return ret;
}

void threadGeneralExamination::slotKeyHasBeenPressed()
{
    static char pressedCnt = 0;
    if(haveBarCode) pressedCnt++;

#if 0
    //test
    haveBarCode = 1;
    qDebug("---test---");
    //test end
#endif

    if(1 == pressedCnt) this->start();
}

void threadGeneralExamination::slotDisplayBarCode(char barCodeStatus, QString str)
{
#if 1
    str += "";
#endif
    if((char)-1 == barCodeStatus) haveBarCode = 0;
    else haveBarCode++;

}

void threadGeneralExamination::slotStartProductionCheck()
{
    currentStatus = SELFCHECK_PRODUCTION;
    this->start();
}

void threadGeneralExamination::slotStartAfterAgingCheck()
{
    currentStatus = SELFCHECK_AGED;
    this->start();
}

void threadGeneralExamination::run()
{
    udpMsg_t udpMsg;
    QString logStr;
    bzero(&udpMsg, sizeof(udpMsg_t));
    qDebug() << "generalExaminationThread is running...\n\n";
    //-8. check time consumer sensors
    QObject::connect(&displayAndNetCheckTimer, SIGNAL(timeout()), this, SLOT(slotHandlerDisplayAndNetCheckTimer()), Qt::QueuedConnection);

    qDebug() << __func__<<  __LINE__ << endl;
    displayAndNetCheckTimer.setInterval(1000);
    QTimer::singleShot(0, &displayAndNetCheckTimer,SLOT(start())); //1000
    qDebug() << __func__<<  __LINE__ << endl;

#ifdef BOXV3_DEBUG_NET
    doHWfinalCheckTask(false);
    exec();
    return;
#endif

    //0. display notes and results
    switch(currentStatus)
    {
    case SELFCHECK_PRODUCTION:
    {
        //clear aging stage's logs
        clearAgingLog();
        //fdisk partition when it's production check stage
        emit signalRecorder(LOG_NOTICE, "pro1");
        sleep(1);
        fdiskForTheNewBox();
        break;
    }
    case SELFCHECK_AGED:
    {
        //display latest aging result
        QString uptimeMax = slotUpdateAgingCheckOut();
        //get aging time, write it to aged log
        logStr = "uptimeMax: ";
        logStr.append(uptimeMax);
        qDebug() << logStr.toLocal8Bit().data();
        emit signalRecorder(LOG_CRIT, "", logStr);
        emit signalRecorder(LOG_NOTICE, "aged1");
        break;
    }
    default:
        break;
    }


    //3. check rtc
    checkInfo.rtc = slotCheckRtcAndSysDate();
    if(WELLCHECK == checkInfo.rtc)
    {
        if(checkInfo.ntpdate)
        {
            ;//compare sysTime with RTC, and update one of them
        }
        logStr = "Success: RTC works well~";
        qDebug() << logStr.data();
        emit signalRecorder(LOG_CRIT, "", logStr);
    }else
    {
        logStr = "Error  : RTC check failed! hwclock failed!";
        qDebug() << logStr.data();
        emit signalRecorder(LOG_CRIT, "", logStr);
    }


    //5. check uart 232
    objectUart uart232((char*)FILE_PATH_UART232);
    connect(&uart232, SIGNAL(signalSendUartCheckResult(char, char)), this, SLOT(slotGetUartCheckResult(char, char)), Qt::DirectConnection);
    connect(this, SIGNAL(signalSelfTransceiverSelf()), &uart232, SLOT(slotSelfTransceiverSelf()));
    QThread threadUart232Check;
    uart232.moveToThread(&threadUart232Check);
    threadUart232Check.start();
    emit signalSelfTransceiverSelf();


    //6. check spi-flash
    switch(slotCheckSpi0Flash())
    {
    case FAILEDEVENT1:
        checkInfo.spi0Flash = FAILEDCHECK;
#ifndef LINUX_NODE_PROC_MTD
        logStr = "Error  : spi-flash. Can not open ";
        logStr.append(BOXV3_SPI_FLASH_DEVNODE);
#else
        logStr = "Error  : file /proc/mtd not found!";
#endif
        qDebug() << logStr.data();
        emit signalRecorder(LOG_CRIT, "", logStr);
        break;
    case FAILEDEVENT2:
        checkInfo.spi0Flash = FAILEDCHECK;

#ifndef LINUX_NODE_PROC_MTD
        logStr = "Error  : spi-flash. Ioctl() ";
        logStr.append(BOXV3_SPI_FLASH_DEVNODE);
        logStr.append(" to get flash manufacture id failed!");
#else
        logStr = "Error  : Can't found any flash partition's info.";
#endif
        qDebug() << logStr.data();
        emit signalRecorder(LOG_CRIT, "", logStr);
        break;

#ifdef LINUX_NODE_PROC_MTD
    case FAILEDEVENT3:
        checkInfo.spi0Flash = FAILEDCHECK;
        logStr = "Error  : Can't get tmpfile for mount info.";
        qDebug() << logStr.data();
        emit signalRecorder(LOG_CRIT, "", logStr);
        break;
    case FAILEDEVENT4:
        checkInfo.spi0Flash = FAILEDCHECK;
        logStr = "Error  : Can't found flash mount info.";
        qDebug() << logStr.data();
        emit signalRecorder(LOG_CRIT, "", logStr);
        break;
#endif
    case WELLCHECK:
        checkInfo.spi0Flash = WELLCHECK;
#ifndef LINUX_NODE_PROC_MTD
        logStr = "Success: Spi-Flash works well. Have been gotten manufacture id~";
#else
        logStr = "Success: spi-flash(mtd) woeks well.";
#endif
        qDebug() << logStr.data();
        emit signalRecorder(LOG_CRIT, "", logStr);
        break;
    case WELLEVENT1:
        checkInfo.spi0Flash = WELLCHECK;
        logStr = "Success: Spi-Flash. But the Flash MF id does not match with boxv3 ";
        logStr.append((char*)BOXV3_FLASH_MF_ID);
        qDebug() << logStr.data();
        emit signalRecorder(LOG_CRIT, "", logStr);
        break;
    default:
        checkInfo.spi0Flash = FAILEDCHECK;
        logStr = "Warning: spi-flash. Unknown flash option.\n";
        qDebug() << logStr.data();
        emit signalRecorder(LOG_CRIT, "", logStr);
        break;
    }

    //7. check spi-lora
    switch(slotCheckSpi1Lora())
    {
    case FAILEDEVENT1:
        checkInfo.spi1Lora = FAILEDCHECK;
        logStr = "Error  :  Spi-Lora check. Can not open device ";
        logStr.append(BOXV3_SPI_LORA_DEVNODE);
        qDebug() << logStr.data();
        emit signalRecorder(LOG_CRIT, "", logStr);
        break;
    case FAILEDEVENT2:
    case FAILEDEVENT3:
        checkInfo.spi1Lora = FAILEDCHECK;
        logStr = "Error  :  Spi-Lora check. Can not ioctl spi lora.";
        qDebug() << logStr.data();
        emit signalRecorder(LOG_CRIT, "", logStr);
        break;
    case FAILEDEVENT4:
        checkInfo.spi1Lora = FAILEDCHECK;
        logStr = "Error  :  Spi-Lora check. Read spi lora data failed.";
        qDebug() << logStr.data();
        emit signalRecorder(LOG_CRIT, "", logStr);
        break;
    case WELLCHECK:
        checkInfo.spi1Lora = WELLCHECK;
        logStr = "Success: Spi-Lora works well~";
        qDebug() << logStr.data();
        emit signalRecorder(LOG_CRIT, "", logStr);
        break;
    default:
        checkInfo.spi1Lora = FAILEDCHECK;
        logStr = "Warning: Spi-lora. Unknown lora option.";
        qDebug() << logStr.data();
        emit signalRecorder(LOG_CRIT, "", logStr);
        break;
    }

    //8. check iic-at88sc104
    if(1)
    {
        QMutexLocker lockerAt88(&mutexAT88);
        objectAT88SC104C at88sc104c((char*)AT88SC104_NODE_NAME);
        //check mf and lot info
        if(WELLCHECK == at88sc104c.getMfAndLotInfo()){
            checkInfo.iicAt88sc104c = WELLCHECK;
            logStr = "Success: AT88SC104C. Have been got MfandLot info.";
            qDebug() << logStr.data();
            emit signalRecorder(LOG_CRIT, "", logStr);
#if 0
            //check passwd
            if(WELLCHECK == at88sc104c.verfyPasswd()){
                checkInfo.iicAt88sc104c = WELLCHECK;
                logStr = "Success: AT88SC104C. Passwd verified ok.";
                qDebug() << logStr.data();
                emit signalRecorder(LOG_CRIT, "", logStr);
            }else
            {
                checkInfo.iicAt88sc104c = FAILEDCHECK;
                logStr = "Error  : at88sc104c check. Wrong passwd.";
                qDebug() << logStr.data();
                emit signalRecorder(LOG_CRIT, "", logStr);
            }
#endif
        }else
        {
            checkInfo.iicAt88sc104c = FAILEDCHECK;
            logStr = "Error  : at88sc104c check. Not yet got MfAntLot info.";
            qDebug() << logStr.data();
            emit signalRecorder(LOG_CRIT, "", logStr);
        }
    }

    //9. check stm8
    objectUart uartStm8((char*)FILE_PATH_UARTSTM8);
    if(0 == uartStm8.checkUartStm8()){
        checkInfo.uart2Stm8 = WELLCHECK;
        logStr = "Success: stm8 works well. Uart2 port communicate with stm8 ok.";
        qDebug() << logStr.data();
        emit signalRecorder(LOG_CRIT, "", logStr);
    }else
    {
        qDebug() << "before displayAndNetCheckTimer stoped." <<endl;
        sleep(1);
        qDebug() << __func__<<  __LINE__ << endl;
        QTimer::singleShot(0, &displayAndNetCheckTimer,SLOT(stop()));
        qDebug() << __func__<<  __LINE__ << endl;
        qDebug() << "after displayAndNetCheckTimer stoped." <<endl;
        //stm8 version not matching, prepare fuse new hw soft to stm8, and the check again
        emit signalRecorder(LOG_NOTICE, "stm8opt0");
        sleep(3);
        QString firmwareFile = getFirmwareName();
        //qDebug("---Debug: firmwarepathname:%s", firmwareFile.toLocal8Bit().data());
        if(firmwareFile.size() > 1){
            for(int i=0; i<3; i++)
            {
                if(0!=i)
                    emit signalRecorder(LOG_CRIT, "", "try update stm8 software again...");

                QByteArray cmdArray(BOXV3_STM8BOOTAPP_FILEPATH);
                cmdArray.append(" ");
                cmdArray.append(firmwareFile.toLocal8Bit());
                cmdArray.append(" &");
                system(cmdArray.data());
                //check it again
                sleep(7);
                system("killall -9 stm8boot");
                if(0 == uartStm8.checkUartStm8())
                {
                    emit signalRecorder(LOG_NOTICE, "stm8opt1");
                    logStr = "Fused: new stm8 software has been fused.";
                    qDebug() << logStr.data();
                    emit signalRecorder(LOG_CRIT, "", logStr);
                    checkInfo.uart2Stm8 = WELLCHECK;
                    logStr = "Success: stm8 works well. Uart2 port communicate with stm8 ok.";
                    qDebug() << logStr.data();
                    emit signalRecorder(LOG_CRIT, "", logStr);
                    break;
                }else
                {
                    emit signalRecorder(LOG_CRIT, "stm8opt3");
                    checkInfo.uart2Stm8 = FAILEDCHECK;
                    logStr = "Error  : stm8. Uart2 port can not communicate with stm8.";
                    qDebug() << logStr.data();
                    emit signalRecorder(LOG_CRIT, "", logStr);
                }
            }
        }else
        {
            checkInfo.uart2Stm8 = FAILEDCHECK;
            logStr = "Error  : no new stm8 firmware in the firmware dir.";
            qDebug() << logStr.data();
            emit signalRecorder(LOG_CRIT, "", logStr);
            emit signalRecorder(LOG_CRIT, "stm8opt2");
        }
        qDebug() << __func__<<  __LINE__ << endl;
        displayAndNetCheckTimer.setInterval(1000);
        QTimer::singleShot(0, &displayAndNetCheckTimer,SLOT(start())); //1000
        qDebug() << __func__<<  __LINE__ << endl;
    }


    //10. net check(WAN and LAN) thread wait for check
    QThread netCheckThread;
    uartStm8.moveToThread(&netCheckThread);
    QObject::connect(&uartStm8, SIGNAL(signalSendNetworkCheckInfo(QByteArray)), this, SLOT(slotGetNetworkCheckInfo(QByteArray)), Qt::DirectConnection);
    QObject::connect(this, SIGNAL(signalReadStm8Uart2ForNetStatus()), &uartStm8, SLOT(slotLanPortCtrlAndCheck()));
    netCheckThread.start();

    qDebug() << "generalExaminationThread exec()...\n";
    exec();
}

