#include "threadkeydetection.h"

#if 1
extern "C" {
#include "ztboxc_deprecated.h"
}
#endif

threadKeyDetection::threadKeyDetection()
{
    memset(&currentStatus, 0, sizeof(key_boxv3_status_t));
    moveToThread(this);
}


int threadKeyDetection::slotStartKeyDect()
{
    char pressFlag=0, sendFlag=0;
    struct timeval pressTime, holdTime;
    __suseconds_t usecDif;
    int fd=-1, ret=-1;
    unsigned char value='1';
    struct pollfd fds[1];

    fd = open(GPIO_VALUE_BOX_V3_SW3, O_RDONLY);
    if( fd == -1 )
    {
        printf("Error: open %s failed!\n", GPIO_VALUE_BOX_V3_SW3);
        return fd;
    }

    fds[0].fd     = fd;
    fds[0].events = POLLIN;
    while(1)
    {
        ret = poll(fds, 1, 1000); //set timeout as 1000ms
        if(0 == ret)
        {
            printf("poll again(fd:%d)...\n", fds[0].fd);
            continue;
        }
        if(-1 == ret) printf("Error: poll %s failed", GPIO_VALUE_BOX_V3_SW3);
        if(fds[0].revents & POLLIN)
        {
            ret = lseek(fd, 0, SEEK_SET);
            if(ret < 0) printf("Error: lseek %s failed", GPIO_VALUE_BOX_V3_SW3);
            ret = read(fd, &value, 1);
            if(ret < 0) printf("Error: read %s failed", GPIO_VALUE_BOX_V3_SW3);
            else if('0' == value)
            {
                if(1==pressFlag)
                {//press and hold the SW3 key
                    gettimeofday(&holdTime, NULL);
                    //software algorithm for Jitters Elimination
                    usecDif = (holdTime.tv_sec - pressTime.tv_sec)*1000000 + (holdTime.tv_usec - pressTime.tv_usec);
                    if(usecDif > 50*1000)//Eliminate for 50ms, you can modify this number
                        if(0 == sendFlag)
                        {
                            sendFlag = 1;
                            printf("sw3 was pressed(Elevel:%c)!\n", value);
                            slotKeyHasBeenPressed();
                        }
                }
                else
                {//only just press the key
                    pressFlag = 1;
                    sendFlag = 0;
                    gettimeofday(&pressTime, NULL);
                }
            }
            else if('1' == value)//loose the key
            {
                pressFlag = 0;
                sendFlag = 0;
            }
        }
    }

}

void threadKeyDetection::slotKeyHasBeenPressed()
{

    if(currentBarCodeArray.isEmpty())
    {
        currentStatus.pressedCnt = 0;
        emit signalRecorder(LOG_NOTICE, "warning barCode0");
    }else
    {
        killallOtherApps();
        currentStatus.pressedCnt++;
    }

    //qDebug("---Debug: correct pressed:%d", currentStatus.pressedCnt);
    if(1 == currentStatus.pressedCnt)
    {
        //qDebug("---Debug: keyhasbeenpressed. checkStatus:%d, barStatus:%d, pressedCnt:%d", currentStatus.checkStatus, currentStatus.barStatus, currentStatus.pressedCnt);
        switch(currentStatus.checkStatus)
        {
        case SELFCHECK_PRODUCTION:
        {
            //if there is a boxv3 barcode scaned
            if(SCAN_BOXV3_CODE == currentStatus.barStatus)
            {
                //start general examination thread, if all check success then start udp stage.
                //start tcp service
                //start each once
                emit signalRecorder(LOG_ALERT, "status pro");

                //set deviceCode to at88sc104c
                QMutexLocker lockerAt88(&mutexAT88);
                objectAT88SC104C at88((char*)AT88SC104_NODE_NAME);
                if(!at88.setDeviceCode(currentBarCodeArray.data()))
                {
                    qDebug("Error: Can't write serialNum to at88sc.");
                }

                //log recorder the deviceCode
                logStr.remove(0, logStr.size());
                logStr.append(QString("SerialNum(recorded):"));
                logStr.append(currentBarCodeArray);
                //qDebug("---Debug: log: %s", logStr.toLocal8Bit().data());
                emit signalRecorder(LOG_CRIT, "", logStr);
                emit signalRecorder(LOG_NOTICE, "info pro0");
            }else
            {
                currentStatus.pressedCnt = 0;
                emit signalRecorder(LOG_NOTICE, "warning barCode0");
            }
            break;
        }
        case SELFCHECK_AGING:
        case SELFCHECK_AGED:
        {
            //if there is a boxv3 barcode scaned
            if(SCAN_BOXV3_CODE == currentStatus.barStatus)
            {
                //set deviceCode to at88sc104c
                QMutexLocker lockerAt88(&mutexAT88);
                objectAT88SC104C at88((char*)AT88SC104_NODE_NAME);
                QByteArray tmpArray;
                tmpArray.resize(AT88SC104_BUF_LEGTH);
                if(!at88.getDeviceCode(tmpArray.data()))
                {
                    emit signalRecorder(LOG_CRIT, "error at88 ENXIO");
                }
                int ret = strncmp(tmpArray.data(), currentBarCodeArray.data(), DEVICECODE_LENGTH);
#ifdef BOXV3_DEBUG_NET
                ret = 0;
#endif
                if(0 == ret)
                {
                    currentStatus.pressedCnt += 1;
                    //update the check status from aging to aged
                    currentStatus.checkStatus = SELFCHECK_AGED;
                    //change aging stage to aged stage;
                    //close aging thread
                    //start aged thread
                    if(SELFCHECK_WAIT != at88.setCheckedFlags(SELFCHECK_AGING))
                    {
                        emit signalRecorder(LOG_CRIT, "error at88 ENXIO");
                    }
                    emit signalRecorder(LOG_ALERT, "status aged");
                    emit signalRecorder(LOG_CRIT, "info aged0");
                }else
                {
                    emit signalRecorder(LOG_NOTICE, "error aged err0");
                    if(SELFCHECK_AGING == currentStatus.checkStatus)
                    {
                        currentStatus.pressedCnt = 0;
                    }
                }
            }
            break;
        }
        default:
        {
            emit signalRecorder(LOG_NOTICE, "warning stage w0");
            break;
        }
        }

    }else
    {
        emit signalRecorder(LOG_NOTICE, "warning keyPress1");
    }


}

