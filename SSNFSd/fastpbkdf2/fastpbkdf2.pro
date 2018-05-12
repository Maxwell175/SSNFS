# SSNFS v0.1
#
# Available under the license(s) specified at https://github.com/MDTech-us-MAN/SSNFS.
#
# Copyright 2018 Maxwell Dreytser
#
#
# NOTICE: This .pro file builds the fastpbkdf2 library, which has its own license.
#     This license is specified in the LICENSE file located in this directory.
#

QT       -= core gui

TARGET = fastpbkdf2
TEMPLATE = lib
CONFIG += staticlib

LIBS += -lcrypto -lssl

# Disable all warnings from this library.
#QMAKE_CFLAGS_WARN_ON -= -Wall
#QMAKE_CXXFLAGS_WARN_ON -= -Wall
#QMAKE_CFLAGS += -w
#QMAKE_CXXFLAGS += -w

SOURCES += fastpbkdf2.c

HEADERS += fastpbkdf2.h

