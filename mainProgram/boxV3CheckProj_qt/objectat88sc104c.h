#ifndef OBJECTAT88SC104C_H
#define OBJECTAT88SC104C_H
/*
 * SHUZIZHITONG BoxV3 AT88SC104 testing utility
 *
 * Copyright (c) 2018	Henry <haoxiansen@zhitongits.com.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * Cross-compile with  arm-none-linux-gnueabi-gcc 4.8.1
 */
#include <QObject>
#include <QDir>
#include <QDebug>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <strings.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "boxv3.h"

#define AT88SC104_BUF_LEGTH	32
//#define OPT_DIR_READ	"get"
#define OPT_DIR_WRITE	"set"
//speak start: you should modified this depend on specific driver program
#define USR_ZONE_MAX_LENGTH 32
typedef struct _AT88SC_IOCTL_ARG{
    char name[USR_ZONE_MAX_LENGTH];
    char usrZoneNum;
    char index;
    char length;
    char buf[USR_ZONE_MAX_LENGTH];
    void* private_data;
}at88sc_ioctl_arg_t;

#define AT88_GET_STATUS 0
#define AT88_SET_PASSWD 1

#define AT88_BURN_FUSE 4
#define AT88_SET_BLK 5
#define AT88_IOCTL_MAGIC 'I'
#define AT88_GET_MFandLOT_INFO 	_IOR(AT88_IOCTL_MAGIC, 1, at88sc_ioctl_arg_t)

#define AT88_GET_SERIAL_NUM		_IOR(AT88_IOCTL_MAGIC, 2, at88sc_ioctl_arg_t)
#define AT88_GET_LICENSE		_IOR(AT88_IOCTL_MAGIC, 3, at88sc_ioctl_arg_t)
#define AT88_GET_P2P_ID			_IOR(AT88_IOCTL_MAGIC, 4, at88sc_ioctl_arg_t)
#define AT88_GET_MAC			_IOR(AT88_IOCTL_MAGIC, 5, at88sc_ioctl_arg_t)
#define AT88_GET_USER_ZONE		_IOR(AT88_IOCTL_MAGIC, 6, at88sc_ioctl_arg_t)

#define AT88_SET_SERIAL_NUM		_IOW(AT88_IOCTL_MAGIC, 12, at88sc_ioctl_arg_t)
#define AT88_SET_LICENSE		_IOW(AT88_IOCTL_MAGIC, 13, at88sc_ioctl_arg_t)
#define AT88_SET_P2P_ID			_IOW(AT88_IOCTL_MAGIC, 14, at88sc_ioctl_arg_t)
#define AT88_SET_MAC			_IOW(AT88_IOCTL_MAGIC, 15, at88sc_ioctl_arg_t)
#define AT88_SET_USER_ZONE		_IOW(AT88_IOCTL_MAGIC, 16, at88sc_ioctl_arg_t)


//speak end
#define OPT_BYTES_CNT 2
#define AT88SC104_NODE_NAME "/dev/crypt"
#define USRZONE_SERIAL_NUM_OFFSET 0
#define USRZONE_P2P_ID_OFFSET 0
#define SERIAL_NUMBER_LENGTH 12
#define P2P_ID_LENGTH 12
#define USR_ZONE_MAC_LENGTH 8
#define USRZONE_LAN_MAC_OFFSET  0
#define USRZONE_WAN_MAC_OFFSET  8
#define USRZONE_4G_MAC_OFFSET   16
#define USRZONE_WIFI_MAC_OFFSET 24
#define BOXV3_MAC_ADDR_LENGTH   6

//CHECK_FLAG
typedef struct _CHECK_FLAG{
    unsigned char production;
    unsigned char aging;
    unsigned char aged;
    unsigned char result;
    unsigned char admin;
    unsigned char i8Reserve[3];
}check_flag_t;

//
#define DEVICECODE_LENGTH   12
#define MAC_LENGTH  8
typedef struct _at88_usr_zone0
{
    unsigned char	i8DeviceCode[12];
    unsigned char	i8Reserve0[12];
    check_flag_t    boxV3CheckFlag;
}at88_usr_zone0_t;
typedef struct _at88_usr_zone3
{
    unsigned char	ui8MacWifi[8];
    unsigned char	ui8Mac4G[8];
    unsigned char	ui8MacLan[8];
    unsigned char	ui8MacWan[8];
}at88_usr_zone3_t;
typedef struct _cfg_info
{
    at88_usr_zone0_t usr_zone0;
    at88_usr_zone3_t usr_zone3;
}cfg_info_t;

QString getFirmwareName(void);

class objectAT88SC104C : public QObject
{
    Q_OBJECT
public:
    objectAT88SC104C(char* filePath);
    ~objectAT88SC104C();

    void showMfLotArray(void);
    void showBuf(char* buf, int len);

    char cmpWithCodesTables(char* src);
    char getMfAndLotInfo(void);
    char verfyPasswd(void);
    bool readUsrZone(char* zoneBuf, char zoneNum);
    bool writeUsrZone(char* zoneBuf, char zoneNum);
    bool getDeviceCode(char* deviceCode);
    bool setDeviceCode(char* deviceCode);
    bool getP2pId(char* p2pId);
    bool setP2pId(char* p2pId);
    bool getSpecificMac(char order, char* mac);
    bool setSpecificMac(char order, char* mac);

    char verifyCheckAccess(void);
    checkflag_enum getCheckStage(void);
    checkflag_enum clearCheckFlags(checkflag_enum currentCheckStage);

    checkflag_enum setCheckedFlags(checkflag_enum currentCheckStage);
signals:

public slots:
private:
    int fd;
    char mfLotArray[AT88SC104_BUF_LEGTH];
    QByteArray nodePath;
};

#endif // OBJECTAT88SC104C_H
