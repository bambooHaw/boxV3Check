#include "threadtcpserver.h"

threadTcpServer::threadTcpServer()
{
    startedFlag = false;
    tcpServer = new QTcpServer;

    moveToThread(this);
}

threadTcpServer::~threadTcpServer()
{
    delete tcpServer;
    qDebug() << "~tcpServerThread()l";
}

QByteArray threadTcpServer::changeCharMacToStdMac(char *mac)
{
    if(!mac)
    {
        qDebug() << "Error: mac addr is empty! ";
        return QByteArray("");
    }
    QByteArray macArray;
    macArray.resize(17);

    macArray[0] = (mac[0] & 0xf0)>>8;
    macArray[1] = (mac[0] & 0x0f);
    macArray[2] = ':';
    macArray[3] = (mac[1] & 0xf0)>>8;
    macArray[4] = (mac[1] & 0x0f);
    macArray[5] = ':';
    macArray[6] = (mac[2] & 0xf0)>>8;
    macArray[7] = (mac[2] & 0x0f);
    macArray[8] = ':';
    macArray[9] = (mac[3] & 0xf0)>>8;
    macArray[10] = (mac[3] & 0x0f);
    macArray[11] = ':';
    macArray[12] = (mac[4] & 0xf0)>>8;
    macArray[13] = (mac[4] & 0x0f);
    macArray[14] = ':';
    macArray[15] = (mac[5] & 0xf0)>>8;
    macArray[16] = (mac[5] & 0x0f);
    return macArray;
}

void threadTcpServer::setLanMac(char* mac)
{
    if(!mac)
    {
        qDebug() << "Error: lan mac addr is empty! ";
        return;
    }

    QByteArray macArray = changeCharMacToStdMac(mac);
    QByteArray cmdArray("ifconfig eth1 hw ether ");
    cmdArray.append(macArray.data());
    //qDebug("---Debug: %s", cmdArray.data());
    system(cmdArray.data());
}

void threadTcpServer::setWanMac(char* mac)
{
    if(!mac)
    {
        qDebug() << "Error: wan mac addr is empty! ";
        return;
    }

    QByteArray macArray = changeCharMacToStdMac(mac);
    QByteArray cmdArray("ifconfig eth0 hw ether ");
    cmdArray.append(macArray.data());
    //qDebug("---Debug: %s", cmdArray.data());
    system(cmdArray.data());
}


int threadTcpServer::slotEnableWifi(void)
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

void threadTcpServer::showBuf(char *buf, int len)
{
    int i;

    for(i=0; i<len; i++){
        if(0== i%10)
            if(0!=i)
                puts("");
        printf("0x%02x ", buf[i]);
    }
    puts("");
}

void threadTcpServer::showTcpMsg(void *packet)
{
    tcpMsgWithoutData_t* msg = (tcpMsgWithoutData_t*)packet;
    qDebug("------------------------------------");
    qDebug("chHeader:");
    showBuf((char*)(msg->chHeader), 3);
    qDebug("chVersion: %#x", msg->chVersion);
    qDebug("ui16Length: %#x", msg->ui16Length);
    qDebug("ui16SerialNO: %#x", msg->ui16SerialNO);
    qDebug("ui16FeekCode: %#x", msg->ui16FeekCode);
    qDebug("ucCmdType: %#x", msg->ucCmdType);
    qDebug("ucMainCmd: %#x", msg->ucMainCmd);
    qDebug("ucSubCmd: %#x", msg->ucSubCmd);
    if(TCP_MSG_WITHOUT_DATA == msg->ui16Length)
    {
        qDebug("verifyCode: %#x", msg->verifyCode);
        qDebug("ui16tail: %#x", msg->ui16tail);
    }
    else
    {
        tcpMsgHaveData_t* p = (tcpMsgHaveData_t*)packet;
        unsigned short dataLen = p->ui16Length - TCP_MSG_WITHOUT_DATA;
        qDebug("pData:");
        showBuf((char*)(p->pData), dataLen);
        qDebug("verifycode: %#x", *(unsigned char*)(p->pData + dataLen));
        //qDebug("ui16tail: %#x", *(unsigned short*)(p->pData + dataLen + 1));
    }
    qDebug("------------------------------------");
}

