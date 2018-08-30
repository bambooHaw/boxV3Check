#ifndef __AT88SC0104_IOCTL_H__
#define __AT88SC0104_IOCTL_H__


#define USR_ZONE_MAX_LENGTH 32

typedef struct _AT88SC_IOCTL_ARG{
	char name[USR_ZONE_MAX_LENGTH];
	char addr;
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


#define AT88_GET_COMMON_INFO 	_IOR(AT88_IOCTL_MAGIC, 20, at88sc_ioctl_arg_t)

#endif