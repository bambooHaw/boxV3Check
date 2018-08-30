#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    //ui->lineEdit_VersionAndRunCnt->setVisible(false);
    QObject::connect(&aliveTimer, SIGNAL(timeout()), this, SLOT(slotShowAlive()));
    qDebug() << __func__<<  __LINE__ << endl;
    aliveTimer.setInterval(1000); //1000
    QTimer::singleShot(0, &aliveTimer, SLOT(start()));
    qDebug() << __func__<<  __LINE__ << endl;

    initDisplayScreen();

    tcpDone = NOTCHECK;

    softVersion = ui->lineEdit_VersionAndRunCnt->text();
    ui->lineEdit_VersionAndRunCnt->setText(QString(BOXV3CHECKAPP_VERSION));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initDisplayScreen()
{

    ui->setupUi(this);
#if 0
    QFont ft;
    ft.setPointSize(16);
    ui->label_serialNum->setFont(ft);
#endif
#if 1
    QImage png;
    //loading
    png.load(BOXV3_QT_PICTURE_GIF_LOADING);
    png.save(BOXV3_QT_PICTURE_GIF_LOADING);
    //temp
    png.load(BOXV3_QT_PICTURE_CPU_TEMP);
    png.save(BOXV3_QT_PICTURE_CPU_TEMP);
    //red
    png.load(BOXV3_QT_PICTURE_LIGHT_RED);
    png.save(BOXV3_QT_PICTURE_LIGHT_RED);
    //green
    png.load(BOXV3_QT_PICTURE_LIGHT_GREEN);
    png.save(BOXV3_QT_PICTURE_LIGHT_GREEN);
    //yellow
    png.load(BOXV3_QT_PICTURE_LIGHT_YELLOW);
    png.save(BOXV3_QT_PICTURE_LIGHT_YELLOW);
    //wait
    png.load(BOXV3_QT_PICTURE_LIGHT_WAIT);
    png.save(BOXV3_QT_PICTURE_LIGHT_WAIT);
#endif

#if 1
    //show light for the key
    QString path = BOXV3_QT_PICTURE_LIGHT_WAIT;
    QPixmap img(path);
    QPixmap imgScaled = img.scaled(QSize(BOXV3_QT_PICTURE_SCALED_SIZE, BOXV3_QT_PICTURE_SCALED_SIZE));

    ui->label_light_keyPressed->setPixmap(imgScaled);
    ui->label_light_keyPressed->resize(imgScaled.width(),imgScaled.height());
#else
    //show loading gif
    movie = new QMovie(BOXV3_QT_PICTURE_GIF_LOADING);
    ui->label_light_keyPressed->setMovie(movie);
    movie->setScaledSize(QSize(50, 50));
    movie->start();
    ui->label_light_keyPressed->show();
#endif

    //invisible time and wifi window, open them when they're avalable
    ui->labelSysTime->setVisible(false);
    ui->dateTimeEditSys->setVisible(false);

    ui->label_ipWifi->setVisible(false);
    ui->lineEdit_ipWifi->setVisible(false);


    //invisible the aging result
    ui->label_lightpic_agingResult->setVisible(false);
    ui->label_light_aging->setVisible(false);
    ui->label_uptime->setVisible(false);
}

void MainWindow::slotShowWifiIp()
{
    ui->label_ipWifi->setVisible(true);
    ui->lineEdit_ipWifi->setVisible(true);

    QString netName("wlan0");
    QString ipStr;
    if(getIfconfigIp(netName, ipStr) == 0){
        ui->lineEdit_ipWifi->setText(ipStr);
    }else{
        //No IP
        ui->lineEdit_ipWifi->setText(QString("NO IP"));
    }
}

void MainWindow::slotShowAlive()
{
    ui->lineEdit_VersionAndRunCnt->setText(QDateTime::currentDateTime().toString("yyyy/MM/dd[HH:mm:ss]"));
}

void MainWindow::showWifiMac()
{
    QNetworkInterface  localInterface = localInterface.interfaceFromName("wlan0");
    QString mac =localInterface.hardwareAddress();
    //ui->lineEdit_macWifi->setText(mac);
}

void MainWindow::slotShowSystemDataAndTime()
{
    ui->labelSysTime->setVisible(true);
    ui->dateTimeEditSys->setVisible(true);
    //if(0 != generalThreadStarted)
    {
        QDateTime dataTime = QDateTime::currentDateTime();
        //dataTime.setTimeZone(QTimeZone);
        //ui->dateTimeEditSys->setDateTime(dataTime.setTimeSpec(Qt::LocalTime););
        //QTimeZone tz = dataTime.timeZone();
        ui->dateTimeEditSys->setDateTime(dataTime);
        ui->dateTimeEditSys->setDisplayFormat("yyyy/MM/dd HH:mm:ss");
    }
}