void threadTcpServer::showTcpLogMsg(void *packet)
{
    tcpMsgHaveData_t* msg = (tcpMsgHaveData_t*)packet;
    if(msg->ui16Length <= TCP_MSG_WITHOUT_DATA)
    {
        qDebug("Error: Wrong tcp Log Msg.");
        return;
    }
    qDebug("------------------------------------");

    showBuf((char*)(msg->chHeader), msg->ui16Length);
#if 0
    qDebug("chHeader:");
    showBuf((char*)(msg->chHeader), 3);
    qDebug("chVersion: %#x", msg->chVersion);
    qDebug("ui16Length: %#x", msg->ui16Length);
    qDebug("ui16SerialNO: %#x", msg->ui16SerialNO);
    qDebug("ui16FeekCode: %#x", msg->ui16FeekCode);
    qDebug("ucCmdType: %#x", msg->ucCmdType);
    qDebug("ucMainCmd: %#x", msg->ucMainCmd);
    qDebug("ucSubCmd: %#x", msg->ucSubCmd);

    qDebug("log data( %ldbytes):", msg->dataLen);
    showBuf((char*)(msg->pData + 4), *(unsigned int*)(msg->pData));
    qDebug("verifycode: %#x", *(unsigned char*)(msg->pData + 4 + msg->dataLen));
    qDebug("ui16tail: %#x", *(unsigned short*)(msg->pData + 4 + msg->dataLen + 1));
#endif
    qDebug("------------------------------------");

}

/*
 * return val: data buf length
*/
char threadTcpServer::parseHeaderfindLength(tcpMsgGetLen_t* hvl)
{

    bool msgFlag = false;
    if(!hvl) return -1;
    else bzero(hvl, sizeof(tcpMsgGetLen_t));

    //1. parsed user-defined packet's Header
    for(int i=0; i<TCP_FINDHEAD_MAXCNT; i++)
    {
        clientCon->read((char*)(hvl->chHeader), 3);
        if((hvl->chHeader[0] == 0xAA) && (hvl->chHeader[1] == 0x55) && (hvl->chHeader[2] == 0xFF))
        {
            msgFlag = true;
            break;
        }
        else continue;
    }
    if(!msgFlag) return -2;

    clientCon->read((char*)(&(hvl->chVersion)), 1);
    clientCon->read((char*)(&(hvl->ui16Length)), 2);

    return 0;
}

void threadTcpServer::fillAppointedData(void* recvMsg, void *p, char* data, unsigned long dataLen, unsigned short feedBck)
{
    tcpMsgWithoutData_t* pac = (tcpMsgWithoutData_t*)p;

    pac->chHeader[0] = 0xAA;
    pac->chHeader[1] = 0x55;
    pac->chHeader[2] = 0xFF;
    pac->chVersion = 0;
    pac->ui16Length = TCP_MSG_WITHOUT_DATA + dataLen;
    pac->ui16SerialNO = ((tcpMsgWithoutData_t*)recvMsg)->ui16SerialNO;

    if(0 == feedBck) ; //Do not change
    else pac->ui16FeekCode = feedBck;

    pac->ucCmdType = ((tcpMsgWithoutData_t*)recvMsg)->ucCmdType;
    pac->ucMainCmd = ((tcpMsgWithoutData_t*)recvMsg)->ucMainCmd;
    pac->ucSubCmd = ((tcpMsgWithoutData_t*)recvMsg)->ucSubCmd;

    if(0 == dataLen)
    {
        // pac->ui16tail = htonl(0x55aa);
        *(unsigned char*)(&pac->ui16tail) = 0x55;
        *((unsigned char*)(&pac->ui16tail) + 1) = 0xaa;
        pac->verifyCode = calcVerifyCode(pac);
    }
    else
    {
        tcpMsgHaveData_t* packet = (tcpMsgHaveData_t*)p;
        memcpy(packet->pData, data, dataLen);
        //*(unsigned short*)(packet->pData + dataLen + 1) = htonl(0x55aa);
        *(unsigned char*)(packet->pData + dataLen + 1) = 0x55;   //ui16tail
        *(unsigned char*)(packet->pData + dataLen + 2) = 0xaa;   //ui16tail
        *(unsigned char*)(packet->pData + dataLen) = calcVerifyCode(pac);
    }
}

