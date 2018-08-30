#ifndef ZTBOXC_DEPRECTED_H
#define ZTBOXC_DEPRECTED_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include <sys/times.h>          // times
#include <sys/types.h>          // pid_t
#include <termios.h>
#include <sys/ioctl.h>          // ioctl
#include <signal.h>
#include <time.h>
#include <sys/timeb.h>


#include <asm/types.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <poll.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <termio.h>
#include <time.h>
#include <unistd.h>

#define	TTY_DEV	"/dev/ttymxc"	//端口路径

#define AT "AT\r"
#define AT_COPS_Q "AT+COPS?\r"
#define AT_HCSQ_Q "AT^HCSQ?\r"
#define AT_AUTHDATA_S "AT^AUTHDATA=0,1\r"
#define AT_NDISDUP_CTNET_S "AT^NDISDUP=1,1,\"CTNET\"\r"
#define AT_NDISDUP_3GNET_S "AT^NDISDUP=1,1,\"3GNET\"\r"
#define AT_NDISDUP_CMNET_S "AT^NDISDUP=1,1,\"CMNET\"\r"

//串口结构
typedef struct{
    char	prompt;		//prompt after reciving data
    int 	baudrate;		//baudrate
    char	databit;		//data bits, 5, 6, 7, 8
    char 	debug;		//debug mode, 0: none, 1: debug
    char 	echo;			//echo mode, 0: none, 1: echo
    char	fctl;			//flow control, 0: none, 1: hardware, 2: software
    char 	tty;			//tty: 0, 1, 2, 3, 4, 5, 6, 7
    char	parity;		//parity 0: none, 1: odd, 2: even
    char	stopbit;		//stop bits, 1, 2
    const int reserved;	//reserved, must be zero
}portinfo_t;
typedef portinfo_t *pportinfo_t;

/*
 *	打开串口，返回文件描述符
 *	pportinfo: 待设置的串口信息
*/
int PortOpen(pportinfo_t pportinfo);
/*
 *	设置串口
 *	fdcom: 串口文件描述符， pportinfo： 待设置的串口信息
*/
int PortSet(int fdcom, const pportinfo_t pportinfo);
/*
 *	关闭串口
 *	fdcom：串口文件描述符
*/
void PortClose(int fdcom);
/*
 *	发送数据
 *	fdcom：串口描述符， data：待发送数据， datalen：数据长度
 *	返回实际发送长度
*/
int PortSend(int fdcom, char *data, int datalen);

int PortRecv(int fdcom, char *data, int datalen);

int rtcTest();
int msataTest();
int TFCardTest();
int e2promTest();
int comm4GJuge();
int  uartTest(int com);
int setMac(const char *mac);
int getMac(char *mac);
int setDevId(const char *id);
int getDevId(char *id);
int netTest(char *server);
int getIp(char *eth, char *ip);
int setIp(char *eth, const char *ip);
int eth0Test(const char *server);
int wifiTest(const char *server);
void testVoiceOut();
void testVoiceIn();
void playRecord();

int ntp_client (const char *ser);

#endif
