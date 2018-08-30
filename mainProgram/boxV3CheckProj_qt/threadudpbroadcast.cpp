#include "threadudpbroadcast.h"

threadUdpBroadcast::threadUdpBroadcast()
{
    startedUdpFlag = false;
    //1. New a udp socket
    udpsocketRecv = new QUdpSocket;
    udpsocketSend = new QUdpSocket;
    udpsocketMainRecv = new QUdpSocket;
    udpsocketMainSend = new QUdpSocket;
    bzero(&udpMsg, sizeof(udpMsg_t));

    moveToThread(this);
}

threadUdpBroadcast::~threadUdpBroadcast()
{
    delete udpsocketRecv;
    delete udpsocketSend;
    delete udpsocketMainRecv;
    delete udpsocketMainSend;
}


void threadUdpBroadcast::slotStartUdpTransceiver(void)
{

    qDebug("---debug---%s(line:%d.)\n", __func__, __LINE__);

    if(false == startedUdpFlag)
    {
        qDebug("---debug---%s(line:%d.)\n", __func__, __LINE__);
        startedUdpFlag = true;
        //1min
        qDebug() << __func__<<  __LINE__ << endl;
        udpTimeKeeper.setInterval(10000);
        QTimer::singleShot(0, &udpTimeKeeper,SLOT(start())); //10000
        qDebug() << __func__<<  __LINE__ << endl;
    }else
    {
        qDebug() << "Udp transceiver has been started! Do not try again!";
        return;
    }
    //make sure wifi works well
   //slotEnableWifi();

    //get wifi ip for udpsocketSend->bind
    QString wifiNetName("wlan0");
    QString wifiIp;
    if(getIfconfigIp(wifiNetName, wifiIp) != 0)
    {
        qDebug() << "Error: slotStartUdpTransceiver failed: wifi has no ip.";
        emit signalRecorder(LOG_CRIT, "", "Error: slotStartUdpTransceiver failed: wifi has no ip.");
        return;
    }else
    {
        ;
        //emit signalRecorder(LOG_CRIT, "", "wifiIp:");
        //emit signalRecorder(LOG_CRIT, "", wifiIp.toLocal8Bit().data());
    }
   //2. bind udp client's socket to the server
   udpsocketRecv->bind(UDP_RECV_SRC_PORT_NUMBER, QUdpSocket::ShareAddress);
   udpsocketSend->bind(QHostAddress(QString(wifiIp)), UDP_SEND_SRC_PORT_NUMBER, QUdpSocket::ShareAddress|QUdpSocket::ReuseAddressHint);

   //2.1 10106
   udpsocketMainRecv->bind(UDP_MAIN_PORT_NUMBER, QUdpSocket::ShareAddress);
   udpsocketMainSend->bind(QHostAddress(QString(wifiIp)), UDP_MAIN_PORT_NUMBER, QUdpSocket::ShareAddress|QUdpSocket::ReuseAddressHint);

   //3. prepare to receive data from the server
   connect(udpsocketRecv, SIGNAL(readyRead()), this, SLOT(slotProcessPendingDatagrams()), Qt::QueuedConnection);
   //3.1 10106 recv
   connect(udpsocketMainRecv, SIGNAL(readyRead()), this, SLOT(slotMainProcessPendingDatagrams()), Qt::QueuedConnection);
}


void threadUdpBroadcast::slotStopUdpTransceiver()
{
#ifndef BOXV3_DEBUG_NET
    qDebug() << __func__<<  __LINE__ << endl;
    QTimer::singleShot(0, &udpTimeKeeper,SLOT(stop()));
    qDebug() << __func__<<  __LINE__ << endl;
    qDebug() << "Warning: udp service has been closed by tcp service.";
    udpsocketRecv->close();
    udpsocketSend->close();
    udpsocketMainRecv->close();
    udpsocketMainSend->close();
    //this->quit();
#endif
}


