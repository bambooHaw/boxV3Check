#include "objectuart.h"

objectUart::objectUart(char *p):
    logArray("")
{
    bzero(filePath, sizeof(filePath));
    strncpy(filePath, p, strlen(p));
    //qDebug() << "filePath:" << QByteArray(filePath) << "\n";

}

objectUart::~objectUart()
{

}

void objectUart::setUartSpeed(int fd, int speed)
{
    unsigned int i;
    int status;
    struct termios opt;
    bzero(&opt,sizeof(struct termios));

    int speed_arry[] = {B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300};
    int name_arry[] = {115200, 38400, 19200, 9600, 4800, 2400, 1200, 300};

    tcgetattr(fd, &opt);

    for(i=0; i<sizeof(speed_arry)/sizeof(int); i++){
        if(name_arry[i] == speed)
            break;
    }

    tcflush(fd, TCIOFLUSH);
    cfsetispeed(&opt, speed_arry[i]);
    cfsetospeed(&opt, speed_arry[i]);

    opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);	//Input
    opt.c_lflag &= ~OPOST; //Output

    status = tcsetattr(fd, TCSANOW, &opt);
    if(0 != status){
        qDebug() << "tcsetattr fd";
        return;
    }

    tcflush(fd, TCIOFLUSH);
}


/*
 *fd: file descriptor
 *dataBits: 7/8
 *stopBits: 1/2
 *parity: N/E/O/S
 */
int objectUart::setUartParity(int fd, int databits, int stopbits, int parity)
{
    struct termios options;

        if(0 != tcgetattr(fd, &options)){
            qDebug("Setup serial 1");
            return -1;
        }
        options.c_lflag &= ~CSIZE;

        switch(databits){
            case 7:
                options.c_lflag |= CS7;
                break;
            case 8:
                options.c_lflag |= CS8;
                break;
            default:
                qDebug("Unsupported data size!\n");
                return -1;
        }


        switch(stopbits){
            case 1:
                options.c_cflag &= ~CSTOPB;
                break;
            case 2:
                options.c_cflag |= CSTOPB;
                break;
            default:
                qDebug("Unsupported stop bits!\n");
                return -1;
        }

        switch(parity){
            case 'n':
            case 'N':
                options.c_cflag &= ~PARENB; //disable
                options.c_iflag &= ~INPCK; //enable parity
                break;
            case 'o':
            case 'O':
                options.c_cflag |= (PARODD | PARENB); //set parity
                options.c_iflag |= INPCK;
                break;
            case 'e':
            case 'E':
                options.c_cflag |= PARENB; //enable pairty
                options.c_cflag &= ~PARODD; //exchange parityBits
                options.c_iflag |= INPCK;
                break;
            case 's':
            case 'S':
                options.c_cflag &= ~PARENB;
                options.c_cflag &= ~CSTOPB;
                break;
            default:
                qDebug("Unsupported parity!\n");
                return -1;
        }


        /*set priority*/
        if(parity != 'n')
            options.c_iflag |= INPCK;

        tcflush(fd, TCIOFLUSH);
        options.c_cc[VTIME] = 150; //set timeout as 5s
        options.c_cc[VMIN] = 0; //apply setopt immediately
        if(0 != tcsetattr(fd, TCSANOW, &options)){
            qDebug("Setup serial 3");
            return -1;
        }

        return 0;
}

void objectUart::calcStm8CmdParity(QByteArray &cmdArray)
{
    char unparity = 0;
    for(int i=0; i < cmdArray.size(); i++)  unparity += cmdArray[i];

    cmdArray[cmdArray.size() - 1 - 2] = 0 - unparity;
}

void objectUart::showBuf(char *buf, int len)
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

void objectUart::blockedMsecDelay(unsigned int msec)
{
    QTime pastTime = QTime::currentTime();

    QTime newTime;
    do{
        newTime = QTime::currentTime();
    }while ((unsigned int)pastTime.msecsTo(newTime) <= msec);
}