void threadTcpServer::fillAppointedDataForLog(void* recvMsg, void *p, char *data, unsigned int dataLen, unsigned short feedBck)
{
    tcpMsgHaveData_t* pac = (tcpMsgHaveData_t*)p;

    pac->chHeader[0] = 0xAA;
    pac->chHeader[1] = 0x55;
    pac->chHeader[2] = 0xFF;
    pac->chVersion = 0;
    pac->ui16Length = TCP_MSG_WITHOUT_DATA + 4 + dataLen;
    pac->ui16SerialNO = ((tcpMsgWithoutData_t*)recvMsg)->ui16SerialNO;

    if(0 == feedBck) ; //Do not change
    else pac->ui16FeekCode = feedBck;

    pac->ucCmdType = ((tcpMsgWithoutData_t*)recvMsg)->ucCmdType;
    pac->ucMainCmd = ((tcpMsgWithoutData_t*)recvMsg)->ucMainCmd;
    pac->ucSubCmd = ((tcpMsgWithoutData_t*)recvMsg)->ucSubCmd;

    if(0 == dataLen) return;
    *(unsigned char*)(pac->pData) = (char)dataLen;
    memcpy(pac->pData + 4, data, dataLen);
    //*(unsigned short*)(packet->pData + dataLen + 1) = htonl(0x55aa);
    *(unsigned char*)(pac->pData + 4 + dataLen + 1) = 0x55;   //ui16tail
    *(unsigned char*)(pac->pData + 4 + dataLen + 2) = 0xaa;   //ui16tail
    *(unsigned char*)(pac->pData + 4 + dataLen) = calcVerifyCode(pac);

}

unsigned char threadTcpServer::calcVerifyCode(void *packet)
{
    unsigned char verifyCode = 0;
    unsigned long dataLen = 0;
    tcpMsgWithoutData_t* p = (tcpMsgWithoutData_t*)packet;
    if(p->ui16Length > TCP_MSG_WITHOUT_DATA)
    {
        dataLen = p->ui16Length - TCP_MSG_WITHOUT_DATA;
        //clear verify code
        *(unsigned char*)(((tcpMsgHaveData_t*)packet)->pData + dataLen) = 0;
    }
    else
    {
        dataLen = 0;
        //clear verify code
        p->verifyCode = 0;
    }

    for(int i=0; i < p->ui16Length; i++){
        verifyCode += *((char*)packet + i);
    }
    return (0 - verifyCode);
}

/*
 * This func will malloc a memory
 * User must free the malloc memory space manually
*/
void *threadTcpServer::createTmpPacketAndRecvToEnd(unsigned short* ui16Length)
{
    void* packet = NULL;
    tcpMsgGetLen_t hvl;

    if(!ui16Length)
    {
        qDebug("Error: ui16Length is NULL!");
        return NULL;
    }


    if(!parseHeaderfindLength(&hvl)) *ui16Length = hvl.ui16Length;
    else
    {
        qDebug("Error: parseHeaderfindLength failed!");
        return NULL;
    }

    if(*ui16Length > 0) packet = malloc(*ui16Length);
    else
    {
        qDebug("Error: malloc packet failed!");
        return NULL;
    }
    //file head to len
    if(packet) memcpy(packet, &hvl, sizeof(tcpMsgGetLen_t));
    else
    {
        qDebug("Error: memcpy packet failed!");
        return NULL;
    }

    //read packet to the end, if it could be.
    clientCon->read((char*)packet+sizeof(tcpMsgGetLen_t) , *ui16Length-sizeof(tcpMsgGetLen_t));

    qDebug("---recv msg len: %d---", *ui16Length);
    if(*ui16Length > TCP_MSG_WITHOUT_DATA)
        qDebug() << "---recv dataLen:" << (*ui16Length - TCP_MSG_WITHOUT_DATA);
    else
        qDebug() << "---recv dataLen: 0";
    return packet;
}

