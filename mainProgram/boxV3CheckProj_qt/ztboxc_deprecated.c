
#include "ztboxc_deprecated.h"


int rtcTest(){
    return system("hwclock");
}

int msataTest(){
    return access("/dev/sda",R_OK);
}

int TFCardTest(){
    if(access("/dev/mmcblk2",R_OK) != 0){
        return -1;
    }

    if(access("/media/mmcblk2",R_OK) == 0){
        return system("cp /opt/HWDaemon.out /media/mmcblk2");
    }else if(access("/media/mmcblk2p1",R_OK) == 0){
        return system("cp /opt/HWDaemon.out /media/mmcblk2p1 -f");
    }else{
        return -1;
    }
}

int I2c_Sendchar(int fd,char c)
{
  printf("We send one char %02X!\n",c);
  write(fd, &c, 1);
  return 1;
}

int I2c_SendStr(int fd, char *s, unsigned short no)
{
    printf("We will send %d char!\n",no);
    return write(fd, s, no);
}


int I2c_RcvStr(int fd, char *s, unsigned short no)
{
  read(fd, s, no);
  return 0;
}

int e2promTest(){
    int fd;
    int ret = -1;
    char *a = "1234";
    char src[12];
    src[0] = 0;
    memcpy(&src[1],a,strlen(a));
    char com[12] = {0};
    fd = open("/dev/i2c-0", O_RDWR);
    if(fd < 0){
        printf("Open %s ERR!\n","/dev/i2c-0");
        return -1;
    }
    ret = ioctl(fd, I2C_SLAVE_FORCE, 0x50);
    if(ret < 0){
        printf("Set slave ERR!\n");
        return -1;
    }
    if(I2c_SendStr(fd,src,strlen(a)+1) < 0){
        return -1;
    }
    usleep(50000);
    I2c_Sendchar(fd,0);
    I2c_RcvStr(fd,com,4);
    //printf("recv is %s\n",com);
    printf("%02X %02X %02X %02X\n",com[0],com[1],com[2],com[3]);
    close(fd);
    if(strcmp(a,com) == 0){
        return 0;
    }else{
        return -1;
    }
}



static void Error(const char *Msg)
{
    fprintf (stderr, "%s\n", Msg);
    fprintf (stderr, "strerror() is %s\n", strerror(errno));
    //exit(1);
    return;
}

static inline void waitFdWriteable(int Fd)
{
    fd_set WriteSetFD;
    FD_ZERO(&WriteSetFD);
    FD_SET(Fd, &WriteSetFD);
    if (select(Fd + 1, NULL, &WriteSetFD, NULL, NULL) < 0)
    {
        Error(strerror(errno));
    }
}

int serialInit(const char * pDeviceName, int iDeviceSpeed)
{
    int iSerialFD = -1;
    struct termios serialAttr;

    iSerialFD = open(pDeviceName, O_RDWR, 0);


    if (iSerialFD < 0)
    Error("Unable to open device");

    if (fcntl(iSerialFD, F_SETFL, O_NONBLOCK) < 0)
    Error("Unable set to NONBLOCK mode");

    memset(&serialAttr, 0, sizeof(struct termios));
    serialAttr.c_iflag = IGNPAR;
    serialAttr.c_cflag = iDeviceSpeed | CS8 | CREAD | CLOCAL;
    serialAttr.c_cc[VMIN] = 0;
    serialAttr.c_cc[VTIME] = 0;

    tcflush(iSerialFD, TCIOFLUSH);

    if (tcsetattr(iSerialFD, TCIOFLUSH, &serialAttr) < 0)
    Error("Unable to set comm port");

    return iSerialFD;
}

void serialSend(int Fd, const char *atCmd, int num)
{
    int i = 0;

    for(i = 0; i < num; i++)
    {
        waitFdWriteable(Fd);
        if (write(Fd, &atCmd[i], 1) < 0)
         Error(strerror(errno));
    }
}

struct termios BackupTtyAttr;