void MainWindow::slotShowCpuTemperature()
{
//    QString cmd = "cat";
//    QStringList param;
//    QProcess process;

//    param << "/sys/class/hwmon/hwmon1/temp1_input";

//    process.start(cmd, param);
//    process.waitForFinished(10000);
//    QByteArray outPut = process.readAllStandardOutput();
    QFile file("/sys/class/hwmon/hwmon1/temp1_input");
    if(!file.open(QFile::ReadOnly|QFile::Text))
    {
        qDebug() << "Could not open cpu temperature file.";
        return;
    }
    QTextStream in(&file);
    QString txt = in.readAll();
    //qDebug() << txt;
    //ui->progressBarTemperature->setValue(txt.toInt());
    QString path = BOXV3_QT_PICTURE_CPU_TEMP;
    QPixmap img(path);
    QPixmap imgScaled = img.scaled(QSize(BOXV3_QT_PICTURE_TEMP_WIDTH, BOXV3_QT_PICTURE_TEMP_HIGH));
    QPixmap imgTempBar = imgScaled.copy(0, 0, (imgScaled.width() * txt.toInt())/100, imgScaled.height());

    ui->label_bar_cpuTemp->setPixmap(imgTempBar);
    ui->label_bar_cpuTemp->resize(imgTempBar.width(), imgTempBar.height());


    txt.replace(QChar('\n'), QString(" "));
    txt.replace(QChar('\v'), QString(" "));
    txt.append(QString("*C"));
    ui->label_celsius->setText(txt);
    file.close();
}


void MainWindow::slotDisplayNotes(QString key, QString data)
{
    if(key.contains("status"))
    {
        ui->label_checkStatus->setText(data);
    }else if(key.contains("serialNum"))
    {
        ui->lineEdit_serialNum->setText(data);
    }else if(key.contains("title"))
    {

    }else if(key.contains("currentMaxUptime"))
    {
        ui->label_uptime->setVisible(true);
        bool ok;
        float uptimeSecs = data.toFloat(&ok);
        long min = uptimeSecs/60;
        long h = min/60;
        min = min%60;
        QString time = QString::number(h, 10);
        time.append("h");
        time += QString::number(min, 10);
        time.append("m");
        ui->lineEdit_VersionAndRunCnt->setText(time);
    }else if(key.contains("uptimeMax"))
    {
        QStringList list = data.split(" ");
        QString uptimeMax;
        if(!list.isEmpty())
        {
            uptimeMax = list.at(1).toLocal8Bit().data();
        }else
        {
            uptimeMax = softVersion;
        }
        ui->label_uptime->setText("uptimeMax");
        ui->lineEdit_VersionAndRunCnt->setText(uptimeMax);

        if(data.contains("time is ok"))
        {
            QString path = BOXV3_QT_PICTURE_LIGHT_GREEN;
            QPixmap img(path);
            QPixmap imgScaled = img.scaled(QSize(BOXV3_QT_PICTURE_SCALED_SIZE, BOXV3_QT_PICTURE_SCALED_SIZE));
            ui->label_light_keyPressed->setPixmap(imgScaled);
            ui->label_light_keyPressed->resize(imgScaled.width(),imgScaled.height());

            ui->label_checkStatus->setText(QString("<h2>老化时间已满足<h2>"));
        }
        else
        {

        }

    }else
    {
        //ui->textEdit_notes->setText(data);
        ui->textEdit_notes->append(data);
    }
}



void MainWindow::slotDisplayBarCode(char status, QByteArray barArray)
{
    barCodeGunScanResult_enum barCodeStatus = (barCodeGunScanResult_enum)status;
    QFont ft;
    ft.setPointSize(36);
    ui->lineEdit_serialNum->setFont(ft);

    QPalette pa;
    pa.setColor(QPalette::Text, Qt::black);
    ui->lineEdit_serialNum->setPalette(pa);

    if(SCAN_AFTER_KEY_PRESSED != barCodeStatus)
        ui->lineEdit_serialNum->setText(QString(barArray));

}