void *threadTcpServer::createNewPacketAndFillIt(void* recvMsg, char *data, unsigned short dataLen, unsigned short fedBck)
{
    void* pack = malloc(TCP_MSG_WITHOUT_DATA + dataLen);
    if(!pack) return NULL;

    bzero(pack, TCP_MSG_WITHOUT_DATA + dataLen);
    //fill contract data to the  pack
    fillAppointedData(recvMsg, pack, data, dataLen, fedBck);

    return pack;
}

void *threadTcpServer::createLogPacketAndFillIt(void* recvMsg, char *log, unsigned short logLen, unsigned short fedBck)
{
    void* pack = malloc(TCP_MSG_WITHOUT_DATA + 4 + logLen);
    if(!pack) return NULL;

    bzero(pack, TCP_MSG_WITHOUT_DATA + 4 + logLen);
    //fill contract log data to the  pack
    fillAppointedDataForLog(recvMsg, pack, log, logLen, fedBck);

    return pack;
}

void threadTcpServer::trasHardwareSoftwareVersion(void *recvMsg)
{

    //must be end with a '\0'
    //system("cat /run/sysversion | awk 'NR==1 {print}'");
    QByteArray version(BOXV3_SOFTWARE_VERSION);
    version.insert(version.size()-1, '\0');
    void* pack = createNewPacketAndFillIt(recvMsg, version.data(), version.size(), DEVICEFEEKERRCODE_OK);
    if(!pack) pack = createNewPacketAndFillIt(recvMsg, NULL, 0, DEVICEFEEKERRCODE_ERR);

    clientCon->write((char*)pack, ((tcpMsgWithoutData_t*)pack)->ui16Length);
    clientCon->waitForBytesWritten();
    free(pack);
    pack = NULL;
}

void threadTcpServer::trasHeartbeat(void* recvMsg)
{
    qDebug("---trasHeartbeat()---");
    void* pack = createNewPacketAndFillIt(recvMsg, NULL, 0, DEVICEFEEKERRCODE_OK);

    qDebug("send a tcp msg:");
    showTcpMsg(pack);
    clientCon->write((char*)pack, ((tcpMsgWithoutData_t*)pack)->ui16Length);
    clientCon->waitForBytesWritten();
    free(pack);
    pack = NULL;
}

void threadTcpServer::setMacAndSerialNum(tcpMsgHaveData_t* packet)
{
    if(packet->ui16Length <= TCP_MSG_WITHOUT_DATA)
    {
        qDebug("Error: Wrong packet for set mac and serial from tcp client.");
        return;
    }
    bool flag = false;
    if(1)
    {
        QMutexLocker lockerAt88(&mutexAT88);
        qDebug() << "---setMacAndSerialNum()---";
        objectAT88SC104C at88sc((char*)AT88SC104_NODE_NAME);
        tcp_pvalue_main_info_t* info = (tcp_pvalue_main_info_t*)(packet->pData);

        //unsigned short dataLen = packet->ui16Length - TCP_MSG_WITHOUT_DATA;
        //1). set serial number
        flag = at88sc.setDeviceCode((char*)(info->serianNum));
        //2). set lan mac
        flag &= at88sc.setSpecificMac(0, (char*)(info->macLan));
        //set ether hw lan
        if(flag) setLanMac((char*)(info->macLan));
        //3). set wan mac
        flag &= at88sc.setSpecificMac(1, (char*)(info->macWan));
        //set ether hw wan
        if(flag) setWanMac((char*)(info->macWan));
        //4). set p2pId
        flag &= at88sc.setP2pId((char*)(info->p2pid));
        qDebug("get serialNum and mac from tcp client:");
        showBuf((char*)(info), sizeof(tcp_pvalue_main_info_t));

    }
    //write done, now, feedback to client
    void* pack = NULL;
    if(flag)
        pack = createNewPacketAndFillIt((void*)packet, NULL, 0, DEVICEFEEKERRCODE_OK);
    else
        pack = createNewPacketAndFillIt((void*)packet, NULL, 0, DEVICEFEEKERRCODE_ERR);

    if(!pack)return;

    qDebug("send a tcp msg:");
    showTcpMsg(pack);
    clientCon->write((char*)pack, ((tcpMsgWithoutData_t*)pack)->ui16Length);
    clientCon->waitForBytesWritten();
    free(pack);
    pack = NULL;
}

