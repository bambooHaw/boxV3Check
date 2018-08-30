#include "objecttcpdumpkiller.h"

objectTcpdumpKiller::objectTcpdumpKiller()
{
}

objectTcpdumpKiller::~objectTcpdumpKiller()
{

}

void objectTcpdumpKiller::slotTcpdumpTimerKiller(unsigned int msec)
{
    connect(&timer0, SIGNAL(timeout()), this, SLOT(slotTcpdumpKiller()), Qt::DirectConnection);
    qDebug() << __func__<<  __LINE__ << endl;
    timer0.setInterval(msec);
    QTimer::singleShot(0, &timer0, SLOT(start())); //msec
    qDebug() << __func__<<  __LINE__ << endl;

}


int objectTcpdumpKiller::slotTcpdumpKiller()
{
    qDebug() << __func__<<  __LINE__ << endl;
    QTimer::singleShot(0, &timer0, SLOT(stop()));
    qDebug() << __func__<<  __LINE__ << endl;
    /*
     * 2) SIGINT  ==  ctrl + c
     * Do not use 9) SIGKILL, this signal will kill program imadiatelly without any chance for program's option
     * 20180409
    */
    system("killall -9 ping");
    return system("killall -2 tcpdump");
}
