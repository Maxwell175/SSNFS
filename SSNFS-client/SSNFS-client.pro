QT += core
QT -= gui

QT += network

CONFIG += c++11

TARGET = SSNFS-client
CONFIG += console
CONFIG -= app_bundle

LIBS += -lfuse

TEMPLATE = app

SOURCES += main.cpp \
#    threadedclient.cpp \
#    clientmanager.cpp \
    fuseclient.cpp

QMAKE_CXXFLAGS += -D_FILE_OFFSET_BITS=64
QMAKE_CXXFLAGS += -D_XOPEN_SOURCE=700
QMAKE_CXXFLAGS += -DFUSE_USE_VERSION=31

QMAKE_CXXFLAGS += -D_CLIENT_VERSION=0.1

HEADERS += \
#    threadedclient.h \
#    clientmanager.h \
    fuseclient.h