void threadTcpServer::getMacAndSerialNum(void *recvMsg)
{
    tcp_pvalue_main_info_t info;

    memset(&info, 0, sizeof(tcp_pvalue_main_info_t));
    qDebug() << "---getMacAndSerialNum()---";
    if(1)
    {
        QMutexLocker lockerAt88(&mutexAT88);
        objectAT88SC104C at88sc((char*)AT88SC104_NODE_NAME);
        at88sc.getDeviceCode((char*)(info.serianNum));
        at88sc.getP2pId((char*)(info.p2pid));
        at88sc.getSpecificMac(0, (char*)(info.macLan));
        at88sc.getSpecificMac(1, (char*)(info.macWan));
        at88sc.getSpecificMac(2, (char*)(info.mac4G));
        at88sc.getSpecificMac(3, (char*)(info.macWifi));
        qDebug("get mac and serial form at88:");
        showBuf((char*)(&info), sizeof(tcp_pvalue_main_info_t));
    }

    void* pack = createNewPacketAndFillIt(recvMsg, (char*)(&info), sizeof(tcp_pvalue_main_info_t), DEVICEFEEKERRCODE_OK);
    if(!pack)return;

    qDebug("send a tcp msg:");
    showTcpMsg(pack);
    clientCon->write((char*)pack, ((tcpMsgWithoutData_t*)pack)->ui16Length);
    clientCon->waitForBytesWritten();
    free(pack);
    pack = NULL;
}

void threadTcpServer::trasLogFile(void *recvMsg)
{
    QString logFileStr(currentLogFileName);
    QFile logFile(logFileStr);

    msleep(10);

    if(!(logFile.open(QFile::ReadOnly | QIODevice::Text)))
    {
        QByteArray info("logFile doesn't exist!");
        qDebug("logFile:%s%s* doesn't exist! Open failed!", BOXV3_SELFCHECK_LOGDIR_PATH, BOXV3_SELFCHECK_LOGNAME_PREFIX);
        void* pack = createNewPacketAndFillIt(recvMsg, info.data(), info.size(), DEVICEFEEKERRCODE_ERR);
        clientCon->write((char*)pack, ((tcpMsgHaveData_t*)pack)->ui16Length);
        clientCon->waitForBytesWritten();
        free(pack);
        pack = NULL;
    }else
    {
        QTextStream in(&logFile);
        QString logFileString = in.readAll();
        QByteArray ch = logFileString.toLatin1();
        logFile.close();

        void* pack = createLogPacketAndFillIt(recvMsg, ch.data(), ch.size(), DEVICEFEEKERRCODE_OK);
        clientCon->write((char*)pack, ((tcpMsgHaveData_t*)pack)->ui16Length);
        clientCon->waitForBytesWritten();
        qDebug("send a tcp log msg(total: %dbytes):", ((tcpMsgHaveData_t*)pack)->ui16Length);
        showTcpLogMsg(pack);
        free(pack);
        pack = NULL;

    }
}

