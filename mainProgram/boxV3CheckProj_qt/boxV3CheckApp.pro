#-------------------------------------------------
#
# Project created by QtCreator 2015-07-15T10:43:53
#
#-------------------------------------------------

QT       += core gui network
#QT       += serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = boxV3CheckApp
TEMPLATE = app


SOURCES += main.cpp\
    myping.c \
    objectat88sc104c.cpp \
    objectuart.cpp \
    objecttcpdumpkiller.cpp \
    threadkeydetection.cpp \
    threadlogrecorder.cpp \
    threadbarcodegun.cpp \
    threadtcpserver.cpp \
    threadgeneralexamination.cpp \
    mainwindow.cpp \
    threadagingmonitor.cpp \
    threadled.cpp \
    threadudpbroadcast.cpp \
    boxv3.cpp \
    ztboxc_deprecated.c

HEADERS  += \
    boxv3.h \
    objectat88sc104c.h \
    tcpzhitongbox.h \
    udpzhitongbox.h \
    objectuart.h \
    objecttcpdumpkiller.h \
    threadkeydetection.h \
    threadlogrecorder.h \
    threadbarcodegun.h \
    threadtcpserver.h \
    threadgeneralexamination.h \
    mainwindow.h \
    threadagingmonitor.h \
    threadled.h \
    threadudpbroadcast.h \
    ztboxc_deprecated.h

FORMS    += \
    mainwindow.ui
