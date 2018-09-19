#include "boxv3.h"
#include <arpa/inet.h>
#include <QNetworkInterface>
#include <QFile>


QMutex mutexAT88;

void prepareNetEnv(void)
{
    system("brctl delif br0 wlan0 >> /dev/null");
    system("ifconfig br0 down >> /dev/null");
    system("brctl delbr br0 >> /dev/null");
    //system("ifconfig wlan0 up");
    //system("route add -net 192.168.1.0/24 wlan0");
}


void killallBrotherApps(char killWlanFlag)
{
    if(killWlanFlag)
    {
        system("killall -9 wpa_supplicant >> /dev/null");
        system("killall -9 wlanMaster.sh >> /dev/null");
    }
    system("killall -9 agingNetCheckApp >> /dev/null");
    system("killall -9 checkAppFor4G >> /dev/null");
}

void killallOtherApps(void)
{
    system("killall -9 HWDaemon.out");  //process monitor
    system("killall -9 MCU");           //web communucation
    system("killall -9 node");          //web setting
    system("killall -9 demoTunnel");    //p2p management
    system("killall -9 hostapd");       //wifi ap
    system("killall -9 udhcpd");        //net brig

    system("killall -9 udhcpc");        //udhcpc for eth0
    system("killall -9 ntpd");
    system("killall -9 ping");
    system("killall -9 tcpdump");
}

void prepareCommunicationEnv(void)
{
    prepareNetEnv();
    killallBrotherApps(0);
    killallOtherApps();
    system("ifconfig eth0 down");
    system("ifconfig eth1 down");
    system("ifconfig usb0 down");
    system("ifconfig lo down");
}

void quit_sighandler(int signum)
{
    qDebug("signum: %d", signum);

    if(SIGINT == signum)
    {
        qDebug("quit by user!");
        killallBrotherApps(1);
        killallOtherApps();
        kill(getpid(), 9);
        //system("exit(0)");
        //system("killall -9 boxV3CheckApp");
    }

    if(SIGUSR1 == signum)
    {
        qDebug("quit by the app itself!");
        killallBrotherApps(1);
        kill(getpid(), 9);
        //system("exit(0)");
        //system("killall -9 boxV3CheckApp");
    }
    if(SIGUSR2 == signum)
    {
        qDebug("restart by the app itself!");
        killallBrotherApps(1);
        system("boxV3CheckApp -f production");
    }
    return;
}


void testPreparation(void)
{
    pid_t cur_pid = getpid();
    QString tmp("pid ");
    tmp += QString::number(cur_pid, 10);
    QString str_pid = tmp.section(" ", 1, 1, QString::SectionSkipEmpty);
    //qDebug("pid:#%s#, length:(tmp:%d),(pid:%d)", str_pid.toLocal8Bit().data(), tmp.size(), str_pid.size());

    //kill it's brothers
    system("ps aux | grep boxV3CheckApp | grep -v grep | awk '{print $1}' > brothers_pid.txt");
    QFile file("brothers_pid.txt");
    if(!file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        qDebug("Error: Can't ps tmp file!");
        return;
    }else
    {
        QString txt;
        QTextStream io(&file);
        while(!io.atEnd())
        {
            txt = "kill -9 ";
            QString line = io.readLine();
            //Do not be self-killer
            if(0 != line.compare(str_pid))
            {
                txt += line;
                if(txt.size() > QString("kill -9  ").size())
                {
                    system(txt.toLocal8Bit().data());
                    txt.append(", which is another boxV3checkApp. Current boxV3CheckApp's pid is ");
                    txt.append(str_pid);
                    qDebug() << txt;
                }
            }
        }
        file.close();
    }
    system("rm -rf brothers_pid.txt");

    //kill all the app, which have a effect to the device
    killallBrotherApps(1);
    killallOtherApps();

    //enable some env interfase
    system("chmod 777 /dev/swim");

#ifndef BOXV3_DEBUG_NET
    //start 4g establish monitor
    system("rm -rf /tmp/lte");
    system("rm -rf /tmp/wlan0");
    system("chmod 777 /opt/checkAppFor4G");
    system("/opt/checkAppFor4G -t 3 >> /dev/null &");
#endif
}

/* get ip whose net name is netName.
 * return:
 * 0:success
 */
int getIfconfigIp(QString& netName, QString& ipStr)
{
    int ret = 0;
    QList<QNetworkInterface> list = QNetworkInterface::allInterfaces();
    QNetworkInterface interface;
    QList<QNetworkAddressEntry> entryList;
    int i=0, j = 0;

    if(netName.isEmpty())
    {
        ret = -EINVAL;
        qDebug("Error: Invalid argument for getIfconfigIp");
        return ret;
    }

    ret = -EAGAIN;
    for(i = 0; i<list.count(); i++)
    {
        interface = list.at(i);
        entryList= interface.addressEntries();

        //qDebug("i cnt: %d", i);
        if(strcmp(interface.name().toLatin1().data(), netName.toLocal8Bit().data())) continue;
        //qDebug() << "DevName: " << interface.name();
        //qDebug() << "Mac: " << interface.hardwareAddress();
        //20180324log: There's error about it'll has a double scan for same name if you use the entryList.count(), What a fuck?
        if(entryList.isEmpty())
        {
            ret = -ENXIO;
            qDebug() << "Error: wifi doesn't get a ip.";
            break;
        }else
        {
            ret = 0;
            ipStr = "";
            QNetworkAddressEntry entry = entryList.at(j);
            //if there has some ip address
            if(entry.ip().toIPv4Address()){
                in_addr in;
                uint32_t ui32Ip = htonl(entry.ip().toIPv4Address());
                memcpy(&in, &ui32Ip, sizeof(ui32Ip));
                ipStr.append(inet_ntoa(in));
                return ret;
            }else
            {
                ret = -ENXIO;
            }

            //qDebug() << "j"<<j<< "Netmask: " << entry.netmask().toString();
            //qDebug() << "j"<<j<< "Broadcast: " << entry.broadcast().toString();
            break;
        }
    }

    return ret;
}
