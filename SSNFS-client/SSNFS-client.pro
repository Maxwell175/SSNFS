# SSNFS Client v0.1
#
# Available under the license(s) specified at https://github.com/MDTech-us-MAN/SSNFS.
#
# Copyright 2018 Maxwell Dreytser
#

!contains(DEFINES, "ISGLOBAL=1"):{
    error(SSNFS-client.pro cannot be used independently. Please use the main SSNFS.pro file.)
}

QT += core
QT -= gui

QT += network

CONFIG += c++11

TARGET = SSNFS-client
CONFIG += console
CONFIG -= app_bundle

target.path = $$PREFIX/bin
INSTALLS += target

LIBS += -lfuse

TEMPLATE = app

include(../Common/Common.pri)

SOURCES += main.cpp \
    fuseclient.cpp \
    registeriface.cpp

HEADERS += \
    fuseclient.h \
    registeriface.h

DEFINES += "_CONFIG_DIR=\"\\\"$$PREFIX/etc\\\"\""
DEFINES += _FILE_OFFSET_BITS=64
DEFINES += _XOPEN_SOURCE=700
DEFINES += FUSE_USE_VERSION=31

DEFINES += _CLIENT_VERSION=0.1