int objectUart::uartTransceiver()
{
    int ret = -1;
    fd_set rdFdset, wrFdset;
    char buf[FILE_PATH_LENGTH];
    struct timeval timer;
    bzero(&timer, sizeof(struct timeval));



    if(strncmp(FILE_PATH_UART232, filePath, strlen(filePath)) && strncmp(FILE_PATH_UART485, filePath, strlen(filePath)))
    {
        qDebug() << "Error:You should use FILE_PATH_UART** to define your own uart node path. It's not support init by a simple string.";
        emit signalSendUartCheckResult(2, 0x0);
        emit signalSendUartCheckResult(4, 0x0);
        return -1;
    }

    qDebug("In this uart check, %s will transceiver with other uart port.\n", filePath);
    fd = open(filePath, O_RDWR);
    //open uart and config it
    if(fd<0)
    {
        qDebug("Can't open '%s'\n", filePath);
        return -1;
    }else
    {
        //setUartSpeed(fd, 115200);
        setUartSpeed(fd, 9600);
       //if(-1 == setUartParity(fd, 8, 1, 'N'))
        if(-1 == setUartParity(fd, 8, 1, 'O'))
        {
            qDebug("Set parity error!\n");
            close(fd);
            return -2;
        }
    }

    /*
     * uart write() read() all the way when it could be done, until some close the object
     * repeat it again in 5s
     */
    qDebug() << "uart thread______:" ;
    qDebug() << QThread::currentThread();
    while(1)
    {
        timer.tv_sec = 5;
        timer.tv_usec = 0;
        FD_ZERO(&rdFdset);
        FD_SET(fd, &rdFdset);
        FD_ZERO(&wrFdset);
        FD_SET(fd, &wrFdset);
        ret = select(fd+1, &rdFdset, &wrFdset, NULL, &timer);
        qDebug("select(%s) return %d, fd=%d", filePath, ret, fd);
        if(0==ret){
            qDebug("Warning: Pay attention to %s's self physical connection. You should short-circuiting the Tx and Rx at 232port", filePath);
            continue;
        }else if(0 > ret)
        {
            qDebug("Warning: select(%s) return %d. [%d]:%s\n", filePath, ret, errno, strerror(errno));
            sleep(1);
            continue;
        }

        if(FD_ISSET(fd, &wrFdset))
        {
            ret = write(fd, filePath, strlen(filePath));
            qDebug("%s write(%d):", filePath, strlen(filePath));
            showBuf(filePath, strlen(filePath));
        }
        if(FD_ISSET(fd, &rdFdset))
        {
            bzero(buf, sizeof(buf));
            ret = read(fd, buf, sizeof(buf));
            qDebug("%s read(%d):", filePath, strlen(buf));
            showBuf(buf, strlen(buf));
#if 0
            if(!strncmp(filePath, FILE_PATH_UART232, strlen(filePath)))
            {
                /* If uart232 recv data "FILE_PATH_UART485" from uart485
                 * It proved that uart232 recv ok and uart485 send ok
                */
                if(!strncmp(buf, FILE_PATH_UART485, strlen(FILE_PATH_UART485)))
                {
                    emit signalSendUartCheckResult(2, 0x10);
                    emit signalSendUartCheckResult(4, 0x01);
                    qDebug() << "uart232 recv ok and uart485 send ok";
                }else
                {
                    emit signalSendUartCheckResult(2, 0x0);
                    emit signalSendUartCheckResult(4, 0x0);
                    qDebug() << "uart232 recv failed and uart485 send failed";
                }
                //uart 232 recv done!
                emit signalSendUartCheckResult(0x20, 0x00);
            }else
            {
                /* If uart485 recv data "FILE_PATH_UART232" from uart232
                 * It proved that uart485 recv ok and uart232 send ok
                */
                if(!strncmp(buf, FILE_PATH_UART232, strlen(FILE_PATH_UART232)))
                {
                    emit signalSendUartCheckResult(4, 0x10);
                    emit signalSendUartCheckResult(2, 0x01);
                    qDebug() << "uart232 recv ok and uart485 send ok";
                }else
                {
                    emit signalSendUartCheckResult(4, 0x0);
                    emit signalSendUartCheckResult(2, 0x0);
                    qDebug() << "uart232 recv failed and uart485 send failed";
                }
                //uart 485 recv done!
                emit signalSendUartCheckResult(0x40, 0x00);
            }
#endif
        }
        //do not write endless
        sleep(1);
    }

    close(fd);
    return 0;
}