int ttyInit(const char * pTTYName)
{
    int TtyFd = -1;
    struct termios TtyAttr;

    TtyFd = open(pTTYName, O_RDWR | O_NDELAY, 0);

    if (TtyFd < 0)
     Error("Unable to open tty");

    TtyAttr.c_cflag = B115200 | CS8 | CREAD | CLOCAL;
    if (tcgetattr(TtyFd, &BackupTtyAttr) < 0)
     Error("Unable to get tty");

    tcflush(TtyFd, TCIOFLUSH);

    if (tcsetattr(TtyFd, TCIOFLUSH, &TtyAttr) < 0)
     Error("Unable to set tty");

    return TtyFd;
}

void ttyQuit(int TtyFd)
{

    if (tcsetattr(TtyFd, TCSANOW, &BackupTtyAttr) < 0)
        Error("Unable to set tty");
    return;
}

int comm4GJuge()
{
    int CommFd;
    int DeviceSpeed = B115200;
    const char *DeviceName = "/dev/ttyUSB2";
    int ModeCommon = 0;
    int CommEstablishFlag = 0;
    FILE *fp;

    char bufMobile[100];
    char commandMobile[64];
    char mobileState;
    mobileState = 0;

    sprintf(commandMobile,"ls /dev  |grep ttyUSB");
    if((fp = popen(commandMobile,"r"))==NULL){
        printf("popen");
        return 0;
    }else{
        while(fgets(bufMobile,100,fp)!=NULL){
            if(strstr(bufMobile,"ttyUSB0")!=NULL){
                mobileState |= 1;
                continue;
            }else if(strstr(bufMobile,"ttyUSB1")!=NULL){
                mobileState |= (1 << 1);
                continue;
            }else if(strstr(bufMobile,"ttyUSB2")!=NULL){
                mobileState |= (1 << 2);
                continue;
            }else if(strstr(bufMobile,"ttyUSB3")!=NULL){
                mobileState |= (1 << 3);
                continue;
            }else if(strstr(bufMobile,"ttyUSB4")!=NULL){
                mobileState |= (1 << 4);
                continue;
            }
        }
        pclose(fp);
    }
    if(mobileState != 31){
        return 4;  //模块故障
    }

    CommFd = serialInit(DeviceName, DeviceSpeed);

    if(CommFd < 0)
    {
        return 0;
    }

    serialSend(CommFd, AT, strlen(AT));
    int i;
    for (i = 0; i <= 10; i++)
    {
        usleep(500);
        char Char = 0;
        fd_set ReadSetFD;

        int Icnt;
        char CommFdInfoFlag = 0;
        char CommFdBuffer[1024];
        Icnt = 0;
        void OutputStdChar(FILE *File)
         {
             char Buffer[10];
             int Len = sprintf(Buffer, "%c", Char);
             fwrite(Buffer, 1, Len, File);
         }

        FD_ZERO(&ReadSetFD);
        FD_SET(CommFd, &ReadSetFD);

        if (select(CommFd + 1, &ReadSetFD, NULL, NULL, NULL) < 0)
            Error(strerror(errno));

        if (FD_ISSET(CommFd, &ReadSetFD))
        {
            while (read(CommFd, &Char, 1) == 1)
            {
                if (Char == '\x0a') {
                    CommFdInfoFlag = 1;
                }

                sprintf(&CommFdBuffer[Icnt], "%c", Char);
                Icnt++;
                if (Icnt >= 1023 )
                {
                    CommFdInfoFlag = 1;
                    break;
                }
            }

            if (CommFdInfoFlag == 1)
            {
                CommFdInfoFlag = 0;
                Icnt = 0;
                    if (!ModeCommon){
                        if (strstr(CommFdBuffer,"CHN-CT") != NULL){
                            printf("it is ok--------CHN-CT-------,电信通信,%s(%d)-%s\n", __FILE__, __LINE__, __FUNCTION__);
                            ModeCommon = 1;
                        } else if (strstr(CommFdBuffer,"CHN-UNICOM") != NULL){
                            printf("it is ok------CHN-UNICOM-----，联通通信,%s(%d)-%s\n", __FILE__, __LINE__, __FUNCTION__);
                            ModeCommon = 2;
                        } else if (strstr(CommFdBuffer,"CMCC") != NULL){
                            printf("it is ok-------CMCC--------，移动通信,%s(%d)-%s\n", __FILE__, __LINE__, __FUNCTION__);
                            ModeCommon = 3;
                        } else {
                            ModeCommon = 0;
                        }

                    }else if (strstr(CommFdBuffer,"NDISSTAT: 1,,,") != NULL){
                        printf("it is ok--------all ok, just wait -------,通信建立成功，等待设备获取ip,%s(%d)-%s\n", __FILE__, __LINE__, __FUNCTION__);
                        CommEstablishFlag = 1;
                        //system("ifconfig eth0 up");
                        close(CommFd);
                        return ModeCommon;
                    }else if (strstr(CommFdBuffer,"ERROR") != NULL){
                        close(CommFd);
                        return 5;//sim卡故障
                    }
            }
        }

        if (!CommEstablishFlag){
            printf("it is ok---------restart build-----------------,%s(%d)-%s\n", __FILE__, __LINE__, __FUNCTION__);
            switch(ModeCommon){
                case 1:  //电信
                    serialSend(CommFd, AT_AUTHDATA_S, strlen(AT_AUTHDATA_S));
                    serialSend(CommFd, AT_NDISDUP_CTNET_S, strlen(AT_NDISDUP_CTNET_S));
                    break;
                case 2://联通
                    serialSend(CommFd, AT_NDISDUP_3GNET_S, strlen(AT_NDISDUP_3GNET_S));
                    break;
                case 3://移动
                    serialSend(CommFd, AT_NDISDUP_CMNET_S, strlen(AT_NDISDUP_CMNET_S));
                    break;
                case 0://查询
                default:
                    serialSend(CommFd, AT_COPS_Q, strlen(AT_COPS_Q));
                    sleep(1);
                    break;
            }
        }
        if(i == 20){
            close(CommFd);
            return ModeCommon;
        }
    }
}

