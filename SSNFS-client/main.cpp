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
#include <registeriface.h>

int main(int argc, char *argv[])
{
    qputenv("QT_LOGGING_RULES", "qt.network.ssl.warning=false");

    QCoreApplication app(argc, argv);

    if (app.arguments().length() >= 2 && ((QString)app.arguments().at(1)).toLower() == "register") {
        if (app.arguments().length() == 2) {
            qInfo() << "Please specify the server you would like to register to.";
            exit(1);
        }
        if (((QString)app.arguments().at(2)).count(QLatin1Char(':') > 1)) {
            qInfo() << "Invalid server specified.";
            exit(1);
        }

        RegisterIface regIface;

        return 0;
    } else {
        qRegisterMetaType<fuse_conn_info*>("fuse_conn_info*");
        qRegisterMetaType<fs_stat*>("fs_stat*");

        FuseClient client;

        return app.exec();
    }
}