int objectUart::uartCommunicateWithStm8(char* buf, int len)
{
//test 007
    int ret = -1;
    fd_set rdFdset;
    int timeoutCnt = 0;
    struct timeval timer;
    bzero(&timer, sizeof(struct timeval));

    if(strncmp(FILE_PATH_UARTSTM8, filePath, strlen(filePath)))
    {
        qDebug() << "Error:You should use FILE_PATH_UART** to define your own uart node path. It's not support init by a simple string.";
        return -1;
    }

    //open uart and config it
    fd = open(filePath, O_RDWR);
    if(fd<0)
    {
        qDebug("Can't open '%s'\n", filePath);
        return -2;
    }else
    {
        setUartSpeed(fd, 115200);
        if(-1 == setUartParity(fd, 8, 1, 'N'))
        {
            qDebug("Set parity error!\n");
            close(fd);
            return -3;
        }
    }

    /*
     * uart2 write() check cmd buf to stm8, select() stm8's read() in 5s,
     */
    /* write once, write check cmd buf to uart485
     * 9 is the cmd length , Please do not change it rashly
    */
    ret = write(fd, buf, len);

    while(1)
    {
        timer.tv_sec = 3;
        timer.tv_usec = 0;
        FD_ZERO(&rdFdset);
        FD_SET(fd, &rdFdset);
        //ret = select(fd+1, &rdFdset, &wrFdset, NULL, &timer);
        ret = select(fd+1, &rdFdset, NULL, NULL, &timer);
        //qDebug("select(%s) return %d, fd=%d", filePath, ret, fd);
        if(0 == ret)
        {
            timeoutCnt++;
            if(2 >= timeoutCnt)
            {
                ret = write(fd, buf, len);
            }else
            {
                qDebug("Can't read.");
                close(fd);
                return -5;
            }
        }
        if(0>ret){
            qDebug("select(%s) return %d. [%d]:%s\n", filePath, ret, errno, strerror(errno));
            close(fd);
            return -4;
        }else
        {
            if(FD_ISSET(fd, &rdFdset)){

                bzero(buf, STM8_CMD_AND_DATA_LENGTH_MAX);
                ret = read(fd, buf, STM8_CMD_AND_DATA_LENGTH_MAX);
                close(fd);
                return 0;

            }
#if 0
            if(FD_ISSET(fd, &wrFdset)){
                /* write once, write check cmd buf to uart485
                 * 9 is the cmd length , Please do not change it rashly
                */
                ret = write(fd, buf, len);
                //debug, wait a few minutes for stm8
                sleep(2);
            }
#endif
        }
    }
    //test 007 end
    close(fd);
    return -6;
}

char objectUart::slotReadStm8Uart2ForNetStatus(void)
{
    char ch = 0;
    fd_set rdFdset;
    int len = 0, ret = -1;
    struct timeval timer;
    char buf[STM8_REGDATA_LENGTH_MAX] = {};


    bzero(&timer, sizeof(struct timeval));

    if(strncmp(FILE_PATH_UARTSTM8, filePath, strlen(filePath)))
    {
        qDebug() << "Error:You should use FILE_PATH_UART** to define your own uart node path. It's not support init by a simple string.";
        return -1;
    }

    //open uart and config it
    fd = open(filePath, O_RDWR);
    if(fd<0)
    {
        qDebug("Can't open '%s'\n", filePath);
        return -2;
    }else
    {
        setUartSpeed(fd, 115200);
        if(-1 == setUartParity(fd, 8, 1, 'N'))
        {
            qDebug("Set parity error!\n");
            close(fd);
            return -3;
        }
    }

    /*
     * uart2 write() check cmd buf to stm8, select() stm8's write() in 5s,
     */

    while(1)
    {
        timer.tv_sec = 5;
        timer.tv_usec = 0;
        FD_ZERO(&rdFdset);
        FD_SET(fd, &rdFdset);

        ret = select(fd+1, &rdFdset, NULL, NULL, &timer);
        qDebug("select(%s) return %d, fd=%d", filePath, ret, fd);
        if(0 == ret)
        {
            qDebug("Loop read uart2 for netStatus select() timeout(5s) again!");
            continue;
        }
        if(0>ret){
            qDebug("select(%s) return %d. [%d]:%s\n", filePath, ret, errno, strerror(errno));
            close(fd);
            return -4;
        }else
        {
            if(FD_ISSET(fd, &rdFdset))
            {
                ret = read(fd, &ch, 1);
                if(STM8_PROTOCOL_HEAD0 == ch)
                {
                    ret = read(fd, &ch, 1);
                    if(STM8_PROTOCOL_HEAD1 == ch)
                    {
                        ret = read(fd, &ch, 1);  //MCURW
                        ret = read(fd, &ch, 1);  //MainCode
                        ret = read(fd, &ch, 1);  //SubCode
                        ret = read(fd, &ch, 1);  //DataLen
                        len = ch;
                        if(len){
                            if(len > STM8_REGDATA_LENGTH_MAX) len = STM8_REGDATA_LENGTH_MAX;
                            bzero(buf, STM8_REGDATA_LENGTH_MAX);
                            ret = read(fd, buf, len);    //RegData
                            //test
                            qDebug("read uart2:");
                            showBuf(buf, len);
                            if(LINKMASK_LAN_PORTALL_CHECKED == beginWanAndLanTest(buf[0]))
                            {
                                ret = read(fd, &ch, 1);  //Parity
                                ret = read(fd, &ch, 1);  //0xF0
                                ret = read(fd, &ch, 1);  //0x0F
                                qDebug() << "All Wan ports have been checked!\n";
                                close(fd);
                                return 0;
                            }
                        }
                        ret = read(fd, &ch, 1);  //Parity
                        ret = read(fd, &ch, 1);  //0xF0
                        ret = read(fd, &ch, 1);  //0x0F

                    }
                }
                //return 0;
                continue;
            }
        }
    }

    close(fd);
    return -6;
}