/*******************************************
 *	获得端口名称
********************************************/
char *get_ptty(pportinfo_t pportinfo)
{
    char *ptty;

    switch(pportinfo->tty){
        case '0':{
            ptty = TTY_DEV"0";
        }break;
        case '1':{
            ptty = TTY_DEV"1";
        }break;
        case '2':{
            ptty = TTY_DEV"2";
        }break;
        case '3':{
            ptty = TTY_DEV"3";
        }break;
        case '4':{
            ptty = TTY_DEV"4";
        }break;
        case '5':{
            ptty = TTY_DEV"5";
        }break;
    }
    return(ptty);
}

/*******************************************
 *	波特率转换函数（请确认是否正确）
********************************************/
int convbaud(unsigned long int baudrate)
{
    switch(baudrate){
        case 2400:
            return B2400;
        case 4800:
            return B4800;
        case 9600:
            return B9600;
        case 19200:
            return B19200;
        case 38400:
            return B38400;
        case 57600:
            return B57600;
        case 115200:
            return B115200;
        default:
            return B9600;
    }
}

/*******************************************
 *	Setup comm attr
 *	fdcom: 串口文件描述符，pportinfo: 待设置的端口信息（请确认）
 *
********************************************/
int PortSet(int fdcom, const pportinfo_t pportinfo)
{
    struct termios termios_old, termios_new;
    int 	baudrate, tmp;
    char	databit, stopbit, parity, fctl;

    bzero(&termios_old, sizeof(termios_old));
    bzero(&termios_new, sizeof(termios_new));
    cfmakeraw(&termios_new);
    tcgetattr(fdcom, &termios_old);			//get the serial port attributions
    /*------------设置端口属性----------------*/
    //baudrates
    baudrate = convbaud(pportinfo -> baudrate);
    cfsetispeed(&termios_new, baudrate);		//填入串口输入端的波特率
    cfsetospeed(&termios_new, baudrate);		//填入串口输出端的波特率
    termios_new.c_cflag |= CLOCAL;			//控制模式，保证程序不会成为端口的占有者
    termios_new.c_cflag |= CREAD;			//控制模式，使能端口读取输入的数据

    // 控制模式，flow control
    fctl = pportinfo-> fctl;
    switch(fctl){
        case '0':{
            termios_new.c_cflag &= ~CRTSCTS;		//no flow control
        }break;
        case '1':{
            termios_new.c_cflag |= CRTSCTS;			//hardware flow control
        }break;
        case '2':{
            termios_new.c_iflag |= IXON | IXOFF |IXANY;	//software flow control
        }break;
    }

    //控制模式，data bits
    termios_new.c_cflag &= ~CSIZE;		//控制模式，屏蔽字符大小位
    databit = pportinfo -> databit;
    switch(databit){
        case '5':
            termios_new.c_cflag |= CS5;
        case '6':
            termios_new.c_cflag |= CS6;
        case '7':
            termios_new.c_cflag |= CS7;
        default:
            termios_new.c_cflag |= CS8;
    }

    //控制模式 parity check
    parity = pportinfo -> parity;
    switch(parity){
        case '0':{
            termios_new.c_cflag &= ~PARENB;		//no parity check
        }break;
        case '1':{
            termios_new.c_cflag |= PARENB;		//odd check
            termios_new.c_cflag &= ~PARODD;
        }break;
        case '2':{
            termios_new.c_cflag |= PARENB;		//even check
            termios_new.c_cflag |= PARODD;
        }break;
    }

    //控制模式，stop bits
    stopbit = pportinfo -> stopbit;
    if(stopbit == '2'){
        termios_new.c_cflag |= CSTOPB;	//2 stop bits
    }
    else{
        termios_new.c_cflag &= ~CSTOPB;	//1 stop bits
    }

    //other attributions default
    termios_new.c_oflag &= ~OPOST;			//输出模式，原始数据输出
    termios_new.c_cc[VMIN]  = 1;			//控制字符, 所要读取字符的最小数量
    termios_new.c_cc[VTIME] = 1;			//控制字符, 读取第一个字符的等待时间	unit: (1/10)second

    tcflush(fdcom, TCIFLUSH);				//溢出的数据可以接收，但不读
    tmp = tcsetattr(fdcom, TCSANOW, &termios_new);	//设置新属性，TCSANOW：所有改变立即生效	tcgetattr(fdcom, &termios_old);
    return(tmp);
}

