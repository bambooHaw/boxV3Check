#ifndef UDPBROADCAST
#define UDPBROADCAST

#include <QHostInfo>
#include <QNetworkInterface>
#include <QUdpSocket>
#include <arpa/inet.h>
#include <QDebug>


#define UDP_RECV_SRC_PORT_NUMBER 58599
#define UDP_SEND_SRC_PORT_NUMBER 7000
#define UDP_MAIN_PORT_NUMBER 10106

#define UDP_SEND_CNT_MAX 1

/*
 * udp server cmd = maincode << 8 | subcode
*/
#define dataReceiveOk 0x01
#define ui16MainCodeReqWifiIp 0x01
#define ui32FBCodeRecvWifiIp 0x01

typedef struct tagStDeviceScanInfo
{
    unsigned int ui32Prefix;		//起始字节 固定值为
    unsigned int ui32Version;	//版本信息 固定值
    unsigned int ui32MsgLength;//指令长度，包括所有(头尾等信息)
    unsigned int ui32SourceIP;//源IP（扫描不需要填写，配置需填写PC机IP）
    unsigned int ui32SendIP;	//目的IP （扫描填写PC机IP；配置需填写设备IP）
    unsigned short ui16MainCode;	//主指令
    unsigned short ui16SubCode;	//次指令
    unsigned int ui32FBCode;//反馈
    unsigned char  i8SvrName[16];		//设备名称“HW-IPNC”固定
    unsigned char i8Domain[16];		//使用领域 “IPNC”固定
    unsigned char i8SoftVersion[64];		//软件版本号，以\0结束字符串
    unsigned int ui32NetStatus;			//设备物理网络状态 0x01连接 0xFF 未连接
    unsigned int	ui32MagicNum;//幻数
    unsigned int	ui32NetVersion;//网络配置版本
    unsigned int	ui32IP;//设备IP
    unsigned int	ui32NetMask;//子网掩码
    unsigned int	ui32Dns; //DNS
    unsigned int	ui32GateWay;//网关
    unsigned char	ui8Mac[8];//MAC地址
    char		i8DeviceCode[32];//设备编号
    char		i8Reserve[64];//预留
    unsigned int 	ui32CheckNum;//校验
    char 	i8FixAddr[64];//安装地点
    char 	i8FixDirection[64];//安装方向
    char 	i8UserDefCode[64];//用户自定义编码
    char 	i8UserDefInfo[64];//用户自定义描述
    unsigned int ui32Verify;			//校验 对目的＋源＋netconfigure模加
    unsigned int ui32Postfix;		//结束字节固定值
}udpMsg_t;

#endif // UDPBROADCAST

