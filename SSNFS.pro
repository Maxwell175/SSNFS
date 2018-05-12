# SSNFS v0.1
#
# Available under the license(s) specified at https://github.com/MDTech-us-MAN/SSNFS.
#
# Copyright 2018 Maxwell Dreytser
#

TEMPLATE = subdirs

isEmpty(PREFIX) {
    CONFIG(debug, debug|release) {
        PREFIX = $$OUT_PWD
    } else {
        PREFIX = /usr/local
    }
}

system("echo DEFINES += ISGLOBAL=1 > $$OUT_PWD/.qmake.cache")
system("echo PREFIX = $$PREFIX >> $$OUT_PWD/.qmake.cache")
QMAKE_DISTCLEAN += $$OUT_PWD/.qmake.cache

server {
    message(Building server component.)

    SUBDIRS += SSNFSd/PH7 \
               SSNFSd/fastpbkdf2 \
               SSNFSd
    SSNFSd.depends = SSNFSd/PH7 SSNFSd/fastpbkdf2
}

client {
    message(Building client component.)

    SUBDIRS += SSNFS-client
}

!server:!client {
    error("Please select the component\(s\) you would like to configure. To do so, use either `qmake \"CONFIG+=server\" SSNFS.pro' or `qmake \"CONFIG+=client\" SSNFS.pro'.")
}