void MainWindow::slotDisplayCheckout(QByteArray checkout)
{
    results_info_t info;
    memcpy(&info, checkout.data(), checkout.size());
    QString path;
    QPixmap img, imgScaled;

    //0. recorder generalThread if start

    //1. display light for wifi
    if(NOTCHECK == info.wifi)
    {
        path = BOXV3_QT_PICTURE_LIGHT_YELLOW;
    }else
    {
        slotShowWifiIp();

        if(WELLCHECK == info.wifi) path = BOXV3_QT_PICTURE_LIGHT_GREEN;
        else path = BOXV3_QT_PICTURE_LIGHT_RED;
    }
    img = path;
    imgScaled = img.scaled(QSize(BOXV3_QT_PICTURE_LIGHT_SIZE, BOXV3_QT_PICTURE_LIGHT_SIZE));
    ui->label_lightpic_wifi->setPixmap(imgScaled);
    ui->label_lightpic_wifi->resize(imgScaled.width(), imgScaled.height());

    //2. ntpclient
    if(WELLCHECK == info.ntpdate) path = BOXV3_QT_PICTURE_LIGHT_GREEN;
    else if(NOTCHECK == info.ntpdate) path = BOXV3_QT_PICTURE_LIGHT_YELLOW;
    else path = BOXV3_QT_PICTURE_LIGHT_RED;
    img = path;
    imgScaled = img.scaled(QSize(BOXV3_QT_PICTURE_LIGHT_SIZE, BOXV3_QT_PICTURE_LIGHT_SIZE));
    ui->label_lightpic_ntpClient->setPixmap(imgScaled);
    ui->label_lightpic_ntpClient->resize(imgScaled.width(), imgScaled.height());

    //3. rtc
    if(WELLCHECK == info.rtc) path = BOXV3_QT_PICTURE_LIGHT_GREEN;
    else if(NOTCHECK == info.rtc) path = BOXV3_QT_PICTURE_LIGHT_YELLOW;
    else path = BOXV3_QT_PICTURE_LIGHT_RED;
    img = path;
    imgScaled = img.scaled(QSize(BOXV3_QT_PICTURE_LIGHT_SIZE, BOXV3_QT_PICTURE_LIGHT_SIZE));
    ui->label_lightpic_rtc->setPixmap(imgScaled);
    ui->label_lightpic_rtc->resize(imgScaled.width(), imgScaled.height());

    //4. systime
    if(WELLCHECK == info.sysTime)
    {
        slotShowSystemDataAndTime();
    }
    //5. uart232
    if((WELLEVENT1 | WELLEVENT2) == info.uart232) path = BOXV3_QT_PICTURE_LIGHT_GREEN;
    else if(NOTCHECK == info.uart232) path = BOXV3_QT_PICTURE_LIGHT_YELLOW;
    else path = BOXV3_QT_PICTURE_LIGHT_RED;
    img = path;
    imgScaled = img.scaled(QSize(BOXV3_QT_PICTURE_LIGHT_SIZE, BOXV3_QT_PICTURE_LIGHT_SIZE));
    ui->label_lightpic_uart232->setPixmap(imgScaled);
    ui->label_lightpic_uart232->resize(imgScaled.width(), imgScaled.height());

    //6. spi-flash
    if(WELLCHECK == info.spi0Flash) path = BOXV3_QT_PICTURE_LIGHT_GREEN;
    else if(NOTCHECK == info.spi0Flash) path = BOXV3_QT_PICTURE_LIGHT_YELLOW;
    else path = BOXV3_QT_PICTURE_LIGHT_RED;
    img = path;
    imgScaled = img.scaled(QSize(BOXV3_QT_PICTURE_LIGHT_SIZE, BOXV3_QT_PICTURE_LIGHT_SIZE));
    ui->label_lightpic_flash->setPixmap(imgScaled);
    ui->label_lightpic_flash->resize(imgScaled.width(), imgScaled.height());

    //7. spi-lora
    if(WELLCHECK == info.spi1Lora) path = BOXV3_QT_PICTURE_LIGHT_GREEN;
    else if(NOTCHECK == info.spi1Lora) path = BOXV3_QT_PICTURE_LIGHT_YELLOW;
    else path = BOXV3_QT_PICTURE_LIGHT_RED;
    img = path;
    imgScaled = img.scaled(QSize(BOXV3_QT_PICTURE_LIGHT_SIZE, BOXV3_QT_PICTURE_LIGHT_SIZE));
    ui->label_lightpic_lora->setPixmap(imgScaled);
    ui->label_lightpic_lora->resize(imgScaled.width(), imgScaled.height());

    //8. at88sc104
    if(WELLCHECK == info.iicAt88sc104c) path = BOXV3_QT_PICTURE_LIGHT_GREEN;
    else if(NOTCHECK == info.iicAt88sc104c) path = BOXV3_QT_PICTURE_LIGHT_YELLOW;
    else path = BOXV3_QT_PICTURE_LIGHT_RED;
    img = path;
    imgScaled = img.scaled(QSize(BOXV3_QT_PICTURE_LIGHT_SIZE, BOXV3_QT_PICTURE_LIGHT_SIZE));
    ui->label_lightpic_at88->setPixmap(imgScaled);
    ui->label_lightpic_at88->resize(imgScaled.width(), imgScaled.height());

    //9. stm8
    if(WELLCHECK == info.uart2Stm8) path = BOXV3_QT_PICTURE_LIGHT_GREEN;
    else if(NOTCHECK == info.uart2Stm8) path = BOXV3_QT_PICTURE_LIGHT_YELLOW;
    else path = BOXV3_QT_PICTURE_LIGHT_RED;
    img = path;
    imgScaled = img.scaled(QSize(BOXV3_QT_PICTURE_LIGHT_SIZE, BOXV3_QT_PICTURE_LIGHT_SIZE));
    ui->label_lightpic_stm8->setPixmap(imgScaled);
    ui->label_lightpic_stm8->resize(imgScaled.width(), imgScaled.height());

    //10. 4g
    if(WELLCHECK == info.network4G) path = BOXV3_QT_PICTURE_LIGHT_GREEN;
    else if(NOTCHECK == info.network4G) path = BOXV3_QT_PICTURE_LIGHT_YELLOW;
    else path = BOXV3_QT_PICTURE_LIGHT_RED;
    img = path;
    imgScaled = img.scaled(QSize(BOXV3_QT_PICTURE_LIGHT_SIZE, BOXV3_QT_PICTURE_LIGHT_SIZE));
    ui->label_lightpic_4g->setPixmap(imgScaled);
    ui->label_lightpic_4g->resize(imgScaled.width(), imgScaled.height());

    //11. Wan0
    if(WELLCHECK == info.networkWan) path = BOXV3_QT_PICTURE_LIGHT_GREEN;
    else if(NOTCHECK == info.networkWan) path = BOXV3_QT_PICTURE_LIGHT_YELLOW;
    else path = BOXV3_QT_PICTURE_LIGHT_RED;
    img = path;
    imgScaled = img.scaled(QSize(BOXV3_QT_PICTURE_LIGHT_SIZE, BOXV3_QT_PICTURE_LIGHT_SIZE));
    ui->label_lightpic_wan0->setPixmap(imgScaled);
    ui->label_lightpic_wan0->resize(imgScaled.width(), imgScaled.height());

    //Lan1
    if(WELLCHECK == info.networkLanPort1) path = BOXV3_QT_PICTURE_LIGHT_GREEN;
    else if(NOTCHECK == info.networkLanPort1) path = BOXV3_QT_PICTURE_LIGHT_YELLOW;
    else path = BOXV3_QT_PICTURE_LIGHT_RED;
    img = path;
    imgScaled = img.scaled(QSize(BOXV3_QT_PICTURE_LIGHT_SIZE, BOXV3_QT_PICTURE_LIGHT_SIZE));
    ui->label_lightpic_lan1->setPixmap(imgScaled);
    ui->label_lightpic_lan1->resize(imgScaled.width(), imgScaled.height());
    //Lan2
    if(WELLCHECK == info.networkLanPort2) path = BOXV3_QT_PICTURE_LIGHT_GREEN;
    else if(NOTCHECK == info.networkLanPort2) path = BOXV3_QT_PICTURE_LIGHT_YELLOW;
    else path = BOXV3_QT_PICTURE_LIGHT_RED;
    img = path;
    imgScaled = img.scaled(QSize(BOXV3_QT_PICTURE_LIGHT_SIZE, BOXV3_QT_PICTURE_LIGHT_SIZE));
    ui->label_lightpic_lan2->setPixmap(imgScaled);
    ui->label_lightpic_lan2->resize(imgScaled.width(), imgScaled.height());
    //Lan3
    if(WELLCHECK == info.networkLanPort3) path = BOXV3_QT_PICTURE_LIGHT_GREEN;
    else if(NOTCHECK == info.networkLanPort3) path = BOXV3_QT_PICTURE_LIGHT_YELLOW;
    else path = BOXV3_QT_PICTURE_LIGHT_RED;
    img = path;
    imgScaled = img.scaled(QSize(BOXV3_QT_PICTURE_LIGHT_SIZE, BOXV3_QT_PICTURE_LIGHT_SIZE));
    ui->label_lightpic_lan3->setPixmap(imgScaled);
    ui->label_lightpic_lan3->resize(imgScaled.width(), imgScaled.height());
    //Lan4
    if(WELLCHECK == info.networkLanPort4) path = BOXV3_QT_PICTURE_LIGHT_GREEN;
    else if(NOTCHECK == info.networkLanPort4) path = BOXV3_QT_PICTURE_LIGHT_YELLOW;
    else path = BOXV3_QT_PICTURE_LIGHT_RED;
    img = path;
    imgScaled = img.scaled(QSize(BOXV3_QT_PICTURE_LIGHT_SIZE, BOXV3_QT_PICTURE_LIGHT_SIZE));
    ui->label_lightpic_lan4->setPixmap(imgScaled);
    ui->label_lightpic_lan4->resize(imgScaled.width(), imgScaled.height());

    //final result
    switch(info.whole)
    {
    case WELLCHECK:
    {
        if(WELLCHECK == tcpDone)
        {
            path = BOXV3_QT_PICTURE_LIGHT_GREEN;
        }else
        {
            path = BOXV3_QT_PICTURE_LIGHT_YELLOW;
        }
        break;
    }
    case FAILEDCHECK:
        path = BOXV3_QT_PICTURE_LIGHT_RED;
        break;
    default:
        path = BOXV3_QT_PICTURE_LIGHT_YELLOW;
        break;
    }
    img = path;
    imgScaled = img.scaled(QSize(BOXV3_QT_PICTURE_SCALED_SIZE, BOXV3_QT_PICTURE_SCALED_SIZE));
    ui->label_light_keyPressed->setPixmap(imgScaled);
    ui->label_light_keyPressed->resize(imgScaled.width(),imgScaled.height());
}

