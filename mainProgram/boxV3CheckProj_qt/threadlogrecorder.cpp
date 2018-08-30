#include "threadlogrecorder.h"

threadLogRecorder::threadLogRecorder()
{
    proLogCreated = false;
    agingLogCreated = false;
    agedLogCreated = false;

    QString cmd("rm -rf ");
    cmd.append(BOXV3_SELFCHECK_LOGDIR_PATH);
    cmd.append(BOXV3_SELFCHECK_TMPLOG);
    system(cmd.toLocal8Bit().data());

    initNoteMap();

    moveToThread(this);
}



void threadLogRecorder::initNoteMap()
{
    //production check notes key-val

    //bar
    noteMap.insert("bar0", "<h2>使用该软件前，应按如下流程，配置扫码枪:</h2>\
                   \n1）初始化设置：请扫描“初始化设置”条形码;\
                   \n2）接口输出格式： 请扫描“串口模式”条形码.\
                   \n3）结束符设置:\
                   \n\ta）请扫描“取消后缀”条形码;\
                   \n\tb）请扫描“加后缀-回车”条形码;\
                   \n4）串口波特率设置： 请扫描“115200”条形码;\
                   \n5）触发模式设置： 请扫描“单一触发模式”条形码;\
                   \n6）其他设置：\
                   \n\ta）如果您对扫码枪的声音感到烦躁，请扫描“关闭声音”条形码；\
                   \n\tb）但强烈建议扫描“开启声音”条形码，以使检测更顺利。\
                    \n\n<h2>如果成功设置，即可扫描一般设置的条形码，进行检测环节了.</h2>\
                    \n\ta）成功设置后，扫描一般类型的条形码，条形码将直接显示在屏幕上。");
    noteMap.insert("bar1", "请确认该序列号是否正确；\
                    \n1)正确: 按一下按键，启动生产自检.\
                    \n2)不正确: 扫描新序列号；\
                    \n注意：\
                    \n1) 如果不能扫描，可能是扫码枪配置不正确（请重新扫描，或者重新配置扫码枪）；也可能是设备串口不好用（请送予返修）；\
                    \n2) 在扫描到序列号后，一旦按下按键，扫描枪的功能将会被关闭！");

    //wifi stage
    noteMap.insert("env0", "正在配置网络，请稍后...\
                    \n注意：\
                    \n\t如果WiFi(ip)显示栏长时间没有显示ip地址，则需要重启设备，重新完成检测.");


    //aging check notes key-val
    noteMap.insert("aging0", "老化中监测已经开始......\
                    \n随时可以使用扫码枪扫描产品序列号，然后使用按键，启动老化后检测！");


    //result
    noteMap.insert("success0", "检测已经结束！\
                   \n结果：合格！\
                   \n剩余工作：\
                   \n\t请不要断电，耐心等待设备和上位机完成信息交互......");
    noteMap.insert("allok", "<h1>辛苦您了，设备已经合格，可以出库啦！</h1>");
    noteMap.insert("failed0", "检测已经结束！\
                   \n结果：不合格！\
                   \n建议：\
                   \n\t1）断电重启，再次检测!\
                   \n\t2）或联系厂商");
   noteMap.insert("failed1", "检测超时！！！\
                  \n\n\n检测已经结束！\
                  \n结果：不合格！\
                  \n建议：\
                  \n\t1）断电重启，再次检测!\
                  \n\t2）或联系厂商");
    //udp0
    noteMap.insert("udp0", "已经传输WiFi IP地址到上位机，请等待上位机操作");
    //tcp0
    noteMap.insert("tcpPro", "辛苦您了，上位机已经成功导出了日志，本次生产检测已经结束。\n此产品可以进入下一个检测环节了。");
    noteMap.insert("tcpAged", "辛苦您了，上位机已经成功导出了日志，本次老化检测已经结束。\n此产品可以可以出库了。");


    //warning
    noteMap.insert("warning2", "<h1>条形码和生产检测条形码不匹配</h1><span style=\"color:red\">\n\n请重新扫描或者联系条形码提供商</span>");


    //stm8
    noteMap.insert("stm8opt0", "<span style=\"color:red\">单片机版本正在更新，请稍后...</span>");
    noteMap.insert("stm8opt1", "单片机软件更新完成");
    noteMap.insert("stm8opt2", "<span style=\"color:red\">错误：没有固件可用来单片机版本更新</span>");
    noteMap.insert("stm8opt3", "<span style=\"color:red\">错误：单片机版本更新失败</span>");

    /*GENERAL*/
    //G1. status
    noteMap.insert("status pro", "<h1>生产检测</h1>");
    noteMap.insert("status aging", "<h1>老化监测中</h1>");
    noteMap.insert("status aged", "<h1>老化后检测</h1>");
    noteMap.insert("status allCheckedOk", "<h1>设备合格</h1>");
    //G2. info production
    noteMap.insert("info pro0", "生产检测已经开始，请耐心等待检测结果!");
    //G3. info aged
    noteMap.insert("info aged0", "老化后检测已经开始，请耐心等待检测结果!");

    /*success*/
    noteMap.insert("success wifiCheck", "WiFi检测: 成功");


    /*WARNING*/
    noteMap.insert("warning at880", "<span style=\"color:red\">访问加密芯片出现错误</span>");
    noteMap.insert("warning keyPress0", "<span style=\"color:red\">检测已经开始，请耐心等待检测结束，不要重复按......</span>");
    noteMap.insert("warning keyPress1", "<span style=\"color:red\">按键无效！</span>");
    noteMap.insert("warning barCode0", "<span style=\"color:red\">无效条形码，请重新扫描或者联系条形码提供商</span>");
    noteMap.insert("warning barCode1", "<span style=\"color:red\">一个序列号正在使用中，本次扫描无效！</span>");
    noteMap.insert("warning stage w0", "<span style=\"color:red\">未知检测阶段</span>");
    /*ERROR*/
    noteMap.insert("error unknown0", "<span style=\"color:red\">发生了一个未知错误！</span>");
    noteMap.insert("error at88 EACCES", "<span style=\"color:red\">没有权限执行自检</span>");
    noteMap.insert("error at88 ENXIO", "<span style=\"color:red\">无法从加密芯片中读取序列号</span>");
    noteMap.insert("error uart485 err0", "<span style=\"color:red\">扫码枪串口（uart485）不能使用</span>");
    noteMap.insert("error button err0", "<span style=\"color:red\">按键不能使用</span>");
    noteMap.insert("error aged err0", "<span style=\"color:red\">老化后测试无法进行</span>");
    noteMap.insert("error status default prod", "<span style=\"color:red\">检测阶段判断失败，默认进行生产检测</span>");
    noteMap.insert("error wifiCheck", "<span style=\"color:red\">WiFi检测：失败</span>");
    noteMap.insert("error udpTimeout", "<span style=\"color:red\">UDP等待超时，请注意保持网络畅通</span>");
    noteMap.insert("error tcpTimeout", "<span style=\"color:red\">TCP等待超时，请注意保持网络畅通</span>");




}


