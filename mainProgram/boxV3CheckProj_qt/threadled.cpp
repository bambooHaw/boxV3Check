#include "threadled.h"

threadLed::threadLed()
{
    flagLed = NOTCHECK;
    initLedGpio();

    moveToThread(this);
}

void threadLed::initLedGpio()
{
    QByteArray cmdArray;
    //get led0 gpio dir
    cmdArray.remove(0, cmdArray.size());
    cmdArray.append("echo ");
    cmdArray.append(QString::number(GPIO_INDEX_LED0, 10));
    cmdArray.append(" > /sys/class/gpio/export");
    system(cmdArray.data());
    //get led1 gpio dir
    cmdArray.remove(0, cmdArray.size());
    cmdArray.append("echo ");
    cmdArray.append(QString::number(GPIO_INDEX_LED1, 10));
    cmdArray.append(" > /sys/class/gpio/export");
    system(cmdArray.data());
    //get led2 gpio dir
    cmdArray.remove(0, cmdArray.size());
    cmdArray.append("echo ");
    cmdArray.append(QString::number(GPIO_INDEX_LED2, 10));
    cmdArray.append(" > /sys/class/gpio/export");
    system(cmdArray.data());
    //get led3 gpio dir
    cmdArray.remove(0, cmdArray.size());
    cmdArray.append("echo ");
    cmdArray.append(QString::number(GPIO_INDEX_LED3, 10));
    cmdArray.append(" > /sys/class/gpio/export");
    system(cmdArray.data());

    //set led0 direction -> out
    cmdArray.remove(0, cmdArray.size());
    cmdArray.append("echo out > ");
    cmdArray.append("/sys/class/gpio/gpio");
    cmdArray.append(QString::number(GPIO_INDEX_LED0, 10));
    cmdArray.append("/direction");
    system(cmdArray.data());
    //set led1 direction -> out
    cmdArray.remove(0, cmdArray.size());
    cmdArray.append("echo out > ");
    cmdArray.append("/sys/class/gpio/gpio");
    cmdArray.append(QString::number(GPIO_INDEX_LED1, 10));
    cmdArray.append("/direction");
    system(cmdArray.data());
    //set led2 direction -> out
    cmdArray.remove(0, cmdArray.size());
    cmdArray.append("echo out > ");
    cmdArray.append("/sys/class/gpio/gpio");
    cmdArray.append(QString::number(GPIO_INDEX_LED2, 10));
    cmdArray.append("/direction");
    system(cmdArray.data());
    //set led3 direction -> out
    cmdArray.remove(0, cmdArray.size());
    cmdArray.append("echo out > ");
    cmdArray.append("/sys/class/gpio/gpio");
    cmdArray.append(QString::number(GPIO_INDEX_LED3, 10));
    cmdArray.append("/direction");
    system(cmdArray.data());

}

void threadLed::optLed(char index, char enable)
{
    QByteArray cmdArray;
    switch(index)
    {
    //led0
    case 0:
    {
        //set led0 value
        cmdArray.remove(0, cmdArray.size());
        cmdArray.append("echo ");
        cmdArray.append(QString::number(enable, 10));
        cmdArray.append(" > /sys/class/gpio/gpio");
        cmdArray.append(QString::number(GPIO_INDEX_LED0, 10));
        cmdArray.append("/value");
        system(cmdArray.data());
        break;
    }
    case 1:
    {
        //set led1 value
        cmdArray.remove(0, cmdArray.size());
        cmdArray.append("echo ");
        cmdArray.append(QString::number(enable, 10));
        cmdArray.append(" > /sys/class/gpio/gpio");
        cmdArray.append(QString::number(GPIO_INDEX_LED1, 10));
        cmdArray.append("/value");
        system(cmdArray.data());
        break;
    }
    case 2:
    {
        //set led2 value
        cmdArray.remove(0, cmdArray.size());
        cmdArray.append("echo ");
        cmdArray.append(QString::number(enable, 10));
        cmdArray.append(" > /sys/class/gpio/gpio");
        cmdArray.append(QString::number(GPIO_INDEX_LED2, 10));
        cmdArray.append("/value");
        system(cmdArray.data());
        break;
    }
    case 3:
    {
        //set led3 value
        cmdArray.remove(0, cmdArray.size());
        cmdArray.append("echo ");
        cmdArray.append(QString::number(enable, 10));
        cmdArray.append(" > /sys/class/gpio/gpio");
        cmdArray.append(QString::number(GPIO_INDEX_LED3, 10));
        cmdArray.append("/value");
        system(cmdArray.data());
        break;
    }
    default:
    {
        break;
    }
    }

}

void threadLed::slotDisplayCheckout(QByteArray checkout)
{
    results_info_t info;
    memcpy(&info, checkout.data(), checkout.size());


    if (ONCHECK == info.whole)
    {
        optLed(0, 1);
        msleep(200);
        optLed(0, 0);
        optLed(1, 1);
        msleep(200);
        optLed(1, 0);
        optLed(2, 1);
        msleep(200);
        optLed(2, 0);
        optLed(3, 1);
        msleep(200);
        optLed(3, 0);
    }else if(NOTCHECK == info.whole)
    {
        optLed(0, 0);
        optLed(1, 0);
        optLed(2, 0);
        optLed(3, 0);
    }else if(WELLCHECK == info.whole)
    {
        optLed(0, 1);
        optLed(1, 1);
        optLed(2, 1);
        optLed(3, 1);
    }else
    {
        optLed(0, 1);
        optLed(1, 1);
        optLed(2, 1);
        optLed(3, 1);
        msleep(300);

        optLed(0, 0);
        optLed(1, 0);
        optLed(2, 0);
        optLed(3, 0);
        msleep(300);

        optLed(0, 1);
        optLed(1, 1);
        optLed(2, 1);
        optLed(3, 1);
        msleep(300);
    }


}

void threadLed::slotLedShowAgingResult(QByteArray checkout)
{
    results_info_t info;
    memcpy(&info, checkout.data(), checkout.size());

    while(1)
    {
        if(NOTCHECK == flagLed)
        {
            optLed(0, 1);
            optLed(1, 1);
            optLed(2, 1);
            optLed(3, 1);
            msleep(600);

            optLed(0, 0);
            optLed(1, 0);
            optLed(2, 0);
            optLed(3, 0);
            msleep(600);
        }
        else if(ONCHECK == flagLed)
        {
            optLed(0, 1);
            msleep(200);
            optLed(0, 0);
            optLed(1, 1);
            msleep(200);
            optLed(1, 0);
            optLed(2, 1);
            msleep(200);
            optLed(2, 0);
            optLed(3, 1);
            msleep(200);
            optLed(3, 0);
        }else if(WELLCHECK == flagLed)
        {
            optLed(0, 1);
            optLed(1, 1);
            optLed(2, 1);
            optLed(3, 1);
            sleep(1);
        }else
        {
            optLed(0, 0);
            optLed(1, 0);
            optLed(2, 0);
            optLed(3, 0);
            sleep(1);
        }
    }
}

void threadLed::slotSetFlagLed(char flag)
{
    flagLed = flag;
}

void threadLed::run()
{

#if 0
    results_info_t result;
    result.whole = WELLCHECK;
    QByteArray array;
    array.resize(sizeof(results_info_t));
    memcpy(array.data(), &result, sizeof(results_info_t));

    //indicate check start
    this->slotDisplayCheckout(array);
#endif
    exec();
}