char objectUart::beginWanAndLanTest(char port)
{
    static netCheckFlag_t netCheckFlag;
    char currentPort = 0;

    if(!(port & ~LINKMASK_PHY)) return -1;


    /* Check WAN port only when it has not been checked before.
     * Notes: Not support check up one more port.
    */
    if(!(port & netCheckFlag.checkedPort))
    {

        if(port & LINKMASK_LAN_PORT1)
        {
            netCheckFlag.checkedPort |= LINKMASK_LAN_PORT1;
            currentPort = LINKMASK_LAN_PORT1;
           // qDebug() << "Lan1 checking...";
        }else if(port & LINKMASK_LAN_PORT2)
        {
            netCheckFlag.checkedPort |= LINKMASK_LAN_PORT2;
            currentPort = LINKMASK_LAN_PORT2;
           // qDebug() << "Lan2 checing...";
        }else if(port & LINKMASK_LAN_PORT3)
        {
            netCheckFlag.checkedPort |= LINKMASK_LAN_PORT3;
            currentPort = LINKMASK_LAN_PORT3;
           // qDebug() << "Lan3 checking...";
        }else if(port & LINKMASK_LAN_PORT4)
        {
            netCheckFlag.checkedPort |= LINKMASK_LAN_PORT4;
            currentPort = LINKMASK_LAN_PORT4;
            //qDebug() << "Lan4 checking...";
        }

        //Start check Wan ports when anyone has connect with the Lan port
        if(currentPort)
        {
            QFile file;
            system("killall -9 ethCheck.sh");
            system("rm -rf /tmp/eth*");
            system("chmod 777 /opt/ethCheck.sh");
            system("/opt/ethCheck.sh");
            //sleep(15);
            //wan --- eth0
            file.setFileName("/tmp/eth0/send");
            if(!file.open(QIODevice::ReadOnly))
            {
               qDebug("open eth0 send file failed!");
            }else
            {
               QTextStream io(&file);
               QString line = io.readLine();
               if(line.contains("OK", Qt::CaseSensitive))
               {
                   netCheckFlag.sendWorkWellPort |= LINKMASK_WAN_PORT0;
               }else
               {
                   if(!(netCheckFlag.sendWorkWellPort & LINKMASK_WAN_PORT0))
                   {
                       netCheckFlag.sendWorkWellPort &= ~LINKMASK_WAN_PORT0;
                   }
               }
               file.close();
            }

            file.setFileName("/tmp/eth0/receive");
            if(!file.open(QIODevice::ReadOnly))
            {
               qDebug("open eth0 receive file failed!");
            }else
            {
               QTextStream io(&file);
               QString line = io.readLine();
               if(line.contains("OK", Qt::CaseSensitive))
               {
                   netCheckFlag.recvWorkWellPort |= LINKMASK_WAN_PORT0;
               }else
               {
                   if(!(netCheckFlag.recvWorkWellPort & LINKMASK_WAN_PORT0))
                   {
                       netCheckFlag.recvWorkWellPort &= ~LINKMASK_WAN_PORT0;
                   }
               }
               file.close();
            }

            //lan --- eth1
            file.setFileName("/tmp/eth1/send");
            if(!file.open(QIODevice::ReadOnly))
            {
               qDebug("open eth1 send file failed!");
            }else
            {
               QTextStream io(&file);
               QString line = io.readLine();
               if(line.contains("OK", Qt::CaseSensitive))
               {
                   netCheckFlag.sendWorkWellPort |= currentPort;
               }else
               {
                   netCheckFlag.sendWorkWellPort &= ~currentPort;
               }
               file.close();
            }
            file.setFileName("/tmp/eth1/receive");
            if(!file.open(QIODevice::ReadOnly))
            {
               qDebug("open eth1 receive file failed!");
            }else
            {
               QTextStream io(&file);
               QString line = io.readLine();
               if(line.contains("OK", Qt::CaseSensitive))
               {
                   netCheckFlag.recvWorkWellPort |= currentPort;
               }else
               {
                   netCheckFlag.recvWorkWellPort &= ~currentPort;
               }
               file.close();
            }
        }
    }
    else
    {
        if(port & LINKMASK_LAN_PORT1)
        {
            ;//qDebug() << "Lan1 connected again!";
        }
        if(port & LINKMASK_LAN_PORT2)
        {
            ;//qDebug() << "Lan2 connected again!";
        }
        if(port & LINKMASK_LAN_PORT3)
        {
            ;//qDebug() << "Lan3 connected again!";
        }
        if(port & LINKMASK_LAN_PORT4)
        {
            ;//qDebug() << "Lan4 connected again!";
        }
    }

//    qDebug("currentPort: %#x", currentPort);
//    qDebug("checkedPort: %#x", netCheckFlag.checkedPort);
//    qDebug("sendWorkWellPort: %#x", netCheckFlag.sendWorkWellPort);
//    qDebug("recvWorkWellPort: %#x", netCheckFlag.recvWorkWellPort);
    //send check info up
    sendNetworkCheckInfo(&netCheckFlag);

    return netCheckFlag.checkedPort;

}

