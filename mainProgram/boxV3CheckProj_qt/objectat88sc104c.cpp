#include "objectat88sc104c.h"


objectAT88SC104C::objectAT88SC104C(char *filePath)
{
    nodePath.remove(0, nodePath.size());
    nodePath.append(filePath);
}

objectAT88SC104C::~objectAT88SC104C()
{
}

void objectAT88SC104C::showMfLotArray()
{
    printf("at88sc mfAndlot:");
    for(int i=0; i<AT88SC104_BUF_LEGTH; i++){
        printf("%#x ", mfLotArray[i]);
    }
    puts("");
}

void objectAT88SC104C::showBuf(char* buf, int len)
{
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

char objectAT88SC104C::cmpWithCodesTables(char *src)
{
    char ATR_and_FAB_COADSTABLES[][10] = {
        /*at88sc104a*/
        {0xcd, 0xa8, 0xcd, 0xa8, 0xcd, 0xa3, 0xb3, 0xd5, 0x48, 0x54},
        /*boxv3*/
        {0xcd, 0xa8, 0xcd, 0xa8, 0xcd, 0xa3, 0xb3, 0xd5, 0x54, 0x54},
        /*at88sc104c*/
        {0x3b, 0xb2, 0x11, 0x00, 0x10, 0x80, 0x00, 0x01, 0x10, 0x10}
    };
    char cnt = sizeof(ATR_and_FAB_COADSTABLES)/sizeof(ATR_and_FAB_COADSTABLES[0]);
    int  i=0, ret = -1;

    for(i=0; i<cnt; i++)
    {
        ret = strncmp(ATR_and_FAB_COADSTABLES[i], src, strlen(src));
        if(!ret)break;
    }
    return (!ret)?WELLCHECK:FAILEDCHECK;
}

char objectAT88SC104C::getMfAndLotInfo()
{
    fd = open(nodePath.data(), O_RDWR);
    if(fd<0){
        qDebug("%s couldn't access!", nodePath.data());
        return FAILEDCHECK;
    }
    bzero(mfLotArray, sizeof(mfLotArray));
    if(ioctl(fd, AT88_GET_MFandLOT_INFO, (unsigned long)mfLotArray)){
        qDebug() << "AT88_GET_MFandLOT_INFO failed\n";
        close(fd);
        return FAILEDCHECK;
    }
    showMfLotArray();
    if(fd>0) close(fd);
    return cmpWithCodesTables((char*)mfLotArray);
}

char objectAT88SC104C::verfyPasswd()
{
    char buf[3][AT88SC104_BUF_LEGTH] = {"1234567890ab", };

    //backup userzone1 data
    memset(buf[2], 0, AT88SC104_BUF_LEGTH);
    if(false == readUsrZone(buf[2], 1)) return FAILEDCHECK;

    memcpy(buf[1], buf[0], AT88SC104_BUF_LEGTH);
    if(false == writeUsrZone(buf[0], 1))
    {
        //try upload backup
        writeUsrZone(buf[2], 1);
        return FAILEDCHECK;
    }

    memset(buf[0], 0, AT88SC104_BUF_LEGTH);
    if(false == readUsrZone(buf[0], 1))
    {
        //upload backup
        writeUsrZone(buf[2], 1);
        return FAILEDCHECK;
    }

#if 0
    printf("buf[0]:");
    showBuf(buf[0], AT88SC104_BUF_LEGTH);
    printf("buf[1]:");
    showBuf(buf[1], AT88SC104_BUF_LEGTH);
#endif

    //upload backup
    writeUsrZone(buf[2], 1);
    //compare
    if(strncmp(buf[0], buf[1], 12)) return FAILEDCHECK;

    return WELLCHECK;
}

bool objectAT88SC104C::readUsrZone(char *zoneBuf, char zoneNum)
{
    int ret = -1;

    fd = open(nodePath.data(), O_RDWR);
    if(fd<0){
        qDebug("%s couldn't access!", nodePath.data());
        return false;
    }

    if(ioctl(fd, AT88_SET_BLK, zoneNum) < 0){
        qDebug() << "AT88 readUsrZone ioctl() failed\n";
        return false;
    }

    bzero(zoneBuf, AT88SC104_BUF_LEGTH);
    ret = read(fd, zoneBuf, AT88SC104_BUF_LEGTH);
    if(ret<0)
    {
        qDebug() << "AT88 readUsrZone failed\n";
        return false;
    }

    //qDebug("---Debug: readUsrZone%d:", zoneNum);
    //showBuf(zoneBuf, AT88SC104_BUF_LEGTH);

    if(fd>0) close(fd);
    return true;
}

bool objectAT88SC104C::writeUsrZone(char *zoneBuf, char zoneNum)
{
    int ret = -1;

    fd = open(nodePath.data(), O_RDWR);
    if(fd<0){
        qDebug("%s couldn't access!", nodePath.data());
        return false;
    }

    if(ioctl(fd, AT88_SET_BLK, zoneNum) < 0){
        qDebug() << "AT88 writeUsrZone ioctl() failed\n";
        close(fd);
        return false;
    }

#if 0
    qDebug("writeUsrZone%d:", zoneNum);
    showBuf(zoneBuf, AT88SC104_BUF_LEGTH);
#endif

    ret = write(fd, zoneBuf, AT88SC104_BUF_LEGTH);
    if(ret<0)
    {
        qDebug() << "AT88 writeUsrZone failed\n";
        close(fd);
        return false;
    }
#if 0
    qDebug("writeUsrZone%d:", zoneNum);
    showBuf(zoneBuf, AT88SC104_BUF_LEGTH);
#endif
    if(fd>0) close(fd);
    return true;
}

bool objectAT88SC104C::getDeviceCode(char *deviceCode)
{
    char zoneBuf[AT88SC104_BUF_LEGTH] = {};
    bool flag = false;
    flag = readUsrZone(zoneBuf, 0);
    if(flag) memcpy(deviceCode, zoneBuf, SERIAL_NUMBER_LENGTH);

    return flag;
}

bool objectAT88SC104C::setDeviceCode(char *deviceCode)
{
    char zoneBuf[AT88SC104_BUF_LEGTH] = {};
    bool flag = false;
    flag = readUsrZone(zoneBuf, 0);
    if(false == flag) return flag;

    memcpy(zoneBuf + USRZONE_SERIAL_NUM_OFFSET, deviceCode, SERIAL_NUMBER_LENGTH);
    flag &= writeUsrZone(zoneBuf, 0);

    return flag;
}

bool objectAT88SC104C::getP2pId(char *p2pId)
{
    char zoneBuf[AT88SC104_BUF_LEGTH] = {};
    bool flag = false;
    flag = readUsrZone(zoneBuf, 1);
    if(flag) memcpy(p2pId, zoneBuf, P2P_ID_LENGTH);

    return flag;
}

bool objectAT88SC104C::setP2pId(char *p2pId)
{
    char zoneBuf[AT88SC104_BUF_LEGTH] = {};
     bool flag = false;
     flag = readUsrZone(zoneBuf, 1);
     if(false == flag) return flag;

     memcpy(zoneBuf + USRZONE_P2P_ID_OFFSET, p2pId, P2P_ID_LENGTH);
     flag &= writeUsrZone(zoneBuf, 1);

     return flag;
}


bool objectAT88SC104C::getSpecificMac(char order, char *mac)
{
    bool flag = false;
    char usrZone[AT88SC104_BUF_LEGTH] = {};
    flag = readUsrZone(usrZone, 3);
    if(flag) memcpy(mac, (usrZone + order*USR_ZONE_MAC_LENGTH), BOXV3_MAC_ADDR_LENGTH);

    return flag;
}
/*
 * @order(Please ref ZHITONG BOX-V3 AT88SC104 Protocols):
 *  0: wifi mac;
 *  1: 4g mac;
 *  2: wan mac;
 *  3: Lan mac
*/
bool objectAT88SC104C::setSpecificMac(char order, char *mac)
{
    bool flag = false;
    char usrZone[AT88SC104_BUF_LEGTH] = {};
    flag = readUsrZone(usrZone, 3);
    if(false == flag) return flag;

    memcpy((usrZone + order*USR_ZONE_MAC_LENGTH), mac, BOXV3_MAC_ADDR_LENGTH);
    flag &= writeUsrZone(usrZone, 3);

    return flag;
}

char objectAT88SC104C::verifyCheckAccess()
{
    char usrZone[AT88SC104_BUF_LEGTH] = {};

    if(!readUsrZone(usrZone, 0))
    {
        qDebug("Error: read access data from at88sc failed!");
        return -ENXIO;
    }

    at88_usr_zone0_t* data = (at88_usr_zone0_t*)usrZone;

    //check undone, can be check again.
    if(SELFCHECK_ALL_STAGE_OK != data->boxV3CheckFlag.result) return 0;
    else return -EACCES;
}

checkflag_enum objectAT88SC104C::getCheckStage()
{
    checkflag_enum checkStage;

    char usrZone[AT88SC104_BUF_LEGTH] = {};

    if(!readUsrZone(usrZone, 0)) return SELFCHECK_NO_ADMISSION;

    at88_usr_zone0_t* data = (at88_usr_zone0_t*)usrZone;

    if(SELFCHECK_OK != data->boxV3CheckFlag.production) checkStage = SELFCHECK_PRODUCTION;
    else
    {
        if(SELFCHECK_OK != data->boxV3CheckFlag.aging) checkStage = SELFCHECK_AGING;
        else
        {
            if(SELFCHECK_OK != data->boxV3CheckFlag.aged) checkStage = SELFCHECK_AGED;
            else
            {
                checkStage = SELFCHECK_ALL_STAGE_OK;
            }
        }
    }

    return checkStage;
}

checkflag_enum objectAT88SC104C::clearCheckFlags(checkflag_enum currentCheckStage)
{

    char usrZone[AT88SC104_BUF_LEGTH] = {};

    if(!readUsrZone(usrZone, 0)) return SELFCHECK_NO_ADMISSION;

    at88_usr_zone0_t* data = (at88_usr_zone0_t*)usrZone;
    switch(currentCheckStage)
    {
    case SELFCHECK_PRODUCTION:
    {
        data->boxV3CheckFlag.production = SELFCHECK_WAIT;
    }
    case SELFCHECK_AGING:
    {
        data->boxV3CheckFlag.aging = SELFCHECK_WAIT;
    }
    case SELFCHECK_AGED:
    {
        data->boxV3CheckFlag.aged = SELFCHECK_WAIT;
    }
    default:
    {
        data->boxV3CheckFlag.result = SELFCHECK_WAIT;
        break;
    }
    }

    if(!writeUsrZone(usrZone, 0)) return SELFCHECK_NO_ADMISSION;

    return SELFCHECK_WAIT;
}

checkflag_enum objectAT88SC104C::setCheckedFlags(checkflag_enum currentCheckStage)
{

    char usrZone[AT88SC104_BUF_LEGTH] = {};

    if(!readUsrZone(usrZone, 0)) return SELFCHECK_NO_ADMISSION;

    at88_usr_zone0_t* data = (at88_usr_zone0_t*)usrZone;
    switch(currentCheckStage)
    {
    case SELFCHECK_ADMIN_PASS:
    {
        data->boxV3CheckFlag.admin = SELFCHECK_ADMIN_PASS;
    }
    case SELFCHECK_ALL_STAGE_OK:
    {
        if(SELFCHECK_ALL_STAGE_OK == currentCheckStage)
        {
            data->boxV3CheckFlag.admin = SELFCHECK_WAIT;
        }
        data->boxV3CheckFlag.result = SELFCHECK_ALL_STAGE_OK;
    }
    case SELFCHECK_AGED:
    {
        data->boxV3CheckFlag.aged = SELFCHECK_OK;
    }
    case SELFCHECK_AGING:
    {
        data->boxV3CheckFlag.aging = SELFCHECK_OK;
    }
    case SELFCHECK_PRODUCTION:
    {
        data->boxV3CheckFlag.production = SELFCHECK_OK;
        if(SELFCHECK_ALL_STAGE_OK == data->boxV3CheckFlag.result) break;
    }
    default:
    {
        data->boxV3CheckFlag.result = SELFCHECK_WAIT;
        break;
    }
    }

    if(!writeUsrZone(usrZone, 0)) return SELFCHECK_NO_ADMISSION;

    return SELFCHECK_WAIT;
}


QString getFirmwareName()
{
    QDir dir(QString(BOXV3_FIRMWARE_PATH));
    QStringList infolist = dir.entryList(QDir::NoDotAndDotDot | QDir::Files);

    qDebug() << "All stm8 firmware's name:";
    int i;
    for(i=0; i<infolist.size(); i++)
        qDebug() << infolist.at(i);

    QString firmwareNamePath;
    if(i>0)
    {
        firmwareNamePath = QString(BOXV3_FIRMWARE_PATH);
        firmwareNamePath += "/";
        firmwareNamePath += infolist.at(0);
    }

     return (1 == infolist.size()) ? firmwareNamePath : QString("");
}
