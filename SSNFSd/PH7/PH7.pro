# SSNFS v0.1
#
# Available under the license(s) specified at https://github.com/MDTech-us-MAN/SSNFS.
#
# Copyright 2018 Maxwell Dreytser
#
#
# NOTICE: This .pro file builds the PH7 library, which has its own license.
#     This license is specified in the license.txt file located in this directory.
#

QT       -= core gui

TARGET = PH7
TEMPLATE = lib
CONFIG += staticlib

DEFINES += PH7_ENABLE_THREADS
# Needed to build PH7:
DEFINES += MAP_FILE=0x0001

# Disable all warnings from this library.
# All those 567 warnings from PH7 make it harder
#    to notice legitimate errors from our code.
QMAKE_CFLAGS_WARN_ON -= -Wall
QMAKE_CXXFLAGS_WARN_ON -= -Wall
QMAKE_CFLAGS += -w
QMAKE_CXXFLAGS += -w

SOURCES += ph7.c

HEADERS += ph7.h