char *threadLogRecorder::getCurrentTime()
{
    static char buf[TIME_BUF_LENGTH] = {};

    QString qStr = QDateTime::currentDateTime().toString("yyyy/MM/dd-HH:mm:ss");

    bzero(buf, TIME_BUF_LENGTH);
    strncpy(buf, qStr.toStdString().c_str(), qStr.length());
    //qDebug() << "Get time: " << qStr.toStdString().c_str() << "len:" << qStr.length();
    if(strlen(buf) >= TIME_BUF_LENGTH)
        buf[sizeof(buf) - 1] = '\0';
    else
        buf[strlen(buf)] = '\0';

    return buf;
}

QString threadLogRecorder::createLogFile(checkflag_enum stage = SELFCHECK_PRODUCTION)
{
    qDebug("stage: %d", stage);
    //get log path
    QString logPath((char*)BOXV3_SELFCHECK_LOGDIR_PATH);
    switch(stage)
    {
    case SELFCHECK_PRODUCTION:
    {
        logPath += BOXV3_SELFCHECK_DIR_PREFIX_PRO;
        break;
    }
    case SELFCHECK_AGING:
    {
        logPath += BOXV3_SELFCHECK_DIR_PREFIX_AGING;
        break;
    }
    case SELFCHECK_AGED:
    {
        logPath += BOXV3_SELFCHECK_DIR_PREFIX_AFTERAGING;
        break;
    }
    default:
    {
        qDebug() << "Error: wrong log recorder.";
        return QString((char*)"");
    }
    }

    QDir logDir(logPath);
    if(!logDir.exists())
    {
        if(!logDir.mkpath(logPath))
        {
            qDebug("Error: Create log dir(%s) failed.", logPath.toLocal8Bit().data());
        }
    }
    QStringList filters;
    filters.append((char*)"*");
    filters.append((char*)BOXV3_SELFCHECK_LOGNAME_SUFFIX);
    logDir.setNameFilters(filters);
    QFileInfoList list = logDir.entryInfoList();


#if 0
    //made a new log name by current systime
    QString logFileNameTmp = QDateTime::currentDateTime().toString("yyyy_MM_dd[HH:mm:ss]");
//    logFileNameTmp.replace(" ", "_");
//    logFileNameTmp.replace("-", "_");
//    logFileNameTmp.replace("/", "_");
//    logFileNameTmp.replace(":", "_");
    logFileNameTmp.append((char*)BOXV3_SELFCHECK_LOGNAME_SUFFIX);

    if(0 != list.length())
    {
        int index = 0;
        forever
        {
            int i=0;
            //compare the other logs' name in the log dir
            for(i = 0; i < list.size(); i++)
            {
                if(0 == list.at(i).fileName().compare(logFileNameTmp)) break;
            }
            //if find someone is equal to new log name, add the index and compare again
            if(i<list.size())
            {
                logFileNameTmp.insert(logFileNameTmp.length()-1-sizeof(BOXV3_SELFCHECK_LOGNAME_SUFFIX), QString::number(index, 10));
                index++;
                continue;
            }else
            {
                //get a newly log name
                break;
            }
        }
    }

    //create the new logFile
    //enter log dir
    QString logFullName(logPath);
    logFullName += logFileNameTmp;
    QFile logFile;
    logFile.setFileName(logFullName);
    if(!logFile.open(QIODevice::ReadWrite|QIODevice::Text))
    {
        qDebug("Error: open logFile(%s) failed.", logFullName.toLocal8Bit().data());
        return QString((char*)"");
    }else
    {
        qDebug("Success: logFile(%s) has been created!", logFullName.toLocal8Bit().data());
        logFile.close();
    }
#else
    //made a new log name by max index + 1
    int indexMax = 0;

    if(0 != list.length())
    {
        for(int i = 0; i<list.size(); i++)
        {
            QString indexStr = list.at(i).fileName().section('.', 0, 0, QString::SectionSkipEmpty);
            bool ok;
            int index = indexStr.toInt(&ok, 10);
            if(index > indexMax) indexMax = index;
        }
        indexMax += 1;
    }

    QString logFileNameTmp = QString::number(indexMax, 10);
    logFileNameTmp.append((char*)BOXV3_SELFCHECK_LOGNAME_SUFFIX);



    //create the new logFile
    //enter log dir
    QString logFullName(logPath);
    logFullName += logFileNameTmp;
    QFile logFile;
    logFile.setFileName(logFullName);
    if(!logFile.open(QIODevice::ReadWrite|QIODevice::Text))
    {
        qDebug("Error: open logFile(%s) failed.", logFullName.toLocal8Bit().data());
        return QString((char*)"");
    }else
    {
        qDebug("Success: logFile(%s) has been created!", logFullName.toLocal8Bit().data());
        logFile.close();
    }

#endif
    return logFullName;
}