void threadTcpServer::analyzeTcpPacket(void* pack)
{
#if 1 //2.  parse msg
    tcpMsgWithoutData_t* msg = (tcpMsgWithoutData_t*)pack;
    //judge if it is Box-V3's cmd
    if(0 != msg->ucMainCmd)
    {
        qDebug() << "It's not Box-v3's msg";
        return;
    }
    //judge the client is wheather read or write
    qDebug("---type:%d, maincmd:%d, subcmd:%d", msg->ucCmdType, msg->ucMainCmd, msg->ucSubCmd);
    switch(msg->ucCmdType)
    {
        case TCP_CLIENT_READ:
        {
            switch(msg->ucSubCmd)
            {
            case 0x00: //1. client want to read heartbeat
                trasHeartbeat(pack);
                break;
            case 0x01://read arm's hardware and software version
                trasHardwareSoftwareVersion(pack);
                break;
            case 0x02://read mac and serial num
                getMacAndSerialNum(pack);
                break;
            case 0x03://get log file and set flag
            {
                trasLogFile(pack);
#ifdef BOXV3_DEBUG_NET
                emit signalTcpTransceiverLogDone();
                break;
#endif
            }
            case 0x04://change to next stage by pc
            {
                if(0x04 == msg->ucSubCmd)
                {
                    emit signalRecorder(LOG_CRIT, "", "change to next stage by pc");
                    trasHeartbeat(pack);
                }
                if(1)
                {
                    QMutexLocker lockerAt88(&mutexAT88);
                    objectAT88SC104C at88((char*)AT88SC104_NODE_NAME);
                    switch (at88.getCheckStage()) {
                    case SELFCHECK_PRODUCTION:
                    {
                        at88.setCheckedFlags(SELFCHECK_PRODUCTION);
                        //qDebug("---Debug: pro check ok flag has been created in at88!");
                        QString logStr("Success: production check all have done ok!(");
                        logStr.append((char)(at88.getCheckStage()));
                        logStr.append(")Production  flag has been created!");
                        emit signalRecorder(LOG_CRIT, "", logStr);
                        emit signalRecorder(LOG_CRIT, "tcpPro");
                        system("rm -rf /opt/private/log/boxv3/agingcheck/*");
                        qDebug("rm -rf /opt/private/log/boxv3/agingcheck/*");
                        break;
                    }
                    case SELFCHECK_AGED:
                    {
                        at88.setCheckedFlags(SELFCHECK_AGED);
                        //qDebug("---Debug: aged check ok flag has been created in at88!");
                        QString logStr("Success: aged check all have done ok!(");
                        logStr.append((char)(at88.getCheckStage()));
                        logStr.append(")Aged check ok flag has been created!");
                        emit signalRecorder(LOG_CRIT, "", logStr);
                        at88.setCheckedFlags(SELFCHECK_ALL_STAGE_OK);
                        //qDebug("---Debug: all check ok flag has been created in at88!");
                        logStr.remove(0, logStr.size());
                        logStr.append("Success: all check have done ok!(");
                        logStr.append((char)(at88.getCheckStage()));
                        logStr.append((char*)")All check ok flag has been created!");
                        emit signalRecorder(LOG_CRIT, "", logStr);
                        emit signalRecorder(LOG_CRIT, "tcpAged");

                        quit_sighandler(SIGUSR2);
                        break;
                    }
                    default:
                        break;
                    }
                }
                emit signalTcpTransceiverLogDone();
                qDebug() << __func__<<  __LINE__ << endl;
                QTimer::singleShot(0, &tcpTimeoutKeeper,SLOT(stop()));
                qDebug() << __func__<<  __LINE__ << endl;
                break;
            }
            default:
                break;
            }
            break;
        }
        case TCP_CLIENT_WRITE:
        {
            switch(msg->ucSubCmd)
            {
            case 0x02: //2. client wants to write mac and serial num
                setMacAndSerialNum((tcpMsgHaveData_t*)pack);
                break;
            default:
                break;
            }
           break;
        }
        default:
        {
            break;
        }
    }

#endif //parse msg

    return;
}

void threadTcpServer::revcMessage()
{
    unsigned short dataLen;
    //1. recv msg, malloc pack
    void* pack = createTmpPacketAndRecvToEnd(&dataLen);
    if(!pack)
    {
        qDebug() << "create packet failed.";
        return;
    }
    //test with show
    qDebug() << "\nget a tcp msg.";
    if(0x03 != ((tcpMsgWithoutData_t*)pack)->ucSubCmd) showTcpMsg(pack);

    //2. analyze the pack
    analyzeTcpPacket(pack);

    //3. Do not forget to free pack
    free(pack);
    pack = NULL;

    //system("touch /home/flags.txt");
    //qDebug() << "/home/flags.txt has been created!";
#if 0
//   clientCon->disconnected();
//   qDebug() << "tcpClient was forced disconnected.";
#endif
}