/*******************************************
 *	Open serial port
 *	tty: 端口号 ttyS0, ttyS1, ....
 *	返回值为串口文件描述符
********************************************/
int PortOpen(pportinfo_t pportinfo)
{
    int fdcom;	//串口文件描述符
    char *ptty;

    ptty = get_ptty(pportinfo);
    //fdcom = open(ptty, O_RDWR | O_NOCTTY | O_NONBLOCK | O_NDELAY);
    fdcom = open(ptty, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if(fdcom < 0){
    }else{
        printf("Open %s OK!\n",ptty);
    }
    return (fdcom);
}

/*******************************************
 *	Close serial port
********************************************/
void PortClose(int fdcom)
{
    close(fdcom);
}

/********************************************
 *	send data
 *	fdcom: 串口描述符，data: 待发送数据，datalen: 数据长度
 *	返回实际发送长度
*********************************************/
int PortSend(int fdcom, char *data, int datalen)
{
    int len = 0;

    len = write(fdcom, data, datalen);	//实际写入的长度
    if(len == datalen){
        return (len);
    }
    else{
        tcflush(fdcom, TCOFLUSH);
        return -1;
    }
}
/*******************************************
 *	receive data
 *	返回实际读入的字节数
 *
********************************************/
int PortRecv(int fdcom, char *data, int datalen)
//int PortRecv()
{
    int readlen = 0, fs_sel;
    fd_set	fs_read;
    struct timeval tv_timeout;

    FD_ZERO(&fs_read);
    FD_SET(fdcom, &fs_read);
    tv_timeout.tv_sec = 0;
    tv_timeout.tv_usec = 500000;

    fs_sel = select(fdcom+1, &fs_read, NULL, NULL, &tv_timeout);
    if(fs_sel){
        printf("We receive some thing!\n");

        char *rec = data;
        int reclen = 0;
        while((readlen = read(fdcom, rec, datalen)) > 0){
            printf("readlen is %d\n",readlen);
            rec = rec + readlen;
            reclen +=readlen;
            usleep(5000);
        }
        return(reclen);
    }
    else{
        return(-1);
    }
    return (readlen);
}


int  uartTest(int com){

    char RecvBuf[40];
    int i,SendLen, RecvLen;
    int fdcom;
    portinfo_t portinfo ={
        '0',                          	// print prompt after receiving
        9600,                      	// baudrate: 9600
        '8',                          	// databit: 8
        '0',                          	// debug: off
        '0',                          	// echo: off
        '2',                          	// flow control: software
        '1',                          	// default tty: COM0
        '0',                          	// parity: none
        '1',                          	// stopbit: 1
         0    	                  	// reserved
    };

    system("killall HWDaemon.out");
    system("/opt/watchdog.out &");

    if(com == 1){ //test the kaiguan
        portinfo.tty = '1';
        //portinfo.baudrate = 9600;
    }else if(com == 4){
        portinfo.tty = '4';
        //set baudrate ...
    }
    fdcom = PortOpen(&portinfo);
    if(fdcom<0){
        printf("Error: open serial port error.\n");
        return -1;
    }
    PortSet(fdcom, &portinfo);
    char search = 0xA0;
    SendLen = PortSend(fdcom, &search, 1);
    if(SendLen < 0){
        return -1;
    }
    RecvLen = PortRecv(fdcom, RecvBuf, sizeof(RecvBuf));
    if(RecvLen>0){
        for(i=0; i<RecvLen; i++){
            printf("Receive data No %d is %2x.\n", i, RecvBuf[i]);
        }
        if(RecvBuf[0] == 0x55 && RecvBuf[1] == 0xAA && RecvBuf[3] == 0xF0 && RecvBuf[4] == 0x0F && com == 1){
            return 0;
        }else if(RecvBuf[0] == 0xA0 && com == 4){
            return 0;
        }else{
            return -1;
        }
    }else{
        printf("Error: receive error.\n");
        return -1;
    }
}

int setMac(const char *mac){
    printf("the set mac is %s\n",mac);
    char cmd[64];
    sprintf(cmd,"echo %s > /media/mmcblk3p1/mac.ini",mac);
    if(system(cmd) != 0){
        return -1;
    }
    sprintf(cmd,"echo %s > /media/mmcblk3p2/mac.ini",mac);
    return system(cmd);
}

int getMac(char *mac){
    FILE *fp;
    if(mac == NULL){
        printf("mac is NULL\n");
        return -1;
    }
    if((fp = fopen("/media/mmcblk3p1/mac.ini","r")) == NULL){
        printf("open err!");
        return -1;
    }

    if(fgets(mac,20,fp) < 0){
        printf("fget err!\n");
        return -1;
    }
    *(mac + strlen(mac) -1) = 0;
    printf("the mac is %s\n",mac);
    fclose(fp);
    return 0;
}

int setDevId(const char *id){
    printf("the set id is %s\n",id);
    char cmd[64];
    sprintf(cmd,"echo %s > /media/mmcblk3p1/deviceId.ini",id);
    if(system(cmd) != 0){
        return -1;
    }
    sprintf(cmd,"echo %s > /media/mmcblk3p2/deviceId.ini",id);
    return system(cmd);
}

int getDevId(char *id){
    FILE *fp;
    if(id == NULL){
        printf("mac is NULL\n");
        return -1;
    }
    if((fp = fopen("/media/mmcblk3p1/deviceId.ini","r")) == NULL){
        printf("open err!");
        return -1;
    }

    if(fgets(id,40,fp) < 0){
        printf("fget err!\n");
        return -1;
    }
    *(id + strlen(id) -1) = 0;
    printf("the is is %s\n",id);
    fclose(fp);
    return 0;
}

void testVoiceOut(){
    system("amixer sset 'Headphone',0 127");
    system("aplay /opt/audio8k16S.wav &");
    system("arecord -d 5 -f S16_LE -t wav /run/test.wav");
}

void testVoiceIn(){
    //system("amixer sset 'Headphone',0 127");
    system("arecord -d 10 -f S16_LE -t wav /run/test.wav");
}

void playRecord(){
    system("aplay /run/test.wav");
}

void audioTest(){

}
