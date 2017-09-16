QT += core
QT -= gui

QT += network

CONFIG += c++11

TARGET = SSNFSd
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    ssnfsserver.cpp \
#    ssnfsthread.cpp \
#    ssnfsrunnable.cpp

HEADERS += \
    ssnfsserver.h
#    ssnfsthread.h \


QMAKE_CXXFLAGS += -D_FILE_OFFSET_BITS=64
QMAKE_CXXFLAGS += -D_XOPEN_SOURCE=700

QMAKE_CXXFLAGS += -D_SERVER_VERSION=0.1