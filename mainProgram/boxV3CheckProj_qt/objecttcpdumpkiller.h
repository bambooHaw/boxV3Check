#ifndef OBJECTTCPDUMPKILLER_H
#define OBJECTTCPDUMPKILLER_H

#include <QObject>
#include <QTimer>
#include <QDebug>

class objectTcpdumpKiller : public QObject
{
    Q_OBJECT
public:
    objectTcpdumpKiller();
    ~objectTcpdumpKiller();

signals:

public slots:
    void slotTcpdumpTimerKiller(unsigned int msec = 0);
    int slotTcpdumpKiller();

private:
    QTimer timer0;
};

#endif // OBJECTTCPDUMPKILLER_H