int objectUart::checkUartStm8()
{
    char flag = 0;
#if 0
    char cmdBuf[2][FILE_PATH_LENGTH] = {
        {0x55, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x02, 0xF0, 0x0F},
        {0x55, 0xAA, 0x00, 0x00, 0x01, 0x00, 0x01, 0xF0, 0x0F}
    };	 //query the main/sub version of stm8 software
    const char fedBckBuf[2][FILE_PATH_LENGTH] = {
        {0x55, 0xaa, 0x80, 0x00, 0x00, 0x01, 0x41, 0x40, 0xf0, 0x0f},
        {0x55, 0xAA, 0x80, 0x00, 0x01, 0x01, 0x12, 0x6e, 0xF0, 0x0F}
    };
    //1. check main version of stm8
    if(uartCommunicateWithStm8(cmdBuf[0], 9)){
        return -1;
    }
    qDebug("Stm8 main version check fedbckbuf:");
    showBuf(cmdBuf[0], FILE_PATH_LENGTH);
    if(0 == strncmp(cmdBuf[0], fedBckBuf[0], 10))
    {
        flag |= 0x10;
    }
    //2. check main version of stm8
    if(uartCommunicateWithStm8(cmdBuf[1], 9)){
        return -1;
    }
    qDebug("Stm8 sub version check fedbckbuf:");
    showBuf(cmdBuf[1], FILE_PATH_LENGTH);
    if(0 == strncmp(cmdBuf[1], fedBckBuf[1], 10))
    {
        flag |= 0x01;
    }

#endif
    char buf[STM8_CMD_AND_DATA_LENGTH_MAX] = {};
    char bufLen = 0;
    char data = 0;

    // get main&sub version of stm8
    memset(buf, 0, sizeof(buf));
    slotGetStm8CmdBuf(buf, &bufLen, 0, 0, 0, 0, &data);
    uartCommunicateWithStm8(buf, bufLen);
    qDebug("Stm8 main version check fedbckbuf:");
    showBuf(buf, STM8_CMD_AND_DATA_LENGTH_MAX);
    //have fedback data
    if(buf[5] > 0x00)
    {
        if(buf[6] == BOXV3_MAIN_VERSION)
        {
            flag |= 0x10;
        }
    }
    memset(buf, 0, sizeof(buf));
    slotGetStm8CmdBuf(buf, &bufLen, 0, 0, 1, 0, &data);
    uartCommunicateWithStm8(buf, bufLen);
    qDebug("Stm8 sub version check fedbckbuf:");
    showBuf(buf, STM8_CMD_AND_DATA_LENGTH_MAX);

    //have fedback data
    if(buf[5] > 0x00)
    {
        if(buf[6] == BOXV3_SUB_VERSION)
        {
            flag |= 0x01;
        }
    }

    //forbid all lan port
    memset(buf, 0, sizeof(buf));
    data = LINKMASK_LAN_PORTALL_CHECKED;
    slotGetStm8CmdBuf(buf, &bufLen, 1, 1, 7, 1, &data);
    uartCommunicateWithStm8(buf, bufLen);

    return ((0x11 == flag) ? 0 : -2);
}

int objectUart::slotGetEthLinkedPortNum(void)
{
    char buf[STM8_CMD_AND_DATA_LENGTH_MAX] = {};
    char cmdArray[][STM8_CMD_AND_DATA_LENGTH_MAX] = {
        {0x55, 0xAA, 0x01, 0x01, 0x06, 0x01, 0x01, 0xF8, 0xF0, 0x0F},	 //enable stm8 auto upload wan linked status
        {0x55, 0xAA, 0x01, 0x01, 0x06, 0x01, 0x00, 0xF9, 0xF0, 0x0F},	 //disable stm8 auto upload wan linked status
        {0x55, 0xAA, 0x00, 0x01, 0x06, 0x00, 0xFB, 0xF0, 0x0F},          //query wan linked status from stm8 once
        {0x55, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x02, 0xF0, 0x0F},          //get info of main version
        {0x55, 0xAA, 0x00, 0x00, 0x01, 0x00, 0x01, 0xF0, 0x0F},          //get info of sub version
        {}
    };
    //query wan linked status from stm8 once
    memcpy(buf, cmdArray[4], STM8_CMD_AND_DATA_LENGTH_MAX);
    qDebug("write buf:");
    showBuf(buf, STM8_CMD_AND_DATA_LENGTH_MAX);
    uartCommunicateWithStm8(buf, 10);
    qDebug("read buf:");
    showBuf(buf, STM8_CMD_AND_DATA_LENGTH_MAX);
    return 0;
}

