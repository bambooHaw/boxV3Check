#include "ztboxc_deprecated.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <setjmp.h>
#include <errno.h>
#include <sys/time.h>

#define PACKET_SIZE 4096
#define MAX_NO_PACKETS  1


char sendpacket[PACKET_SIZE];
char recvpacket[PACKET_SIZE];
int sockfd,datalen = 56;
double temp_rtt[MAX_NO_PACKETS];
double all_time = 0;


struct sockaddr_in dest_addr;
struct sockaddr_in from;
struct timeval tvrecv;
pid_t pid;


//两个timeval相减
void tv_sub(struct timeval *recvtime,struct timeval *sendtime)
{
    long sec = recvtime->tv_sec - sendtime->tv_sec;
    long usec = recvtime->tv_usec - sendtime->tv_usec;
    if(usec >= 0){
        recvtime->tv_sec = sec;
        recvtime->tv_usec = usec;
    }else{
        recvtime->tv_sec = sec - 1;
        recvtime->tv_usec = -usec;
    }
}


/****检验和算法****/
unsigned short cal_chksum(unsigned short *addr,int len)
{
    int nleft = len;
    int sum = 0;
    unsigned short *w = addr;
    unsigned short check_sum = 0;

    while(nleft>1)       //ICMP包头以字（2字节）为单位累加
    {
        sum += *w++;
        nleft -= 2;
    }

    if(nleft == 1)      //ICMP为奇数字节时，转换最后一个字节，继续累加
    {
        *(unsigned char *)(&check_sum) = *(unsigned char *)w;
        sum += check_sum;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    check_sum = ~sum;   //取反得到校验和
    return check_sum;
}

/*设置ICMP报头*/
int pack(int pack_no)
{
    int packsize;
    struct icmp *icmp;
    struct timeval *tval;
    icmp = (struct icmp*)sendpacket;
    icmp->icmp_type = ICMP_ECHO; //ICMP_ECHO类型的类型号为0
    icmp->icmp_code = 0;
    icmp->icmp_cksum = 0;
    icmp->icmp_seq = pack_no;    //发送的数据报编号
    icmp->icmp_id = pid;

    packsize = 8 + datalen;     //数据报大小为64字节
    tval = (struct timeval *)icmp->icmp_data;
    gettimeofday(tval,NULL);        //记录发送时间
    //校验算法
    icmp->icmp_cksum =  cal_chksum((unsigned short *)icmp,packsize);
    return packsize;
}

/****发送三个ICMP报文****/
int  send_packet()
{
    int packetsize;


    packetsize = pack(0);   //设置ICMP报头
    //发送数据报
    if(sendto(sockfd,sendpacket,packetsize,0,
        (struct sockaddr *)&dest_addr,sizeof(dest_addr)) < 0)
    {
        perror("sendto error");
        return -1;
    }


    return 0;
}


/******剥去ICMP报头******/
int unpack(char *buf,int len)
{
    //printf("%s\n",__func__);
    //int i;
    int iphdrlen;       //ip头长度
    struct ip *ip;
    struct icmp *icmp;
    struct timeval *tvsend;
    double rtt;


    ip = (struct ip *)buf;
    iphdrlen = ip->ip_hl << 2; //求IP报文头长度，即IP报头长度乘4
    icmp = (struct icmp *)(buf + iphdrlen); //越过IP头，指向ICMP报头
    len -= iphdrlen;    //ICMP报头及数据报的总长度
    if(len < 8)      //小于ICMP报头的长度则不合理
    {
        printf("ICMP packet\'s length is less than 8\n");
        return -1;
    }
    //确保所接收的是所发的ICMP的回应
    if((icmp->icmp_type == ICMP_ECHOREPLY) && (icmp->icmp_id == pid))
    {
        tvsend = (struct timeval *)icmp->icmp_data;
        tv_sub(&tvrecv,tvsend); //接收和发送的时间差
        //以毫秒为单位计算rtt
        rtt = tvrecv.tv_sec*1000 + tvrecv.tv_usec/1000;
        temp_rtt[0] = rtt;
        all_time += rtt;    //总时间
        //显示相关的信息
        printf("%d bytes from %s: icmp_seq=%u ttl=%d time=%.1f ms\n",
                len,inet_ntoa(from.sin_addr),
                icmp->icmp_seq,ip->ip_ttl,rtt);
        return 0;
    }else{
        //printf("recv is wronge!\n");
        return -1;
    }
}




/****接受所有ICMP报文****/
int recv_packet()
{
    int n,fromlen;
    //extern int error;
    fromlen = sizeof(from);

    //接收数据报
    if((n = recvfrom(sockfd,recvpacket,sizeof(recvpacket),0,(struct sockaddr *)&from,&fromlen)) > 0)
    {
        //printf("rec something!\n");
        gettimeofday(&tvrecv,NULL);     //记录接收时间
        return unpack(recvpacket,n);       //剥去ICMP报头
    }


    return -1;
}


/*主函数*/
int netTest(char *p)
{
    struct hostent *host;
    struct protoent *protocol;
    //unsigned long inaddr = 0;
    int size = 50 * 1024;

    //不是ICMP协议
    if((protocol = getprotobyname("icmp")) == NULL)
    {
        perror("getprotobyname");
        return -1;
    }

    //生成使用ICMP的原始套接字，只有root才能生成
    if((sockfd = socket(AF_INET,SOCK_RAW,protocol->p_proto)) < 0)
    {
        perror("socket error");
        return -1;
    }


    /*扩大套接字的接收缓存区导50K，这样做是为了减小接收缓存区溢出的
      可能性，若无意中ping一个广播地址或多播地址，将会引来大量的应答*/
    setsockopt(sockfd,SOL_SOCKET,SO_RCVBUF,&size,sizeof(size));
    bzero(&dest_addr,sizeof(dest_addr));    //初始化
    dest_addr.sin_family = AF_INET;     //套接字域是AF_INET(网络套接字)

    //判断主机名是否是IP地址
    if(inet_addr(p) == INADDR_NONE)
    {
        if((host = gethostbyname(p)) == NULL) //是主机名
        {
            perror("gethostbyname error");
            goto fail;
        }
        memcpy((char *)&dest_addr.sin_addr,host->h_addr,host->h_length);
    }
    else{ //是IP 地址
        dest_addr.sin_addr.s_addr = inet_addr(p);
    }
    pid = getpid();
    printf("PING %s(%s):%d bytes of data.\n",p,
            inet_ntoa(dest_addr.sin_addr),datalen);

    if(send_packet() < 0){      //发送ICMP报文
        goto fail;
    }

    if(recv_packet() == 0){      //接收ICMP报文
        printf("The network ok!\n");
        close(sockfd);
        return 0;
    }

fail:
    close(sockfd);
    printf("the network not ok!\n");
    return -1;
}




int getIp(char *eth, char *ip){
	int sockfd;
    struct ifreq tmp;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if( sockfd < 0)
    {
        printf("create getIp socket fail\n");
        return -1;
    }
    memset(&tmp,0,sizeof(struct ifreq));
    strncpy(tmp.ifr_name,eth,sizeof(tmp.ifr_name)-1);

	if (ioctl(sockfd, SIOCGIFADDR, &tmp) < 0)
    {
    //	printf("eth ip ioctl error\n");
		close(sockfd);
		return -1;
    }
	struct sockaddr_in *ptr;
    ptr = (struct sockaddr_in *) &tmp.ifr_ifru.ifru_addr;
	strcpy(ip,inet_ntoa(ptr->sin_addr));
	close(sockfd);
	return 0;
}


int setIp(char *eth, const char *ip){
    char com[64];
    sprintf(com,"ifconfig %s %s",eth,ip);
    return system(com);
}

int eth0Test(const char *server){
    system("ifconfig wlan0 down");
    int ret = netTest(server);
    system("ifconfig wlan0 up");
    return ret;
}

int wifiTest(const char *server){
    system("ifconfig eth0 down");
    int ret = netTest(server);
    system("ifconfig eth0 up");
    return ret;
}
