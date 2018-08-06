# SSNFSd v0.1
#
# Available under the license(s) specified at https://github.com/MDTech-us-MAN/SSNFS.
#
# Copyright 2018 Maxwell Dreytser
#

!contains(DEFINES, "ISGLOBAL=1"):{
    error(SSNFSd.pro cannot be used independently. Please use the main SSNFS.pro file.)
}

QT += core
QT -= gui

QT += network sql

CONFIG += c++11

TARGET = SSNFSd
CONFIG += console
CONFIG -= app_bundle

target.path = $$PREFIX/bin
INSTALLS += target

TEMPLATE = app

include(../Common/Common.pri)

INCLUDEPATH += PH7
DEPENDPATH += PH7

LIBS += -LPH7 -lPH7

SOURCES += \
    main.cpp \
    ssnfsserver.cpp \
    serversettings.cpp \
    ssnfsworker.cpp \
    ssnfswebworker.cpp \
    initiface.cpp

HEADERS += \
    ssnfsserver.h \
    log.h \
    serversettings.h \
    ssnfsworker.h \
    initiface.h

webpanel.path = $$PREFIX/etc/SSNFS/webpanel
webpanel.files = webpanel/*
INSTALLS += webpanel

DEFINES += _SERVER_VERSION=0.1
DEFINES += "_CONFIG_DIR=\"\\\"$$PREFIX/etc/SSNFS\\\"\""

DEFINES += _FILE_OFFSET_BITS=64
DEFINES += _XOPEN_SOURCE=700

RESOURCES += \
    resources.qrc