void objectUart::slotGetStm8CmdBuf(char* buf, char* bufLen, char rdwr, char mainCmd, char subCmd, char dataLen, char *data)
{

    if(!buf || !bufLen)
    {
        qDebug() << "Error: Wrong buf pointer.";
        return;
    }
    QByteArray stm8Cmd;
    stm8Cmd.append((char)0x55);
    stm8Cmd.append((char)0xaa);
    stm8Cmd.append(rdwr);
    stm8Cmd.append(mainCmd);
    stm8Cmd.append(subCmd);
    stm8Cmd.append(dataLen);
    if(dataLen>0)
        stm8Cmd.append(data, dataLen);
    stm8Cmd.append((char)0x00);//clear the parity
    stm8Cmd.append((char)0xf0);
    stm8Cmd.append((char)0x0f);

    calcStm8CmdParity(stm8Cmd);

    *bufLen = stm8Cmd.size();
    memcpy(buf, stm8Cmd.data(), *bufLen);
}

void objectUart::slotLanPortCtrlAndCheck()
{
    char buf[STM8_CMD_AND_DATA_LENGTH_MAX] = {};
    char bufLen = 0;
    char data = 0;
    char currentPort = 0;
    int i = 0;

    // forbiden all wan port, to prevent broadcast storm
    data = 0xe4;
    slotGetStm8CmdBuf(buf, &bufLen, 1, 1, 7, 1, &data);
    uartCommunicateWithStm8(buf, bufLen);

    system("echo PL9 1 1 1 1 1 > /debugfs/sunxi_pinctrl/sunxi_pin_configure");
    system("brctl delif br0 wlan0");
    system("ifconfig br0 down");
    system("brctl delbr br0");

    system("ifconfig lo down");
    system("ifconfig usb0 down");
    system("ifconfig wlan0 down");
    /*this version stm8 program has a problem when read stm8. So, do not read stm8*/
    //test lan port4 in turn
    for(i=1; i<5; i++)
    {
        switch(i)
        {
        case 1:
            currentPort = LINKMASK_LAN_PORT1;
            //data = 250;
            break;
        case 2:
            currentPort = LINKMASK_LAN_PORT2;
            //data = 222;
            break;
        case 3:
            currentPort = LINKMASK_LAN_PORT3;
            //data = 190;
            break;
        case 4:
            currentPort = LINKMASK_LAN_PORT4;
            //data = 126;
            break;
        default:
            break;
        }
        //enable the current lan port
        data = LINKMASK_LAN_PORTALL_CHECKED & (~currentPort);
        bzero(buf, sizeof(buf));
        slotGetStm8CmdBuf(buf, &bufLen, 1, 1, 7, 1, &data);
        uartCommunicateWithStm8(buf, bufLen);
        //test  currentport
        qDebug("----------------------test port(%#x)(i:%d)...", currentPort, i);
        beginWanAndLanTest(currentPort);
    }
   qDebug("All Lan ports has been checked done!");
}

void objectUart::slotTouchBarCodeGunRecorder()
{
    QByteArray array("mkdir -p ");
    array.append(QString(BOX_V3_BAR_CODE_TMP_FILE_DIR));
    system(array.data());

#if 0
    array.remove(0, array.size());
    array.append("rm -rf ");
    array.append(QString(BOX_V3_BAR_CODE_TMP_FILE));
    system(array.data());
#endif
    array.remove(0, array.size());
    array.append("touch ");
    array.append(QString(BOX_V3_BAR_CODE_TMP_FILE));
    system(array.data());
}

