/*
 * SSNFS Client v0.1
 *
 * Available under the license(s) specified at https://github.com/MDTech-us-MAN/SSNFS.
 *
 * Copyright 2017 Maxwell Dreytser
 */

#ifndef SSNFSSERVER_H
#define SSNFSSERVER_H

#include <QTcpServer>
#include <QList>
#include <QSqlDatabase>
#include <QSslKey>
#include <QQueue>
#include <QThread>
#include <common.h>
#include <ssnfsworker.h>
#include <spdlog/spdlog.h>
#include <atomic>

class SSNFSServer : public QTcpServer
{
    Q_OBJECT
public:
    SSNFSServer(QObject *parent = 0);
    QSqlDatabase configDB;

    QThread sockThread;

    QString testBase;

    QSslKey privateKey;
    QSslCertificate certificate;

    //QList<SSNFSClient*> clients;

protected:
    void incomingConnection(qintptr socketDescriptor) override;

};

#endif // SSNFSSERVER_H
