#!/bin/bash
#V1.0 20180702 @by Henry | first write this
#V1.1 20180713 @by Henry | modified for all mtdblock* | debug for the dead loop all the way

cnodePath=/dev/mtd1
nodePath=/dev/mtdblock1
nodeName=mtdblock1
cnodeName=mtd1
mountPath=/mnt/mtdblock1
maxPercent=80
fileNum=$(ls -lR ${mountPath}|grep "^-" |wc -l)

for ((l=0; l<1; l++)); do

	#1. if the ${nodeName} is existed
	existName=$(ls -l /dev/ | grep "^b" | grep ${nodeName})
	if [ -n "${existName}" ]; then

		#2. if the ${nodeName} is mounted
		mountStatus=$(mount | grep ${nodePath} | grep ${mountPath})
		if [ -n "${mountStatus}" ]; then

			#echo "${mountPath}'s fileNum:${fileNum}"
			usedPercent=$(df -h | grep ${nodeName}| awk '{print $5}' | cut -d '%' -f 1)
			usedSize=$(du -sh ${mountPath}| awk '{print $1}')
			totalSize=$(mtd_debug info ${cnodePath} | grep mtd.size | awk '{print $4}')
			echo "$nodePath was used $usedPercent% storage space[fileNum:${fileNum}, fileSize:$usedSize, total${totalSize}]."
			if [ ${usedPercent} -gt ${maxPercent} ]; then
				echo "${mountPath} storage space is unhealthy[over ${maxPercent}%]!"
				for ((i=0; i<${fileNum}; i++)); do
					#echo "i:${i}"
					if [ ${usedPercent} -gt ${maxPercent} ]; then
						echo "Deleting dir(${nodePath})'s oldest file:"
						ls -rt ${mountPath} |head -1
						find ${mountPath}/* |head -1|xargs rm -rf
						#get the lastest file numbers
						fileNum=$(ls -lR ${mountPath}|grep "^-" |wc -l)
						usedPercent=$(df -h | grep ${nodeName}| awk '{print $5}' | cut -d '%' -f 1)
						usedSize=$(du -sh ${mountPath}| awk '{print $1}')
						totalSize=$(mtd_debug info ${cnodePath} | grep mtd.size | awk '{print $4}')
						echo "$nodePath was used $usedPercent% storage space[fileNum:${fileNum}, fileSize:$usedSize, total${totalSize}] now."
					else
						#echo "${mountPath} storage space is healthy!"
						break
					fi
				done
			fi
		else
			echo "Warning: ${nodePath} hasn't been mounted on ${mountPath}"
			echo "Trying: mounting ${nodePath} on ${mountPath} once..."
			mkdir -p ${mountPath}
			mount -t jffs2 ${nodePath} ${mountPath}
			continue

:<<comments0
#3. if the dir ${mountPath} is exist
if [ -d ${mountPath} ]; then
curDirSize=$(du -sh ${mountPath}| awk '{print $1}')
echo "dir(${mountPath})'s size: ${curDirSize}"
else
echo "Error: dir(${mountPath}) doesn't exist!"
fi
comments0
		fi

	else
		echo "Error: ${nodePath} doesn't exist! Or it's not a block device!"
	fi

done