int objectUart::slotStartBarCodeGun()
{
    int fd=0, ret=-1;
    char code = 0, cnt = 0;
    fd_set rd_fdset;
    struct timeval dly_tm; //delay time in select()

    slotTouchBarCodeGunRecorder();
    fd = open(FILE_PATH_UART485, O_RDWR);
    if(fd<0)
    {
        qDebug("Error: open %s failed!", FILE_PATH_UART485);
        return -1;
    }

    setUartSpeed(fd, 115200);
    ret = setUartParity(fd, 8, 1, 'N');
    if(-1 == ret){
        qDebug("Error: set parity failed!\n");
        return ret;
    }

    //qDebug("Select(%s)...\n", FILE_PATH_UART485);

    cnt = 0;
    while(1)
    {
        FD_ZERO(&rd_fdset);
        FD_SET(fd, &rd_fdset);
        dly_tm.tv_sec = 0;
        dly_tm.tv_usec = 200*1000;
        ret = select(fd+1, &rd_fdset, NULL, NULL, &dly_tm);
        if(0==ret){
            if(cnt > 0){
                emit signalBarCodeGunSanDone(cnt);
                cnt = 0;
            }
            //qDebug("select(%s) again overtime(1s) return(%d), fd=%d", FILE_PATH_UART485, ret, fd);
            continue;
        }
        if(0 > ret){
            qDebug("select(%s) return %d. [%d]:%s\n", FILE_PATH_UART485, ret, errno, strerror(errno));
            close(fd);
            return -4;
        }
        if(FD_ISSET(fd, &rd_fdset)){
            ret = read(fd, &code, 1);
            if(ret > 0) cnt++;
            //qDebug("read(%s) return(%d).\n", FILE_PATH_UART485, ret);
            //qDebug("%c", code);
            //printf("slotWriteCharToFile...\n");
            slotWriteCharToFile(&code, (char*)BOX_V3_BAR_CODE_TMP_FILE);
        }
    }
    close(fd);

    return 0;
}

int objectUart::slotWriteCharToFile(char *code, char *filePath)
{

    int fd = open(filePath, O_RDWR|O_APPEND);
    if(fd < 0)
    {
        qDebug("open %s failed!", filePath);
        return fd;
    }
    write(fd, code, 1);
    close(fd);
    return 0;

#if 0
    char buf[64] = {};
    sprintf(buf, "echo %c >> %s", *code, FilePath);

    return system(buf);
#endif

#if 0
    QByteArray array("echo ");
    array.append(QString(code));
    array.append(QString(" >> "));
    array.append(QString(FilePath));

    return system(array.data());
#endif
}


char objectUart::slotSelfTransceiverSelf()
{
    int ret = -1;
    fd_set rdFdset, wrFdset;
    char buf[FILE_PATH_LENGTH];
    struct timeval timer;
    bzero(&timer, sizeof(struct timeval));

    qDebug("In this uart check, %s will transceiver with itself.", filePath);
    fd = open(filePath, O_RDWR);
    //open uart and config it
    if(fd<0)
    {
        qDebug("Can't open '%s'\n", filePath);
        return -1;
    }else
    {
        setUartSpeed(fd, 115200);
        if(-1 == setUartParity(fd, 8, 1, 'N'))
        {
            qDebug("Set parity error!\n");
            close(fd);
            return -2;
        }
    }

    /*
     * uart write() read() all the way when it could be done, until some close the object
     * repeat it again in 5s
     */
    while(1)
    {
        timer.tv_sec = 5;
        timer.tv_usec = 0;
        FD_ZERO(&rdFdset);
        FD_SET(fd, &rdFdset);
        FD_ZERO(&wrFdset);
        FD_SET(fd, &wrFdset);
        ret = select(fd+1, &rdFdset, &wrFdset, NULL, &timer);
        qDebug("select(%s) return %d, fd=%d", filePath, ret, fd);
        if(0==ret){
            qDebug("Warning: Pay attention to %s's self physical connection. You should short-circuiting the Tx and Rx at 232port", filePath);
            continue;
        }else if(0 > ret)
        {
            qDebug("Warning: select(%s) return %d. [%d]:%s\n", filePath, ret, errno, strerror(errno));
            continue;
        }
        if(FD_ISSET(fd, &rdFdset))
        {
            bzero(buf, sizeof(buf));
            ret = read(fd, buf, sizeof(buf));
            qDebug("%s read(%d):", filePath, strlen(buf));
            showBuf(buf, strlen(buf));
            if(!strncmp(buf, FILE_PATH_UART232, strlen(filePath)))
            {
                /* If uart232 recv data "FILE_PATH_UART232" from uart232
                 * It proved that uart232 works well
                */
                emit signalSendUartCheckResult(0x2, 0x10);
                break;
            }
        }
        if(FD_ISSET(fd, &wrFdset))
        {
            emit signalSendUartCheckResult(0x2, 0x01);
            ret = write(fd, filePath, strlen(filePath));
            qDebug("%s write(%d):", filePath, strlen(filePath));
            showBuf(filePath, strlen(filePath));
        }
        //do not write endless
        sleep(1);
    }

    close(fd);
    return 0;

}

char objectUart::sendNetworkCheckInfo(netCheckFlag_t *netCheckFlag)
{
    QByteArray netCheckFlagArray;
    netCheckFlagArray.resize(sizeof(netCheckFlag_t));
    if(!netCheckFlag) return -1;
    memcpy(netCheckFlagArray.data(), netCheckFlag, netCheckFlagArray.size());
    emit signalSendNetworkCheckInfo(netCheckFlagArray);

    return 0;
}

