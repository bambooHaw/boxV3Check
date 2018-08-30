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
#include <sys/ioctl.h>

#include "at88sc0104_ioctl.h"
//#define OPT_DIR_READ	"get"
#define OPT_DIR_WRITE	"set"
#define AT88SC104_NODE_NAME "/dev/crypt"


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

	int ret = -1, fd = -1;
	unsigned int cmd = 2;
	at88sc_ioctl_arg_t msg = {
		.name = "General ",
	};


	if(argc < 3)
	{
		printf("usage: optAt88 <set|get> [wantYouWant]\n");
		ret = -EINVAL;
		return ret;
	}else
	{
		if((strlen(argv[1]) + strlen(argv[2]) + strlen(msg.name)) > sizeof(msg.name))
		{
			ret = -EINVAL;
			return ret;
		}else
		{

			if(!strcmp(OPT_DIR_WRITE, argv[1]))
			{
				if(argc < 4)
				{
					printf("usage: optAt88 <set> [wantYouWant] [value]\n");
					ret = -EINVAL;
					return ret;
				}else
				{
					if(strlen(argv[3]) > USR_ZONE_MAX_LENGTH)
						strncpy(msg.buf, argv[3], USR_ZONE_MAX_LENGTH);
					else 
						strcpy(msg.buf, argv[3]);

					//test
					printf("msg.buf:");
					show_buf(msg.buf, 32);
				}
			}

		
			strcat(msg.name, argv[1]);
			strcat(msg.name, "s ");
			strcat(msg.name, argv[2]);


			if(!strcmp("mfAndLot", argv[2]))
			{
				cmd = AT88_GET_MFandLOT_INFO;
			}
			else if(!strcmp("serialNum", argv[2]))
			{
				if(strcmp(OPT_DIR_WRITE, argv[1])) cmd = AT88_SET_SERIAL_NUM;
				else cmd = AT88_GET_SERIAL_NUM;
				msg.usrZoneNum = 0;
				msg.index = 0;
				msg.length = 12;
			}else if(!strcmp("p2pId", argv[2]))
			{
				if(strcmp(OPT_DIR_WRITE, argv[1])) cmd = AT88_SET_P2P_ID;
				else cmd = AT88_GET_P2P_ID;
				msg.usrZoneNum = 1;
				msg.index = 0;
				msg.length = 16;
			}else if(!strcmp("license", argv[2]))
			{
				if(strcmp(OPT_DIR_WRITE, argv[1])) cmd = AT88_SET_LICENSE;
				else cmd = AT88_GET_LICENSE;
				msg.usrZoneNum = 2;
				msg.index = 0;
				msg.length = 32;
			}else if(!strcmp("macLan", argv[2]))
			{
				if(strcmp(OPT_DIR_WRITE, argv[1])) cmd = AT88_SET_MAC;
				else cmd = AT88_GET_MAC;
				msg.usrZoneNum = 3;
				msg.index = 0;
				msg.length = 8;
			}else if(!strcmp("macWan", argv[2]))
			{
				if(strcmp(OPT_DIR_WRITE, argv[1])) cmd = AT88_SET_MAC;
				else cmd = AT88_GET_MAC;
				msg.usrZoneNum = 3;
				msg.index = 8;
				msg.length = 8;
			}else if(!strcmp("mac4g", argv[2]))
			{
				if(strcmp(OPT_DIR_WRITE, argv[1])) cmd = AT88_SET_MAC;
				else cmd = AT88_GET_MAC;
				msg.usrZoneNum = 3;
				msg.index = 16;
				msg.length = 8;
			}else if(!strcmp("macWifi", argv[2]))
			{
				if(strcmp(OPT_DIR_WRITE, argv[1]))
				{
					cmd = AT88_SET_MAC;
				}
				else cmd = AT88_GET_MAC;
				msg.usrZoneNum = 3;
				msg.index = 24;
				msg.length = 8;
			}
		}
	}


	
	fd = open(AT88SC104_NODE_NAME, O_RDWR);
	if (fd < 0)
	{
		printf("Error: open at88sc failed!\n");
		ret = fd;
		return ret;
	}
	printf("ioctl msg.name: %s\n", msg.name);

	ret = ioctl(fd, cmd, &msg);
	printf("ioctl msg.buf:");
	show_buf(msg.buf, msg.length);

	close(fd);
    return ret;
}
