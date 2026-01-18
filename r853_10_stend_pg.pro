#-------------------------------------------------
#
# Project created by QtCreator 2025-12-05T14:59:20
#
#-------------------------------------------------

QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

win32:RC_ICONS += r853_10_stend_pg.ico
TARGET = r853_10_stend_pg
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    serial_port.cpp

HEADERS  += mainwindow.h \
    serial_port.h

FORMS    += mainwindow.ui

RESOURCES += \
    rec.qrc

