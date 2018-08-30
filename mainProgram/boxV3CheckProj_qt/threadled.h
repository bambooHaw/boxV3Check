#ifndef THREADLED_H
#define THREADLED_H

#include <QObject>
#include <QThread>
#include "boxv3.h"

class threadLed : public QThread
{
    Q_OBJECT
public:
    threadLed();

    void initLedGpio(void);
    void optLed(char index, char enable);
public slots:
    void slotDisplayCheckout(QByteArray checkout);
    void slotLedShowAgingResult(QByteArray checkout);
    void slotSetFlagLed(char flag);
protected:
    void run(void);
    char flagLed;
};


#endif // THREADLED_H
