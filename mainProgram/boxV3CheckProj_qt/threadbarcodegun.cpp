#include "threadbarcodegun.h"

threadBarCodeGun::threadBarCodeGun()
{
    moveToThread(this);
}

threadBarCodeGun::~threadBarCodeGun()
{
}


void threadBarCodeGun::showBuf(char *buf, int len)
{
    int i;

    for(i=0; i<len; i++){
        if(0== i%10)
            if(0!=i)
                puts("");
        printf("0x%02x ", buf[i]);
    }
    puts("");
}

//void threadBarCodeGun::slotSetTmpSerial(QSerialPort *port)
//{
//    tmpSerial = port;
//}
//void threadBarCodeGun::slotReadData(void)
//{
//    qDebug() << "slotReadData(void), read:";
//    QByteArray arr = tmpSerial->readAll();
//    showBuf(arr.data(), arr.size());
//}

barCodeGunScanResult_enum threadBarCodeGun::slotAnalysizeBarCode(QString barCodeStr)
{

    //deal with bar str first
    barCodeStr.replace('\n', ' ');
    barCodeStr.replace('\v', ' ');
    barCodeStr.replace(" ", "");
    barCodeArray = barCodeStr.toLocal8Bit();

    //qDebug("---Debug: barCodeArray:%s(size:%d)", barCodeArray.data(), barCodeArray.size());
    if(barCodeArray.size() != BOXV3_BARCODE_LENGTH) return SCAN_BADCODE;

    for(int i=0; i<barCodeArray.size(); i++)
    {
        int val = barCodeArray[i];
        if(((val >= 48) && (val <= 57)) || (val == 10) || (val == 11))
        {
            ;//qDebug() << barCodeArray[i];
        }else
        {
            return SCAN_BADCODE;
        }
    }

    //boxv3 judgement
    if( (barCodeArray[0] != (char)'1') || (barCodeArray[1] != (char)'4') || (barCodeArray[2] != (char)'6') )
    {
        return SCAN_WRONGCODE;
    }


    return SCAN_BOXV3_CODE;
}

char threadBarCodeGun::slotReadScanedBarCode(void)
{
    QString barCodeLastLineStr;
    QFile barCodeFile((char*)BOX_V3_BAR_CODE_TMP_FILE);
    if(barCodeFile.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        while(!barCodeFile.atEnd())
        {
            QByteArray line = barCodeFile.readLine();
            barCodeLastLineStr = QString(line);
            //qDebug("\nline: %s\n", lastLineStr.toLocal8Bit().data());
        }
    }
    barCodeFile.close();


    //analysize the code
    //send the serial num to key press thread
    switch(slotAnalysizeBarCode(barCodeLastLineStr))
    {
    case SCAN_BOXV3_CODE:
    {
        emit signalSendScanedBarCode((char)SCAN_BOXV3_CODE, barCodeArray);
        break;
    }
    default:
    {
        emit signalSendScanedBarCode((char)SCAN_INVALID, QByteArray("Invalid BarCode."));
        break;
    }
    }

    return 0;
}


void threadBarCodeGun::slotBarCodeGunScanDone(char codeCnt)
{
    //qDebug("---Debug: Scan(%d):", codeCnt);
    codeCnt += 0;
    slotReadScanedBarCode();

}


void threadBarCodeGun::run()
{
    char str[32] = {};
    sprintf(str, "rm -rf %s", BOX_V3_BAR_CODE_TMP_FILE);
    system(str);

    objectUart serial((char*)FILE_PATH_UART485);
    connect(&serial, SIGNAL(signalBarCodeGunSanDone(char)), this, SLOT(slotBarCodeGunScanDone(char)), Qt::DirectConnection);

    if(serial.slotStartBarCodeGun() < 0)
    {
        emit signalRecorder(LOG_CRIT|LOG_NOTICE|LOG_DEBUG, "error uart485 err0");
    }

    exec();
}