void threadTcpServer::transceiverMsg()
{
    qDebug() << "transceiverMsg: " << QThread::currentThread();
    emit signalRecorder(LOG_NOTICE, "", "new tcpClient is beening listened...");

    //server gets the new socket which has been connected latest
    clientCon = tcpServer->nextPendingConnection();

    //if(clientCon->state() && QAbstractSocket::ConnectedState)
    {
        //shot down the udp broadcast this time
        emit signalStopUdpBroadcast();
        //make sys delete clientCon in proper time
        connect(clientCon, SIGNAL(disconnected()), clientCon, SLOT(deleteLater()), Qt::QueuedConnection);
        //if readyRead
        connect(clientCon, SIGNAL(readyRead()), this, SLOT(revcMessage()), Qt::QueuedConnection);
    }

#if 0
    //disconnect after clientCon's transmition has been done.
    //clientCon->waitForDisconnected(40000);
#endif
}

void threadTcpServer::slotSendLogName(QByteArray logFileName)
{
    currentLogFileName = logFileName;
}

void threadTcpServer::slotStartTcpTrasceiver()
{

    qDebug("---debug---%s(line:%d.)\n", __func__, __LINE__);

    if(false == startedFlag)
    {
        qDebug("---debug---%s(line:%d.)\n", __func__, __LINE__);
        startedFlag = true;

#ifndef BOXV3_DEBUG_NET
        qDebug() << __func__<<  __LINE__ << endl;
        tcpTimeoutKeeper.setInterval(60000);
        QTimer::singleShot(0, &tcpTimeoutKeeper,SLOT(start())); //60000
        qDebug() << __func__<<  __LINE__ << endl;
#else
        qDebug() << __func__<<  __LINE__ << endl;
        tcpTimeoutKeeper.setInterval(3000);
        QTimer::singleShot(0, &tcpTimeoutKeeper,SLOT(start())); //3000
        qDebug() << __func__<<  __LINE__ << endl;
#endif
        while(1)
        {
            if(!tcpServer->listen(QHostAddress::Any, TCP_PORT_NUMBER))
            {
                qDebug() << "tcp listen error:" << tcpServer->errorString();
                sleep(1);
            }else
            {
                break;
            }
        }

        //the Server emit newConnection() signal when there's a new client connecting the server
        //QObject::connect(tcpServer, SIGNAL(newConnection()), this, SLOT(transceiverMsg()), Qt::QueuedConnection);
        connect(tcpServer, SIGNAL(newConnection()), this, SLOT(transceiverMsg()), Qt::QueuedConnection);
    }
}

void threadTcpServer::slotTcpTimeout()
{
#ifndef BOXV3_DEBUG_NET
    emit signalRecorder(LOG_CRIT, "error tcpTimeout");

#if 1
    qDebug() << "tcp Host(" << tcpServer->serverAddress() << ") port:" << tcpServer->serverPort();
    qDebug() << "tcp sockerr:" << tcpServer->serverError() << "err: " << tcpServer->errorString();

#if 0
    tcpServer->waitForNewConnection(100000);
    clientCon = tcpServer->nextPendingConnection();
    qDebug() << "willbe state: " << QAbstractSocket::ConnectedState;
    qDebug() << "client tcp state: "<< clientCon->state();
#endif

#endif

#endif
}

void threadTcpServer::run()
{

    QObject::connect(&tcpTimeoutKeeper, SIGNAL(timeout()), this, SLOT(slotTcpTimeout()), Qt::QueuedConnection);
    exec();
#if 0
    //qDebug() << "--debug--tcpServerThread run(): " << QThread::currentThread();
    QTcpServer server;
    char buf[1024] = {};
    server.listen(QHostAddress::Any, 17117);
    server.waitForNewConnection(100000);
    qDebug() << "New Connection Established!";
    QTcpSocket *socket;
    socket = server.nextPendingConnection();
    while(socket->state()&&QAbstractSocket::ConnectedState)
    {
        bzero(buf, sizeof(buf));
        socket->waitForReadyRead(1000);
        socket->read((char*)buf,sizeof(buf));
        qDebug() << "read buf:" << buf;
        socket->write("HelloWorld!");
        socket->waitForBytesWritten();
    }
    qDebug() << "Connection close";
    socket->close();
    server.close();
#endif

}

