/*
 * SSNFS Client v0.1
 *
 * Available under the license(s) specified at https://github.com/MDTech-us-MAN/SSNFS.
 *
 * Copyright 2017 Maxwell Dreytser
 */

#include <QCoreApplication>

#include <spdlog/spdlog.h>

#include "ssnfsserver.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    SSNFSServer server;

    server.listen(QHostAddress::Any, 2050);

    app.exec();
}
