#!/bin/sh
eth0out=1.1.1.1
eth1out=2.2.2.2
if [[ ! -d /tmp/eth0 ]]; then
	mkdir /tmp/eth0
fi
if [[ ! -d /tmp/eth1 ]]; then
	mkdir /tmp/eth1
fi
ifconfig eth0 192.168.112.1
ifconfig eth1 192.168.111.1
echo "Start">/tmp/eth0/send
echo "Listen">/tmp/eth0/receive
echo "Start">/tmp/eth1/send
echo "Listen">/tmp/eth1/receive
tcpdump -i eth0 > /tmp/eth0.log&
tcpdump -i eth1 > /tmp/eth1.log&
ping ${eth0out} -c 10 -I eth0&
ping ${eth1out} -c 10 -I eth1&

sleep 10

killall ping
killall tcpdump

cat /tmp/eth0.log | grep ${eth1out}
if [ $? -ne 0 ];then
	echo "eth0 receive error!"
	echo "Error">/tmp/eth0/receive
else
	echo "eth0 receive ok!"
	echo "OK">/tmp/eth0/receive
fi
cat /tmp/eth0.log | grep ${eth0out}
if [ $? -ne 0 ];then
	echo "eth0 send error!"
	echo "Error">/tmp/eth0/send
else
	echo "eth0 send ok!"
	echo "OK">/tmp/eth0/send
fi
cat /tmp/eth1.log | grep ${eth0out}
if [ $? -ne 0 ];then
	echo "eth1 receive error!"
	echo "Error">/tmp/eth1/receive
else
	echo "eth1 receive ok!"
	echo "OK">/tmp/eth1/receive
fi
cat /tmp/eth1.log | grep ${eth1out}
if [ $? -ne 0 ];then
	echo "eth0 send error!"
	echo "Error">/tmp/eth1/send
else
	echo "eth0 send ok!"
	echo "OK">/tmp/eth1/send
fi