void threadUdpBroadcast::slotProcessPendingDatagrams()
{
    static char cnt = 0;
    udpMsg_t tmpMsg;
    do{
        /*readDatagram() interface will clean the first argument's space, so we should get wifi info again*/
        udpsocketRecv->readDatagram((char*)&tmpMsg, sizeof(udpMsg_t), &currentUdpServerHost, &currentUdpServerPort);
        if(udpServerHost.isNull())
        {
            udpServerHost = currentUdpServerHost;
            udpServerPort = currentUdpServerPort;
        }else
        {
            //just allow this app communicates with one udp server
            if(udpServerHost != currentUdpServerHost)
            {
                qDebug() << "Warning: Sorry, this burnCheck app already has connect a udp server! Do not connect again!";
                break;
            }else
            {
                udpServerHost = currentUdpServerHost;
                udpServerPort = currentUdpServerPort;
            }
        }

        qDebug() << "udp readDatagram from Host(" << QHostAddress(udpServerHost) << ") port:" << udpServerPort << "success";
        qDebug() << "udp readDatagram from current Host(" << QHostAddress(currentUdpServerHost) << ") port:" << currentUdpServerPort << "success";
        /*copy important msg from udp server*/
        cpImportantMsgFromServer(&udpMsg, &tmpMsg);
        /*fill wifi info*/
        fillUdpDataPackage();
        sendUdpDateToServer();

        cnt++;
        if(1 == cnt) emit signalRecorder(LOG_NOTICE, "udp0");

    }while(udpsocketRecv->hasPendingDatagrams());
}


void threadUdpBroadcast::slotMainProcessPendingDatagrams()
{
    static char cnt = 0;
    udpMsg_t tmpMsg;
    do{
        /*readDatagram() interface will clean the first argument's space, so we should get wifi info again*/
        udpsocketMainRecv->readDatagram((char*)&tmpMsg, sizeof(udpMsg_t), &currentUdpServerHost, &currentUdpServerPort);

        qDebug() << "Main: udp readDatagram from Host(" << QHostAddress(currentUdpServerHost) << ") port:" << currentUdpServerPort << "success";
        /*copy important msg from udp server*/
        cpImportantMsgFromServer(&udpMsg, &tmpMsg);
        /*fill wifi info*/
        fillUdpDataPackage();
        sendUdpDateToServer();

        cnt++;
        if(1 == cnt) emit signalRecorder(LOG_NOTICE, "udp0");

    }while(udpsocketMainRecv->hasPendingDatagrams());
}


int threadUdpBroadcast::slotEnableWifi(void)
{
    int ret = 0;

    ret = system("ifconfig wlan0 up");
    if(ret)return ret;
    system("killall -9 wpa_supplicant");
    ret = system("wpa_supplicant -i wlan0 -c /etc/wpa_supplicant.conf &");
    if(ret)return ret;
    ret = system("udhcpc -i wlan0");

    return ret;
}

void threadUdpBroadcast::slotUdpTimeout()
{
    emit signalRecorder(LOG_CRIT, "error udpTimeout");
    /*fill wifi info*/
    fillUdpDataPackage();
    sendMainUdpDateToServer();
}

void threadUdpBroadcast::getNativeNetworkInfo()
{
    QList<QNetworkInterface> list = QNetworkInterface::allInterfaces();
    QNetworkInterface interface;
    QList<QNetworkAddressEntry> entryList;
    int i=0, j = 0;

    for(i = 0; i<list.count(); i++)
    {
        interface = list.at(i);
        entryList= interface.addressEntries();

#ifdef BOXV3_DEBUG_DISPLAY
        qDebug("(%s,line%d)i cnt: %d", __func__, __LINE__, i);
#endif
        if(strcmp(interface.name().toLatin1().data(), WIFI_NAME_V3BOX)) continue;
        //qDebug() << "DevName: " << interface.name();
        //qDebug() << "Mac: " << interface.hardwareAddress();
        //strcpy((char*)(udp_msg.ui8Mac), interface.hardwareAddress().toLocal8Bit().data());
        changeMacToNum((char*)(udpMsg.ui8Mac), interface.hardwareAddress().toLocal8Bit().data());
        //20180324log: There's error about it'll has a double scan for same name if you use the entryList.count(), What a fuck?
        if(entryList.isEmpty())
        {
            qDebug() << "Error: wifi doesn't get a ip.";
            break;
        }else
        {
            QNetworkAddressEntry entry = entryList.at(j);
            udpMsg.ui32IP = htonl(entry.ip().toIPv4Address());
            qDebug() << "udpMsg.ui32IP -> ip: " << QString::number(udpMsg.ui32IP, 10).toUpper();
            //qDebug() << "j"<<j<< "Netmask: " << entry.netmask().toString();
            //qDebug() << "j"<<j<< "Broadcast: " << entry.broadcast().toString();
        }
    }
}