#if 0
char uartObject::slotCheckUart232AndUart485()
{
    int ret = -1;
    char anyRecvCnt = 0;
    int fdUart232 = -1, fdUart485 = -1, fdTmp = -1;
    char fdExchangeFlag = 0;
    fd_set rdFdset;
    char buf[FILE_PATH_LENGTH];
    struct timeval timer;
    bzero(&timer, sizeof(struct timeval));

    fdUart232 = open(FILE_PATH_UART232, O_RDWR); //open uart and config it
    if(fdUart232<0)
    {
        qDebug("Can't open '%s'\n", FILE_PATH_UART232);
        return -1;
    }else
    {
        setUartSpeed(fdUart232, 115200);
        if(-1 == setUartParity(fdUart232, 8, 1, 'N'))
        {
            qDebug("Set %s parity error!\n", FILE_PATH_UART232);
            close(fdUart232);
            return -2;
        }
    }
    fdUart485 = open(FILE_PATH_UART485, O_RDWR); //open uart and config it
    if(fdUart485<0)
    {
        qDebug("Can't open '%s'\n", FILE_PATH_UART485);
        close(fdUart232);
        return -1;
    }else
    {
        setUartSpeed(fdUart485, 115200);
        if(-1 == setUartParity(fdUart485, 8, 1, 'N'))
        {
            qDebug("Set %s parity error!\n", FILE_PATH_UART485);
            close(fdUart232);
            close(fdUart485);
            return -2;
        }
    }
    qDebug("In this uart check, %s will transceiver with other uart port.\n", filePath);


    /*
     * uart write() read() all the way when it could be done, until some close the object
     * repeat it again in 5s
     */
    qDebug() << "uart thread______:" ;
    qDebug() << QThread::currentThread();

    //Loop detect uart232 and uart485
    while(1)
    {
        while(1)//1. uart232 send, uart485 recv
        {
            timer.tv_sec = 5;
            timer.tv_usec = 0;
            FD_ZERO(&rdFdset);
            FD_SET(fdUart485, &rdFdset);

            //write some msg
            bzero(buf, sizeof(buf));
            buf[0] = fdUart232;
            ret = write(fd, buf, 1);
            qDebug("fd%d write %d to fd%d.\n", fdUart232, fdUart232, fdUart485);

            //delay for read, if it could be read
            if(fdUart232>fdUart485)
                ret = select(fdUart232+1, &rdFdset, NULL, NULL, &timer);
            else
                ret = select(fdUart485+1, &rdFdset, NULL, NULL, &timer);
            qDebug("select(%s) return %d, fd=%d", filePath, ret, fd);
            if(0==ret){
                qDebug("Warning: Pay attention to UART232 and UART485's physical connection with a uart485to232 converter.");
                break;
            }else if(0 > ret)
            {
                qDebug("Warning: select(%s) return %d. [%d]:%s\n", filePath, ret, errno, strerror(errno));
                sleep(1);
                break;
            }else
            {
                if(FD_ISSET(fdUart485, &rdFdset))
                {
                    bzero(buf, sizeof(buf));
                    ret = read(fd, buf, 1);
                    qDebug("fd%d read %d from fd%d.\n", fdUart485, fdUart232, fdUart232);
                    /* If uart485 recv data fdUart232 from uart232
                     * It proved that uart485 recv ok and uart232 send ok
                     */
                    if(fdUart232 == buf[0])
                    {
                        if(!fdExchangeFlag)
                        {
                            emit signalSendUartCheckResult(2, 0x01);
                            emit signalSendUartCheckResult(4, 0x10);
                            qDebug() << "uart485 recv ok and uart232 send ok";
                        }else
                        {
                            emit signalSendUartCheckResult(4, 0x01);
                            emit signalSendUartCheckResult(2, 0x10);
                            qDebug() << "uart232 recv ok and uart485 send ok";
                        }
                    }else
                    {
                        emit signalSendUartCheckResult(2, 0x0);
                        emit signalSendUartCheckResult(4, 0x0);
                        qDebug() << "uart485 recv failed and uart232 send failed";
                    }
                    anyRecvCnt++;
                    break;
                }
            }
        }//1. uart232 send, uart485 recv end

        //2. uart485 send, uart232 recv
        fdTmp = fdUart232;
        fdUart232 = fdUart485;
        fdUart485 = fdTmp;
        if(!fdExchangeFlag) fdExchangeFlag= 1; //Indicate the exchange of uart fds
        else fdExchangeFlag = 0;
        //if any success twice, then break;
        if(anyRecvCnt > 2) break;

    }//Loop detect uart232 and uart485 end

    close(fdUart232);
    close(fdUart485);
    return 0;

}
#endif