void threadKeyDetection::slotGetScanedBarCode(char res, QByteArray barCodeArray)
{
    barCodeGunScanResult_enum result = (barCodeGunScanResult_enum)res;

    switch(currentStatus.pressedCnt)
    {
    case 0:
    {
        switch(result)
        {
        case SCAN_BOXV3_CODE:
        {
            //update correct barCode info
            currentBarCodeArray = barCodeArray;
            currentStatus.barStatus = SCAN_BOXV3_CODE;
            //display the barCode
            emit signalDisplayBarCode((char)SCAN_BOXV3_CODE, currentBarCodeArray);
            break;
        }
        default:
        {
            //update bad barCode info
            currentBarCodeArray.remove(0, currentBarCodeArray.length());
            currentStatus.barStatus = SCAN_INVALID;
            //emit dispaly get an invalid barCode.
            emit signalDisplayBarCode((char)SCAN_INVALID, barCodeArray);
            break;
        }
        }
        break;
    }
    default:
    {
        //emit dispaly invalid option after key pressed.
        emit signalRecorder(LOG_NOTICE, "warning barCode1");
        break;
    }
    }

}


void threadKeyDetection::tryGetLatestTime(void)
{
    int ret = 0;
    ret = system("ifconfig wlan0 up");
    if(ret)return;
    system("killall -9 hostapd");
    for(ret=0; ret<3; ret++)
    {
        system("killall -9 wpa_supplicant");
        system("wpa_supplicant -i wlan0 -c /etc/wpa_supplicant.conf &");
        system("udhcpc -i wlan0 -n -q -t 4");
        QString netName("wlan0");
        QString ipStr;
        if(getIfconfigIp(netName, ipStr) == 0) break;
    }
    //update time after wifi works well
    //system("ntpclient -s -d -c 1 -i 2 -h cn.pool.ntp.org");
}

void threadKeyDetection::run()
{
    //update check status
    if(1)
    {
        QMutexLocker lockerAt88(&mutexAT88);
        objectAT88SC104C at88((char*)AT88SC104_NODE_NAME);
        currentStatus.checkStatus = at88.getCheckStage();
        if(SELFCHECK_NO_ADMISSION == currentStatus.checkStatus)
        {
            currentStatus.checkStatus = SELFCHECK_PRODUCTION;
            emit signalRecorder(LOG_CRIT, "error status default prod");
        }
    }

    if(SELFCHECK_ALL_STAGE_OK != currentStatus.checkStatus)
    {
        prepareNetEnv();
        testPreparation();
    }

    //check status and send it to log thread
    switch(currentStatus.checkStatus)
    {
    case SELFCHECK_PRODUCTION:
    {
        emit signalRecorder(LOG_CRIT, "status pro");
        break;
    }
    case SELFCHECK_AGING:
    {
        emit signalRecorder(LOG_ALERT, "status aging");
        //tryGetLatestTime();
        break;
    }
    case SELFCHECK_AGED:
    {
        emit signalRecorder(LOG_CRIT, "status aged");
        break;
    }
    case SELFCHECK_ALL_STAGE_OK:
    {
        emit signalRecorder(LOG_CRIT, "status allCheckedOk");

        sleep(1);
        quit_sighandler(SIGUSR1);

        break;
    }
    default:
    {
        emit signalRecorder(LOG_CRIT, "error unknown0");
        break;
    }
    }

    system("echo 120 > /sys/class/gpio/export");
    system("echo in > /sys/class/gpio/gpio120/direction");

#ifndef BOXV3_DEBUG_NET
    if(this->slotStartKeyDect())
    {
        emit signalRecorder(LOG_CRIT, "error button err0");
    }
#else
    currentBarCodeArray = BOXV3_TEST_BARCODE;
    currentStatus.pressedCnt = 0;
    currentStatus.barStatus = SCAN_BOXV3_CODE;
    slotKeyHasBeenPressed();
#endif

    exec();
}

