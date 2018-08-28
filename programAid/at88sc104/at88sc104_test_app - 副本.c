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
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <strings.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define AT88SC104_BUF_LEGTH	64
//#define OPT_DIR_READ	"get"
#define OPT_DIR_WRITE	"set"
//speak start: you should modified this depend on specific driver program
#define AT88_GET_STATUS 0
#define AT88_SET_PASSWD 1
#define AT88_GET_MFandLOT_INFO 2
#define AT88_BURN_FUSE 4
#define AT88_SET_BLK 5
//speak end
#define OPT_BYTES_CNT 2
#define AT88SC104_NODE_NAME "/dev/crypt"
#define USR_ZONE_MAX_LENGTH 32


void show_buf(char* buf, int len){
	if(!buf){
		printf("Wrong argument to show!\n");
		return;
	}
	int i = 0;
	for(i=0; i<len; i++){
		printf("%#x ", buf[i]);
	}
	printf("\n");
}

int main(int argc, char* argv[])
{

	int ret;
	
	char i=0, buf[USR_ZONE_MAX_LENGTH] = {};
	unsigned char memory_num = 0;
	char *endstr;
    int cmd=0;
	int fd = open(AT88SC104_NODE_NAME, O_RDWR);
	if (fd < 0)
	{
		printf("you must to check the erro num in the source file!\n");
		return -1;
	}


#if 0

	bzero(buf, sizeof(buf));
	ret = ioctl(fd, 18690, (unsigned long)buf);
	if(ret < 0)
	{
		printf("AT88_GET_MFandLOT_INFO failed, ret = %d\n", ret);
	}
	printf("Get mfAndLot:");
	show_buf(buf, 64);
#endif

#if 1

	if(argc <= 2)
	{
		printf("usage: optAt88 <set|get> [MemoryNum]\n");
		return -1;
	}
	//16 stands for hex mode num
	memory_num = strtoul(argv[2],&endstr,16);
	if(!strcmp(argv[1], OPT_DIR_WRITE)) cmd = 1;
	else cmd = 0;
	printf("cmd:%d, memory_num:%d\n", cmd, memory_num);

	
	ret = ioctl(fd, 5, memory_num);
	if(ret < 0)
	{
		printf("set num error,ret = %d\n",ret);
	}
	//read
	if(!cmd){
		bzero(buf, sizeof(buf));
		ret = read(fd, buf, USR_ZONE_MAX_LENGTH);
		if(ret<0)
		{
			printf("%s %s %d:read error\n",__FILE__,__FUNCTION__,__LINE__);
			close(fd);
			return -1;
		}
		printf("usr%d_zone:", memory_num);
		show_buf(buf, USR_ZONE_MAX_LENGTH);
	}else{
		//write
		bzero(buf, sizeof(buf));
		if(argc < 4){
			//strncpy(buf, "123abc123", 12);
		}
		else 
			strncpy(buf, argv[3], (strlen(argv[3]) > sizeof(buf))? sizeof(buf):strlen(argv[3]));
		printf("write       buf:");
		show_buf(buf, USR_ZONE_MAX_LENGTH);
		ret = write(fd, buf, USR_ZONE_MAX_LENGTH);
		if(ret<0)
		{
			printf("%s %s %d:write error\n",__FILE__,__FUNCTION__,__LINE__);
			close(fd);
			return -1;
		}
	}

#endif 
	
	close(fd);
    return 0;
}
