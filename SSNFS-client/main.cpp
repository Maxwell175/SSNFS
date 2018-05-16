/*
 * SSNFS Client v0.1
 *
 * Available under the license(s) specified at https://github.com/MDTech-us-MAN/SSNFS.
 *
 * Copyright 2018 Maxwell Dreytser
 */

#include <QCoreApplication>
#include <stdio.h>
#include <fuseclient.h>

int main(int argc, char *argv[])
{
    qputenv("QT_LOGGING_RULES", "qt.network.ssl.warning=false");

    qInfo() << Common::getSystemInfo();

    QCoreApplication app(argc, argv);

    qRegisterMetaType<fuse_conn_info*>("fuse_conn_info*");
    qRegisterMetaType<fs_stat*>("fs_stat*");

    FuseClient client;

    return app.exec();
}
