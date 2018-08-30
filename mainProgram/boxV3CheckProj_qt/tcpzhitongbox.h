#ifndef TCPTRANSCEIVER
#define TCPTRANSCEIVER

#include <QTcpSocket>
#include <QTcpServer>
#include <QNetworkInterface>
#include <QDebug>

#define TCP_PORT_NUMBER 17117

#define TCP_FINDHEAD_MAXCNT 3
#define TCP_CLIENT_READ 0x0
#define TCP_CLIENT_WRITE 0x1
typedef struct _tcpMsgGetLen{
    unsigned char chHeader[3];
    unsigned char chVersion;
    unsigned short ui16Length;
    _tcpMsgGetLen()
    {
        memset(chHeader,0,3);
        chVersion = 0;
        ui16Length = 0;
    }
}tcpMsgGetLen_t;

typedef struct _tcpMsgWithoutData{
    unsigned char chHeader[3];
    unsigned char chVersion;
    unsigned short ui16Length;
    unsigned short ui16SerialNO;
    unsigned short ui16FeekCode;
    unsigned char ucCmdType;
    unsigned char ucMainCmd;
    unsigned char ucSubCmd;
    unsigned char verifyCode;
    unsigned short ui16tail;
    _tcpMsgWithoutData()
    {
        memset(chHeader,0,3);
        chVersion = 0;
        ui16Length = 0;
        ui16SerialNO = 0;
        ui16FeekCode = 0;
        ucCmdType = 0;
        ucMainCmd = 0;
        ucSubCmd = 0;
        verifyCode = 0;
        ui16tail = 0;
    }
}tcpMsgWithoutData_t;

#define TCP_MSG_WITHOUT_DATA sizeof(tcpMsgWithoutData_t)

typedef struct _tcpMsgHaveData{
    unsigned char chHeader[3];
    unsigned char chVersion;
    unsigned short ui16Length;
    unsigned short ui16SerialNO;
    unsigned short ui16FeekCode;
    unsigned char ucCmdType;
    unsigned char ucMainCmd;
    unsigned char ucSubCmd;
    //unsigned long dataLen;
    unsigned char pData[1];   //contain of verifyCode ui16tail
    _tcpMsgHaveData()
    {
        memset(chHeader,0,3);
        chVersion = 0;
        ui16Length = 0;
        ui16SerialNO = 0;
        ui16FeekCode = 0;
        ucCmdType = 0;
        ucMainCmd = 0;
        ucSubCmd = 0;
        //dataLen = 0;
        pData[0] = 0;
    }
}tcpMsgHaveData_t;

#if 0
typedef struct _tcpMsgForLog{
    unsigned char chHeader[3];
    unsigned char chVersion;
    unsigned short ui16Length;
    unsigned short ui16SerialNO;
    unsigned short ui16FeekCode;
    unsigned char ucCmdType;
    unsigned char ucMainCmd;
    unsigned char ucSubCmd;
    unsigned int dataLen;
    unsigned char pData[1];   //contain of verifyCode ui16tail
    _tcpMsgForLog()
    {
        memset(chHeader,0,3);
        chVersion = 0;
        ui16Length = 0;
        ui16SerialNO = 0;
        ui16FeekCode = 0;
        ucCmdType = 0;
        ucMainCmd = 0;
        ucSubCmd = 0;
        dataLen = 0;
        pData[0] = 0;
    }
}tcpMsgForLog_t;
#endif

//设备反馈码
typedef enum
{
     DEVICEFEEKERRCODE_OK = 0x8000,		//正常
     DEVICEFEEKERRCODE_ERR = 0x8001,	//错误
     DEVICEFEEKERRCODE_PTRNULL = 0x8002,	//指针为空
     DEVICEFEEKERRCODE_UNSUPPORTED = 0x8003,	//不支持
     DEVICEFEEKERRCODE_VERSION = 0x8004,	//版本错误
     DEVICEFEEKERRCODE_LENGTH = 0x8005,	//长度错误
     DEVICEFEEKERRCODE_VERIFY =	0x8006,	//校验错误
     DEVICEFEEKERRCODE_CMDNOTFIND = 0x8007,//指令未找到
     DEVICEFEEKERRCODE_DATAFORMAT =	0x8008,		//数据格式错误
     DEVICEFEEKERRCODE_IPFORMAT	= 0x8009,	//IP格式错误
     DEVICEFEEKERRCODE_UPFLASHEARSE	= 0x800A,	//格式化错误
     DEVICEFEEKERRCODE_UPSERIAL	= 0x800B,	//序号错误，可能不连续
     DEVICEFEEKERRCODE_UPFLASHPROG = 0x800C,		//烧录失败
     DEVICEFEEKERRCODE_BUSY	= 0x800D,		//忙
     DEVICEFEEKERRCODE_SOCKETNUMFULL = 0x800E	//链接数已满
}DEVICEFEEKERRCODE;

#endif // TCPTRANSCEIVER