void MainWindow::slotDisplayAgingMonitorResult(QByteArray checkout, QByteArray msg)
{
    results_info_t info;
    memcpy(&info, checkout.data(), checkout.size());
    QString path;
    QPixmap img, imgScaled;

    //Aging result
    if(WELLCHECK == info.agingResult) path = BOXV3_QT_PICTURE_LIGHT_GREEN;
    else if(NOTCHECK == info.agingResult) path = BOXV3_QT_PICTURE_LIGHT_YELLOW;
    else path = BOXV3_QT_PICTURE_LIGHT_RED;
    img = path;
    imgScaled = img.scaled(QSize(BOXV3_QT_PICTURE_LIGHT_SIZE, BOXV3_QT_PICTURE_LIGHT_SIZE));


    ui->label_lightpic_agingResult->setVisible(true);
    ui->label_light_aging->setVisible(true);

    ui->label_lightpic_agingResult->setPixmap(imgScaled);
    ui->label_lightpic_agingResult->resize(imgScaled.width(), imgScaled.height());

    ui->label_light_aging->setText("Aging: " + msg);
}

void MainWindow::slotAllPhyCheckedDone(char result)
{

    QString path(BOXV3_QT_PICTURE_LIGHT_YELLOW);

    QPalette pa;

    if(WELLCHECK == result)
    {
        if(WELLCHECK == tcpDone)
        {
            path = BOXV3_QT_PICTURE_LIGHT_GREEN;
        }else
        {
            path = BOXV3_QT_PICTURE_LIGHT_YELLOW;
        }
        pa.setColor(QPalette::Text, Qt::black);
        ui->textEdit_notes->setPalette(pa);
    }
    else
    {
        path = BOXV3_QT_PICTURE_LIGHT_RED;
        pa.setColor(QPalette::Text, Qt::red);
        ui->textEdit_notes->setPalette(pa);
    }

    //make the press light up to green to notice somebody check have done
    QPixmap img(path);
    QPixmap imgScaled = img.scaled(QSize(BOXV3_QT_PICTURE_SCALED_SIZE, BOXV3_QT_PICTURE_SCALED_SIZE));
    ui->label_light_keyPressed->setPixmap(imgScaled);
    ui->label_light_keyPressed->resize(imgScaled.width(),imgScaled.height());
}

void MainWindow::slotTcpTransceiverLogDone()
{
    tcpDone = WELLCHECK;
    QPalette pa;
    pa.setColor(QPalette::Text, Qt::black);
    ui->textEdit_notes->setPalette(pa);    
}