void threadLogRecorder::deleteCurrentAgingLogFile()
{
    QByteArray cmdArray("rm -rf ");
    cmdArray.append(currentLogFile);
    system(cmdArray.data());
}

void threadLogRecorder::slotGetLogFile()
{
    QMutexLocker logLock(&logMutex);
    currentLogFile = createLogFile(currentCheckStage).toLocal8Bit();
    emit signalSendLogName(currentLogFile);
}
void threadLogRecorder::slotWriteLogItem(QString logArray)
{

    QByteArray log("echo \"");
    log.append("[");
    log.append(getCurrentTime());
    log.append("]");
    log.append(logArray.toLocal8Bit());
    log.append("\" >> ");

    QMutexLocker logLock(&logMutex);
    if(currentLogFile.isEmpty())
    {
        log.append(BOXV3_SELFCHECK_LOGDIR_PATH);
        log.append(BOXV3_SELFCHECK_TMPLOG);
    }else
    {
        log.append(currentLogFile.data());
    }

    //qDebug("---Debug: log data: %s", log.data());
    system(log.data());
}

void threadLogRecorder::slotRecorder(char level, QString key, QString data)
{

    if(!key.isEmpty())
    {
        if(noteMap.contains(key))
        {
            QMap<QString,QString>::iterator it = noteMap.find(key);
            data = it.value();
        }
    }

    switch(level)
    {
    case LOG_ALERT:
    {
        if(key.contains("status pro"))
        {
            if(!proLogCreated)
            {
                proLogCreated = true;
                slotGetCheckStageAndCreateNewLogFile(SELFCHECK_PRODUCTION);
            }
        }
        if(key.contains("status aging"))
        {
            if(!agingLogCreated)
            {
                agingLogCreated = true;
                slotGetCheckStageAndCreateNewLogFile(SELFCHECK_AGING);
            }
        }
        if(key.contains("status aged"))
        {
            if(!agedLogCreated)
            {
                agedLogCreated = true;
                slotGetCheckStageAndCreateNewLogFile(SELFCHECK_AGED);
            }
        }

        if(key.contains("currentMaxUptime"))
        {
            emit signalDisplayNotes(key, data);
            break;
        }

        if(key.contains("uptimeMax"))
        {
            emit signalDisplayNotes(key, data);
            break;
        }

    }
    case LOG_CRIT:
    {
        slotWriteLogItem(data);
    }
    case LOG_NOTICE:
    {
        emit signalDisplayNotes(key, data);
    }
    case LOG_INFO:
    case LOG_DEBUG:
    default:
    {
        qDebug("%s:%s\n", key.toLocal8Bit().data(), data.toLocal8Bit().data());
        break;
    }
    }
}

void threadLogRecorder::slotGetCheckStageAndCreateNewLogFile(checkflag_enum status)
{
    currentCheckStage = status;

    //delete current aging logfile and stop the thread, when aged check requested at this moment
    if(SELFCHECK_AGED == currentCheckStage)
    {
        deleteCurrentAgingLogFile();
        emit signalStopAgingCheck();
    }

    //update the logfile name
    slotGetLogFile();

    //start matching thread
    switch(currentCheckStage)
    {
    case SELFCHECK_PRODUCTION:
    {
        emit signalStartProductionCheck();
        break;
    }
    case SELFCHECK_AGING:
    {
        emit signalStartAgingCheck();
        break;
    }
    case SELFCHECK_AGED:
    {
        emit signalStartAfterAgingCheck();
        break;
    }
    default:
    {
        qDebug("Error: get a wrong check stage argument.");
        break;
    }
    }

}

void threadLogRecorder::run()
{
    exec();
}