void threadUdpBroadcast::changeMacToNum(char *dst, char *src)
{
    bzero(dst, BOXV3_UI8MAC_LENGTH);
    char *p = strtok(src, ":");
    int i = 0;

    //qDebug() << "mac: ";
    while (NULL != p)
    {
        dst[i++] = strtoul(p, NULL, 16);
        //qDebug("mac[%d]: %#x", i-1, dst[i-1]);
        p = strtok(NULL, ":");
    }
    //qDebug() << "end";
}

void threadUdpBroadcast::cpImportantMsgFromServer(udpMsg_t *dst, udpMsg_t *src)
{
    if(0 == src->ui32SourceIP)
        dst->ui32SourceIP = htonl(QHostAddress(udpServerHost).toIPv4Address());
    else
        dst->ui32SourceIP = src->ui32SourceIP;
}

void threadUdpBroadcast::sendUdpDateToServer()
{
    for(int i=0; i<UDP_SEND_CNT_MAX; i++)
    {
        udpsocketSend->writeDatagram((char*)&udpMsg, sizeof(udpMsg_t), QHostAddress::Broadcast, udpServerPort);
        qDebug() << "udp write to server done!" ;
    }
}

void threadUdpBroadcast::sendMainUdpDateToServer()
{
    for(int i=0; i<UDP_SEND_CNT_MAX; i++)
    {
        udpsocketMainSend->writeDatagram((char*)&udpMsg, sizeof(udpMsg_t), QHostAddress::Broadcast, UDP_MAIN_PORT_NUMBER);
        qDebug() << "udp write to server done!" ;
    }
}

void threadUdpBroadcast::getSelfSign(unsigned short* sign)
{
    QMutexLocker lockerAt88(&mutexAT88);
    objectAT88SC104C at88((char*)AT88SC104_NODE_NAME);
    switch(at88.getCheckStage())
    {
    case SELFCHECK_PRODUCTION:
    {
        *sign = 0xb0;
        break;
    }
    default:
    {
        *sign = 0xb1;
        break;
    }
    }
}

void threadUdpBroadcast::fillUdpDataPackage()
{    /* Notes:
     *  There's just a part of msg of the udp data package,
     *  but don't worry about it. Some parts of the pacakage
     *  will be filled in anther place, like getNativeNetworkInfo()
    */
    udpMsg.ui32MsgLength = sizeof(udpMsg_t);

    udpMsg.ui32Prefix = 0x0FF055AA;
    udpMsg.ui32Postfix= 0x550FF0AA;
    udpMsg.ui32Version = 0x00010000;
    udpMsg.ui32MagicNum = 0x12345678;
    udpMsg.ui16MainCode = 0;
    udpMsg.ui32FBCode = 0x80000000;

    /*getself sign*/
    getSelfSign(&udpMsg.ui16SubCode);

    //fill wifi mac and ip
    getNativeNetworkInfo();
    //get device code from usr_zone0 bit[0:15] of at88sc104c
    if(1)
    {
        QMutexLocker lockerAt88(&mutexAT88);
        objectAT88SC104C at88sc104((char*)AT88SC104_NODE_NAME);
        bzero(udpMsg.i8DeviceCode, sizeof(udpMsg.i8DeviceCode));
        at88sc104.getDeviceCode(udpMsg.i8DeviceCode);
    }
#if 0
    qDebug("\nudp getDeviceCode:");
    QByteArray tmpArray;
    tmpArray.append(udpMsg.i8DeviceCode);
    qDebug() << tmpArray.data();
#endif

    QByteArray svrName("BOX-V3");
    memcpy(udpMsg.i8SvrName, svrName.data(), svrName.size());
    QByteArray domain("ASP");
    memcpy(udpMsg.i8Domain, domain.data(), domain.size());

}


void threadUdpBroadcast::run()
{
    QObject::connect(&udpTimeKeeper, SIGNAL(timeout()), this, SLOT(slotUdpTimeout()), Qt::QueuedConnection);
    exec();
}

