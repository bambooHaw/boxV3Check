/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.8.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QGroupBox *groupBox;
    QCheckBox *checkBox_deviceNode;
    QCheckBox *checkBox_ltemodule;
    QCheckBox *checkBox_SIMSlot;
    QCheckBox *checkBox_SIMDataService;
    QCheckBox *checkBox_SIMOperator;
    QCheckBox *checkBox_SIMDialing;
    QCheckBox *checkBox_SIMSignal;
    QCheckBox *checkBox_netAccess;
    QLineEdit *lineEdit_deviceNode;
    QLineEdit *lineEdit_LTEmodule;
    QLineEdit *lineEdit_SIMslot;
    QLineEdit *lineEdit_service;
    QLineEdit *lineEdit_operator;
    QLineEdit *lineEdit_dialing;
    QLineEdit *lineEdit_signal;
    QLineEdit *lineEdit_netaccess;
    QLineEdit *lineEdit_temp;
    QCheckBox *checkBox_temp;
    QCheckBox *checkBox_iccid;
    QLineEdit *lineEdit_iccid;
    QLineEdit *lineEdit_currentMode;
    QCheckBox *checkBox_currentMode;
    QCheckBox *checkBox_simSwitch;
    QLineEdit *lineEdit_simSwitch;
    QCheckBox *checkBox_pingResult;
    QLineEdit *lineEdit_pingResult;
    QCheckBox *checkBox_cclk;
    QLineEdit *lineEdit_cclk;
    QCheckBox *checkBox_eons;
    QLineEdit *lineEdit_eons;
    QCheckBox *checkBox_cardmode;
    QLineEdit *lineEdit_cardmode;
    QCheckBox *checkBox_simst;
    QLineEdit *lineEdit_simst;
    QCheckBox *checkBox_cme_error;
    QLineEdit *lineEdit_cme_error;
    QLineEdit *lineEdit_version;
    QGroupBox *groupBox_2;
    QTextEdit *textEdit_info;
    QLineEdit *lineEdit_sysTime;
    QGroupBox *groupBox_3;
    QLineEdit *lineEdit_successCnt;
    QLineEdit *lineEdit_noteSuccess;
    QLineEdit *lineEdit_noteFailed;
    QLineEdit *lineEdit_failedCnt;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        MainWindow->resize(1257, 738);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QStringLiteral("centralwidget"));
        groupBox = new QGroupBox(centralwidget);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        groupBox->setGeometry(QRect(30, 50, 761, 591));
        checkBox_deviceNode = new QCheckBox(groupBox);
        checkBox_deviceNode->setObjectName(QStringLiteral("checkBox_deviceNode"));
        checkBox_deviceNode->setGeometry(QRect(0, 30, 121, 23));
        checkBox_ltemodule = new QCheckBox(groupBox);
        checkBox_ltemodule->setObjectName(QStringLiteral("checkBox_ltemodule"));
        checkBox_ltemodule->setGeometry(QRect(0, 60, 111, 23));
        checkBox_SIMSlot = new QCheckBox(groupBox);
        checkBox_SIMSlot->setObjectName(QStringLiteral("checkBox_SIMSlot"));
        checkBox_SIMSlot->setGeometry(QRect(0, 120, 131, 23));
        checkBox_SIMDataService = new QCheckBox(groupBox);
        checkBox_SIMDataService->setObjectName(QStringLiteral("checkBox_SIMDataService"));
        checkBox_SIMDataService->setGeometry(QRect(0, 150, 151, 23));
        checkBox_SIMOperator = new QCheckBox(groupBox);
        checkBox_SIMOperator->setObjectName(QStringLiteral("checkBox_SIMOperator"));
        checkBox_SIMOperator->setGeometry(QRect(0, 180, 141, 23));
        checkBox_SIMDialing = new QCheckBox(groupBox);
        checkBox_SIMDialing->setObjectName(QStringLiteral("checkBox_SIMDialing"));
        checkBox_SIMDialing->setGeometry(QRect(0, 210, 161, 23));
        checkBox_SIMSignal = new QCheckBox(groupBox);
        checkBox_SIMSignal->setObjectName(QStringLiteral("checkBox_SIMSignal"));
        checkBox_SIMSignal->setGeometry(QRect(0, 270, 141, 23));
        checkBox_netAccess = new QCheckBox(groupBox);
        checkBox_netAccess->setObjectName(QStringLiteral("checkBox_netAccess"));
        checkBox_netAccess->setGeometry(QRect(0, 240, 92, 23));
        lineEdit_deviceNode = new QLineEdit(groupBox);
        lineEdit_deviceNode->setObjectName(QStringLiteral("lineEdit_deviceNode"));
        lineEdit_deviceNode->setGeometry(QRect(140, 30, 601, 25));
        lineEdit_LTEmodule = new QLineEdit(groupBox);
        lineEdit_LTEmodule->setObjectName(QStringLiteral("lineEdit_LTEmodule"));
        lineEdit_LTEmodule->setGeometry(QRect(140, 60, 601, 25));
        lineEdit_SIMslot = new QLineEdit(groupBox);
        lineEdit_SIMslot->setObjectName(QStringLiteral("lineEdit_SIMslot"));
        lineEdit_SIMslot->setGeometry(QRect(140, 120, 601, 25));
        lineEdit_service = new QLineEdit(groupBox);
        lineEdit_service->setObjectName(QStringLiteral("lineEdit_service"));
        lineEdit_service->setGeometry(QRect(140, 150, 601, 25));
        lineEdit_operator = new QLineEdit(groupBox);
        lineEdit_operator->setObjectName(QStringLiteral("lineEdit_operator"));
        lineEdit_operator->setGeometry(QRect(140, 180, 601, 25));
        lineEdit_dialing = new QLineEdit(groupBox);
        lineEdit_dialing->setObjectName(QStringLiteral("lineEdit_dialing"));
        lineEdit_dialing->setGeometry(QRect(140, 210, 601, 25));
        lineEdit_signal = new QLineEdit(groupBox);
        lineEdit_signal->setObjectName(QStringLiteral("lineEdit_signal"));
        lineEdit_signal->setGeometry(QRect(140, 270, 601, 25));
        lineEdit_netaccess = new QLineEdit(groupBox);
        lineEdit_netaccess->setObjectName(QStringLiteral("lineEdit_netaccess"));
        lineEdit_netaccess->setGeometry(QRect(140, 240, 601, 25));
        lineEdit_temp = new QLineEdit(groupBox);
        lineEdit_temp->setObjectName(QStringLiteral("lineEdit_temp"));
        lineEdit_temp->setGeometry(QRect(140, 300, 601, 25));
        checkBox_temp = new QCheckBox(groupBox);
        checkBox_temp->setObjectName(QStringLiteral("checkBox_temp"));
        checkBox_temp->setGeometry(QRect(0, 300, 131, 23));
        checkBox_iccid = new QCheckBox(groupBox);
        checkBox_iccid->setObjectName(QStringLiteral("checkBox_iccid"));
        checkBox_iccid->setGeometry(QRect(0, 330, 131, 23));
        lineEdit_iccid = new QLineEdit(groupBox);
        lineEdit_iccid->setObjectName(QStringLiteral("lineEdit_iccid"));
        lineEdit_iccid->setGeometry(QRect(140, 330, 601, 25));
        lineEdit_currentMode = new QLineEdit(groupBox);
        lineEdit_currentMode->setObjectName(QStringLiteral("lineEdit_currentMode"));
        lineEdit_currentMode->setGeometry(QRect(140, 540, 601, 25));
        checkBox_currentMode = new QCheckBox(groupBox);
        checkBox_currentMode->setObjectName(QStringLiteral("checkBox_currentMode"));
        checkBox_currentMode->setGeometry(QRect(0, 540, 121, 23));
        checkBox_simSwitch = new QCheckBox(groupBox);
        checkBox_simSwitch->setObjectName(QStringLiteral("checkBox_simSwitch"));
        checkBox_simSwitch->setGeometry(QRect(0, 90, 101, 23));
        lineEdit_simSwitch = new QLineEdit(groupBox);
        lineEdit_simSwitch->setObjectName(QStringLiteral("lineEdit_simSwitch"));
        lineEdit_simSwitch->setGeometry(QRect(140, 90, 601, 25));
        checkBox_pingResult = new QCheckBox(groupBox);
        checkBox_pingResult->setObjectName(QStringLiteral("checkBox_pingResult"));
        checkBox_pingResult->setGeometry(QRect(0, 510, 121, 23));
        lineEdit_pingResult = new QLineEdit(groupBox);
        lineEdit_pingResult->setObjectName(QStringLiteral("lineEdit_pingResult"));
        lineEdit_pingResult->setGeometry(QRect(140, 510, 601, 25));
        checkBox_cclk = new QCheckBox(groupBox);
        checkBox_cclk->setObjectName(QStringLiteral("checkBox_cclk"));
        checkBox_cclk->setGeometry(QRect(0, 360, 121, 23));
        lineEdit_cclk = new QLineEdit(groupBox);
        lineEdit_cclk->setObjectName(QStringLiteral("lineEdit_cclk"));
        lineEdit_cclk->setGeometry(QRect(140, 360, 601, 25));
        checkBox_eons = new QCheckBox(groupBox);
        checkBox_eons->setObjectName(QStringLiteral("checkBox_eons"));
        checkBox_eons->setGeometry(QRect(0, 390, 121, 23));
        lineEdit_eons = new QLineEdit(groupBox);
        lineEdit_eons->setObjectName(QStringLiteral("lineEdit_eons"));
        lineEdit_eons->setGeometry(QRect(140, 390, 601, 25));
        checkBox_cardmode = new QCheckBox(groupBox);
        checkBox_cardmode->setObjectName(QStringLiteral("checkBox_cardmode"));
        checkBox_cardmode->setGeometry(QRect(0, 420, 121, 23));
        lineEdit_cardmode = new QLineEdit(groupBox);
        lineEdit_cardmode->setObjectName(QStringLiteral("lineEdit_cardmode"));
        lineEdit_cardmode->setGeometry(QRect(140, 420, 601, 25));
        checkBox_simst = new QCheckBox(groupBox);
        checkBox_simst->setObjectName(QStringLiteral("checkBox_simst"));
        checkBox_simst->setGeometry(QRect(0, 450, 92, 23));
        lineEdit_simst = new QLineEdit(groupBox);
        lineEdit_simst->setObjectName(QStringLiteral("lineEdit_simst"));
        lineEdit_simst->setGeometry(QRect(140, 450, 601, 25));
        checkBox_cme_error = new QCheckBox(groupBox);
        checkBox_cme_error->setObjectName(QStringLiteral("checkBox_cme_error"));
        checkBox_cme_error->setGeometry(QRect(0, 480, 111, 23));
        lineEdit_cme_error = new QLineEdit(groupBox);
        lineEdit_cme_error->setObjectName(QStringLiteral("lineEdit_cme_error"));
        lineEdit_cme_error->setGeometry(QRect(140, 480, 601, 25));
        lineEdit_version = new QLineEdit(centralwidget);
        lineEdit_version->setObjectName(QStringLiteral("lineEdit_version"));
        lineEdit_version->setGeometry(QRect(800, 51, 201, 25));
        groupBox_2 = new QGroupBox(centralwidget);
        groupBox_2->setObjectName(QStringLiteral("groupBox_2"));
        groupBox_2->setGeometry(QRect(800, 160, 441, 471));
        textEdit_info = new QTextEdit(groupBox_2);
        textEdit_info->setObjectName(QStringLiteral("textEdit_info"));
        textEdit_info->setGeometry(QRect(0, 20, 421, 441));
        lineEdit_sysTime = new QLineEdit(centralwidget);
        lineEdit_sysTime->setObjectName(QStringLiteral("lineEdit_sysTime"));
        lineEdit_sysTime->setGeometry(QRect(800, 101, 201, 25));
        groupBox_3 = new QGroupBox(centralwidget);
        groupBox_3->setObjectName(QStringLiteral("groupBox_3"));
        groupBox_3->setGeometry(QRect(1020, 50, 201, 81));
        lineEdit_successCnt = new QLineEdit(groupBox_3);
        lineEdit_successCnt->setObjectName(QStringLiteral("lineEdit_successCnt"));
        lineEdit_successCnt->setGeometry(QRect(70, 20, 113, 25));
        lineEdit_noteSuccess = new QLineEdit(groupBox_3);
        lineEdit_noteSuccess->setObjectName(QStringLiteral("lineEdit_noteSuccess"));
        lineEdit_noteSuccess->setGeometry(QRect(10, 20, 51, 25));
        lineEdit_noteFailed = new QLineEdit(groupBox_3);
        lineEdit_noteFailed->setObjectName(QStringLiteral("lineEdit_noteFailed"));
        lineEdit_noteFailed->setGeometry(QRect(10, 50, 61, 25));
        lineEdit_failedCnt = new QLineEdit(groupBox_3);
        lineEdit_failedCnt->setObjectName(QStringLiteral("lineEdit_failedCnt"));
        lineEdit_failedCnt->setGeometry(QRect(70, 50, 113, 25));
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QStringLiteral("menubar"));
        menubar->setGeometry(QRect(0, 0, 1257, 22));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QStringLiteral("statusbar"));
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", Q_NULLPTR));
        groupBox->setTitle(QApplication::translate("MainWindow", "ME909S-821 check", Q_NULLPTR));
        checkBox_deviceNode->setText(QApplication::translate("MainWindow", "deviceNode", Q_NULLPTR));
        checkBox_ltemodule->setText(QApplication::translate("MainWindow", "LTEmodule", Q_NULLPTR));
        checkBox_SIMSlot->setText(QApplication::translate("MainWindow", "SIMSlot", Q_NULLPTR));
        checkBox_SIMDataService->setText(QApplication::translate("MainWindow", "SIMDataService", Q_NULLPTR));
        checkBox_SIMOperator->setText(QApplication::translate("MainWindow", "SIMOperator", Q_NULLPTR));
        checkBox_SIMDialing->setText(QApplication::translate("MainWindow", "SIMDialing", Q_NULLPTR));
        checkBox_SIMSignal->setText(QApplication::translate("MainWindow", "SIMSignal", Q_NULLPTR));
        checkBox_netAccess->setText(QApplication::translate("MainWindow", "NetAccess", Q_NULLPTR));
        checkBox_temp->setText(QApplication::translate("MainWindow", "temperature", Q_NULLPTR));
        checkBox_iccid->setText(QApplication::translate("MainWindow", "ICCID", Q_NULLPTR));
        lineEdit_currentMode->setText(QString());
        checkBox_currentMode->setText(QApplication::translate("MainWindow", "currentMode", Q_NULLPTR));
        checkBox_simSwitch->setText(QApplication::translate("MainWindow", "SIMSwitch", Q_NULLPTR));
        checkBox_pingResult->setText(QApplication::translate("MainWindow", "pingResult", Q_NULLPTR));
        checkBox_cclk->setText(QApplication::translate("MainWindow", "CCLK", Q_NULLPTR));
        checkBox_eons->setText(QApplication::translate("MainWindow", "EONS", Q_NULLPTR));
        checkBox_cardmode->setText(QApplication::translate("MainWindow", "CARDMODE", Q_NULLPTR));
        checkBox_simst->setText(QApplication::translate("MainWindow", "SIMST", Q_NULLPTR));
        checkBox_cme_error->setText(QApplication::translate("MainWindow", "CMEERROR", Q_NULLPTR));
        lineEdit_version->setText(QApplication::translate("MainWindow", "Version", Q_NULLPTR));
        groupBox_2->setTitle(QApplication::translate("MainWindow", "Notes:", Q_NULLPTR));
        lineEdit_sysTime->setText(QApplication::translate("MainWindow", "sysTime", Q_NULLPTR));
        groupBox_3->setTitle(QApplication::translate("MainWindow", "Counter:", Q_NULLPTR));
        lineEdit_successCnt->setText(QApplication::translate("MainWindow", "successCnt", Q_NULLPTR));
        lineEdit_noteSuccess->setText(QApplication::translate("MainWindow", "OK:", Q_NULLPTR));
        lineEdit_noteFailed->setText(QApplication::translate("MainWindow", "Failed:", Q_NULLPTR));
        lineEdit_failedCnt->setText(QApplication::translate("MainWindow", "failedCnt", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
