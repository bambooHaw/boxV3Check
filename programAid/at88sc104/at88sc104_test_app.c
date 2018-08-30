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
#define BOXV3_UI8MAC_LENGTH 8
#define BOXV3_UI8MAC_ACT_LENGTH 6
#define INPUT_MAC_LENGTH	17
#define AT88SC104_NODE_NAME "/dev/crypt"



void show_buf(char* buf, int index, int len){
	if(!buf){
		return;
	}
	int i = 0;
	for(i=index; i<index+len; i++){
		printf("%c", buf[i]);
	}
	printf("\n");
}

void changeMacToNum(char *dst, char *src)
{
    char *p = strtok(src, ":");
    int i = 0;

	
    bzero(dst, BOXV3_UI8MAC_LENGTH);

    //printf("mac: ");
    while (NULL != p)
    {
        dst[i++] = strtoul(p, NULL, 16);
        //printf("mac[%d]: %#x", i-1, dst[i-1]);
        p = strtok(NULL, ":");
    }
	//puts("");
}

static char *utoa(unsigned int value, char *string, int radix)
{
	char stack[16];
	int  i;
	int  j;
	char digit_string[] = "0123456789ABCDEF";	
	
	if(value == 0)
	{
		//zero
		string[0] = '0';
		string[1] = '\0';
		return string;
	}
	
	for(i = 0; value > 0; ++i)
	{
		// characters in reverse order are put in 'stack'.
		stack[i] = digit_string[value % radix];
		value /= radix;
	}
	
	//restore reversed order result to user string
    for(--i, j = 0; i >= 0; --i, ++j)
	{
		string[j] = stack[i];
	}
	//must end with '\0'.
	string[j] = '\0';
	
	return string;
}


void changeNumToMac(char* dst, const char* src){
	volatile int i = 0, j=0;
	char string[32] = {};
	for(i=0; i<BOXV3_UI8MAC_ACT_LENGTH; i++){
		bzero(string, sizeof(string));
		utoa((unsigned int)(src[i]), string, 16);
		if('\0' == string[1]){
			string[1] = string[0];
			string[0] = '0';
		}
		memcpy(dst + j, string, 2);
		j = j + 2;
		if(i<BOXV3_UI8MAC_ACT_LENGTH -1){
			memcpy(dst+j, ":", 1);
			j = j + 1;
		}
	}
	dst[j] = '\0';
}



int main(int argc, char* argv[])
{

	int ret;
	
	char i=0, buf[USR_ZONE_MAX_LENGTH] = {};
	unsigned char memory_num = 0, mac_num = 0;
	char macIndex[8] = {};
	char macAct[18] = {};
	unsigned long object_code = 0;
	char *endstr;
    int cmd=0, opt=0;
	int fd = open(AT88SC104_NODE_NAME, O_RDWR);
	if (fd < 0)
	{
		fprintf(stderr, "Error: open(AT88SC104_NODE_NAME, O_RDWR) failed!\n");
		return -EIO;
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

	if(argc < 3)
	{
		fprintf(stdout, "usage: optAt88 <set|get> [optCode]\n");
		close(fd);
		return -EINVAL;
	}
	//16 stands for hex mode num
	object_code = strtoul(argv[2], &endstr, 16);
	if(!strcmp(argv[1], OPT_DIR_WRITE)) cmd = 1;
	else cmd = 0;

	memory_num = object_code & 0xf;
	if(object_code & 0x20) opt = 1;
	else opt = 0; 	//read only access

	
	ret = ioctl(fd, AT88_SET_BLK, memory_num);
	if(ret < 0)
	{
		fprintf(stderr, "Error: ioctl(fd, AT88_SET_BLK, memory_num) failed(ret = %d)!\n", ret);
		close(fd);
	    return ret;
	}
	if(!cmd){
		//read
		bzero(buf, sizeof(buf));
		ret = read(fd, buf, USR_ZONE_MAX_LENGTH);
		if(ret<0)
		{
			fprintf(stderr, "Error: read(fd, buf, USR_ZONE_MAX_LENGTH) failed!\n");
			close(fd);
			return ret;
		}
		switch(memory_num){
		case 0:
			printf("SerialNum ");
			show_buf(buf, 0, 12);
			break;
		case 1:
			printf("P2PID ");
			show_buf(buf, 0, 16);
			break;
		case 2:
			printf("License ");
			show_buf(buf, 0, 32);
			break;
		case 3:{	
			mac_num = ((object_code>>8) & 0xf);
			switch(mac_num){
			case 1:
				printf("MacLan ");
				changeNumToMac(macAct, buf);
				show_buf(macAct, 0, 17);
				break;
			case 2:
				printf("MacWan ");
				changeNumToMac(macAct, buf+8);
				show_buf(macAct, 0, 17);
				break;
			case 3:
				printf("Mac4G ");
				changeNumToMac(macAct, buf+16);
				show_buf(macAct, 0, 17);
				break;
			case 4:
				printf("MacWifi ");
				changeNumToMac(macAct, buf+24);
				show_buf(macAct, 0, 17);
				break;
			default:
				break;
			}
			break;
		}
		default:
			break;
		}
		//recorder the answer
	}else{
		//write
		if(!opt){
			fprintf(stderr, "Error: Read only access!");
			return -EINVAL;
		}
		bzero(buf, sizeof(buf));
		if(argc < 4){
			//strncpy(buf, "123abc123", 12);
		}else
		{
		//	strncpy(buf, argv[3], (strlen(argv[3]) > sizeof(buf))? sizeof(buf):strlen(argv[3]));
			bzero(buf, sizeof(buf));
			ret = read(fd, buf, USR_ZONE_MAX_LENGTH);
			switch(memory_num){
			case 0:
				memcpy(buf, argv[3], 12);
				break;
			case 1:
				memcpy(buf, argv[3], 16);
				break;
			case 2:
				memcpy(buf, argv[3], 32);
				break;
			case 3:{	
				mac_num = ((object_code>>8) & 0xf);
				changeMacToNum(macIndex, argv[3]);
				switch(mac_num){
				case 1:
					memcpy(buf, macIndex, 6);
					memset(buf+6, 0, 2);
					break;
				case 2:
					memcpy(buf+8, macIndex, 6);
					memset(buf+6+8, 0, 2);
					break;
				case 3:
					memcpy(buf+16, macIndex, 6);
					memset(buf+6+16, 0, 2);
					break;
				case 4:
					memcpy(buf+24, macIndex, 6);
					memset(buf+6+24, 0, 2);
					break;
				default:
					break;
				}
				break;
			}
			default:
				break;
			}
		}
		
		ret = write(fd, buf, USR_ZONE_MAX_LENGTH);
		if(ret<0)
		{
			fprintf(stderr, "%s %s %d:write error\n",__FILE__,__FUNCTION__,__LINE__);
			close(fd);
			return ret;
		}
	}

#endif 
	
	close(fd);
    return 0;
}
