# SSNFSd v0.1
#
# Available under the license(s) specified at https://github.com/MDTech-us-MAN/SSNFS.
#
# Copyright 2017 Maxwell Dreytser
#

QT += core
QT -= gui

QT += network sql

CONFIG += c++11

TARGET = SSNFSd
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

include(../Common/Common.pri)

SOURCES += main.cpp \
    ssnfsserver.cpp \
    serversettings.cpp

HEADERS += \
    ssnfsserver.h \
    log.h \
    serversettings.h

DEFINES += _SERVER_VERSION=0.1
DEFINES += "_CONFIG_DIR=\"\\\"$$PWD\\\"\""

DEFINES += _FILE_OFFSET_BITS=64
DEFINES += _XOPEN_SOURCE=700
