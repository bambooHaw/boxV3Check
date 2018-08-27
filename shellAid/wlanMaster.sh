#!/bin/bash
#Please put this file in the /opt directory
#version 6.0
#start wifi sta,got module&link&ip state
#
maxTryCnt=10
flagpath=/tmp/wlan0
wifinterface=wlan0
ssid=`cat /etc/wpa_supplicant.conf | grep 'ssid' | awk  -F "=\"" '{print $2}' | awk -F "\"" '{print $1}'`>/dev/null

# make sure Interface 'wlan0' was added
for ((i=0; i<${maxTryCnt}; i++)); do
	#0. prepare the tmp info dir
	output=$(rm -rf ${flagpath})
	if [[ ! -d ${flagpath} ]]; then
		output=$(mkdir -p ${flagpath})
	fi
	`echo "start">${flagpath}/module`
	`echo "start">${flagpath}/link`
	`echo "start">${flagpath}/ip`
	`echo "start">${flagpath}/linkinfo`
	`echo "start">${flagpath}/ipinfo`
	`echo "start">${flagpath}/siginfo`
	
	#1. prepare the env
	output=$(killall -9 udhcpc >> /dev/null)
	output=$(killall -9 hostapd >> /dev/null)
	output=$(killall -9 wpa_supplicant >> /dev/null)
	output=$(ifconfig br0 down >> /dev/null)
	output=$(brctl delif br0 wlan0 >> /dev/null)
	output=$(brctl delbr br0 >> /dev/null)
	#must down the interface first, to make sure wpa_supplicant run in normal
	output=$(ifconfig ${wifinterface} down)
	#added configged the interface, auto up the interface in background
	wpa_supplicant -i ${wifinterface} -c /etc/wpa_supplicant.conf -B
	for ((j=0; j<3; j++)); do 
		sleep 3
		#if ifconfig ${wifinterface} | grep "RUNNING">/dev/null;then
		if ifconfig ${wifinterface} >/dev/null;then
			echo "OK">${flagpath}/module
			echo "module is OK!"
		else
			echo "Error">${flagpath}/module
			echo "module is bad"
			let "j+=1"
			continue
		fi
		if iwconfig ${wifinterface} | grep ${ssid}>/dev/null;then
			echo "OK">${flagpath}/link
			echo "link is OK!"
			iwconfig ${wifinterface} | grep 'Signal level' | awk  -F "=" '{print $2}' | awk -F " " '{print $1}' >${flagpath}/linkinfo
			iwconfig ${wifinterface} | grep 'Signal level' | awk  -F "=" '{print $3}' | awk -F " " '{print $1}' >${flagpath}/siginfo
			break
		else
			echo "Error">${flagpath}/link
			echo "link is bad"
		fi
	done
	output=$(killall -9 wpa_supplicant >> /dev/null)
	
	output=$(udhcpc -i ${wifinterface} -n -q -t 2 -A 1)
	
	if ifconfig ${wifinterface} | grep "inet addr">/dev/null;then
		echo "OK">${flagpath}/ip
		echo "ip is OK!"
		ifconfig ${wifinterface} | grep 'inet addr' | awk  -F ":" '{print $2}' | awk -F " " '{print $1}' >${flagpath}/ipinfo
		iwconfig ${wifinterface} | grep 'Signal level' | awk  -F "=" '{print $2}' | awk -F " " '{print $1}' >>${flagpath}/linkinfo
		iwconfig ${wifinterface} | grep 'Signal level' | awk  -F "=" '{print $3}' | awk -F " " '{print $1}' >>${flagpath}/siginfo
		break
	else
		echo "Error">${flagpath}/ip
		echo "ip is bad"
		iwconfig ${wifinterface} | grep 'Signal level' | awk  -F "=" '{print $2}' | awk -F " " '{print $1}' >>${flagpath}/linkinfo
		iwconfig ${wifinterface} | grep 'Signal level' | awk  -F "=" '{print $3}' | awk -F " " '{print $1}' >>${flagpath}/siginfo
	fi	
done
