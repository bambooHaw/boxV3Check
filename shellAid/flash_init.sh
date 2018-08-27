#!/bin/bash

#All copyright reserved, but will obey GPL V2.0
#V1.0 2018.07.02 	#by Hensen |  first write
#V2.0 2018.07.31	#by Hensen |  involved in syslogd management

#func:
# 该脚本在boxV3CheckApp进行烧录检测的起始阶段会被调用，以完成盒子3上spi-flash的格式化操作
#1. init spi-flash in boxv3 which uses mtd subsystem of Linux
#2. Notes: 
# 		Might involved in syslogd's stop and start


#syslogd management config. 
#Notes:
#	1. These configs must be as same as /etc/inittab file's if there are some.
#	2. These configs might be used or not, it depends on if syslogd managed by /etc/inittab file with "sysinit::respawn" property
syslogdRotateSize=800
syslogdRotateCnt=4

#flash init config
flagFileName=README.txt
#4k
pageSize=0x1000
#256k
blockSize=0x40000
#2M
partSize=0x200000
partitionInfo="NorFlash-p"
timesCntFlashcp=3

#1. operate all flash node 
flashPartsNum=$(cat /proc/mtd | grep ${partitionInfo} |cut -d ":" -f0 | awk '{print NR}' | tail -1)
#1) get part sign num, eg: /dev/mtd0 -> 0
tmpString=$(cat /proc/mtd | grep ${partitionInfo} |cut -d ":" -f0 | awk 'NR==1' | cut -d 'd' -f2)
if [ ! -n "$1" ]; then 
	echo "Usage: $0 + partition's Num(eg: $0 0)."
	exit 64
fi
partitionSign=$1
if [ $1 -gt ${flashPartsNum} ]; then
	echo "Warning: the max flashPartsNum is ${flashPartsNum}."
	exit 64
fi

blockNodeName=mtdblock${partitionSign}
cdevNode=/dev/mtd${partitionSign}
blockNode=/dev/mtdblock${partitionSign}
mountPath=/mnt/mtdblock${partitionSign}
#2) check if the blockNode is existed
existName=$(ls -l /dev/ | grep "^b" | grep ${blockNodeName})
if [ ! -n "${existName}" ]; then
	echo "error: ${nodePath} doesn't exist! Or it's not a block device!"
	exit 74
else
	#3) mkfs the flash 
	echo "creating jffs2 fileSystem on ${cdevNode}"
	#We could wait until cmd exec is over in this way
	output=$(rm -rf myDir-*)
	for ((i=0; i<${timesCntFlashcp}; i++)); do
		tmpString=myDir-$(date "+%Y-%m-%d[%H:%M]")
		output=$(mkdir ${tmpString})
		output=$(touch ${tmpString}/${flagFileName})
		output=$(date "+%Y-%m-%d[%H:%M]" > ${tmpString}/${flagFileName})
		output=$(echo "System just mkfs.jffs2 for this partition." >> ${tmpString}/${flagFileName})
		output=$(sync)
		
		output=$(mkfs.jffs2  -s ${pageSize} -e ${blockSize} -p ${partSize} -d ./${tmpString} -o  jffs2.img)
		output=$(sync)
		output=$(rm -rf ${tmpString})
		echo "prepare env for flash init"
		output=$(killall -9 syslogd)
		output=$(umount ${blockNode})
		echo "erase all ${cdevNode}..."
		output=$(flash_eraseall ${cdevNode})
		echo "done!"
		output=$(sync)
		echo "make jffs2 img for ${cdevNode}..."
		output=$(flashcp jffs2.img ${cdevNode})
		echo "done!"
		output=$(sync)
		output=$(rm jffs2.img)
		output=$(sync)
		#4) test for the flash
		output=$(umount ${mountPath})
		output=$(sync)
		output=$(rm -rf ${mountPath})
		output=$(sync)
		output=$(mkdir -p ${mountPath})
		output=$(sync)
		output=$(mount -t jffs2 ${blockNode} ${mountPath})
		echo "resume the sys services..."
		tmpString=$(ps ajx | grep syslogd | grep -v grep)
		if [ ! -n "${tmpString}" ]; then
			echo "attempt resume syslogd in manual"
			output=$(syslogd -n -s ${syslogdRotateSize} -b ${syslogdRotateCnt} &)
		fi
		#4.1) if the mtdblock* mounted with a jffs2 img
		tmpString=$(mount | grep ${blockNode} | grep ${mountPath})
		if [ ! -n "${tmpString}" ]; then
			echo "warning: ${blockNode} can't be mounted on ${mountPath}, try again ..."
			continue
		else
			#4.2) if the flash made jffs2 success
			tmpString=$(cat ${mountPath}/${flagFileName})
			if [ ! -n "${tmpString}" ]; then 
				echo "warning: Can't access jffs2 file system on ${mountPath}, try again ..."
				continue
			else 
				break
			fi
		fi
	done
	#check again
	#4.1`) if the mtdblock* mounted with a jffs2 img
	tmpString=$(mount | grep ${blockNode} | grep ${mountPath})
	if [ ! -n "${tmpString}" ]; then
		echo "error: ${blockNode} can't be mounted on ${mountPath}"
		exit 69
	else
		#4.2`) if the flash made jffs2 success
		tmpString=$(cat ${mountPath}/${flagFileName})
		if [ ! -n "${tmpString}" ]; then 
			echo "error: Can't access jffs2 fileSystem on ${mountPath}!"
			exit 71
		else 
			echo "created jffs2 fileSystem on ${cdevNode}: success"
			exit 0
		fi
	fi
fi