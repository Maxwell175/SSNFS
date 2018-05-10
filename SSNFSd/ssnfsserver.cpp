/*
 * SSNFS Client v0.1
 *
 * Available under the license(s) specified at https://github.com/MDTech-us-MAN/SSNFS.
 *
 * Copyright 2018 Maxwell Dreytser
 */

#include "ssnfsserver.h"
#include <log.h>
#include <serversettings.h>
#include <ssnfsworker.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <QEventLoop>
#include <QDir>
#include <QStorageInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlResult>
#include <QTimer>

#include <limits>
#include <errno.h>

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)
#define HELLO_STR "SSNFS server version " STR(_SERVER_VERSION)

SSNFSServer::SSNFSServer(QObject *parent)
    : QTcpServer(parent) // This is the directory that we will serve up.
{
    if (ServerSettings::get("PrivateKeySource") == "file") {
        QString newFile = ServerSettings::get("PrivateKeyFilePath");
        Log::info(Log::Categories["Core"], "Loading private key from file: {0}", ToChr(newFile));
        QFile pkeyFile(newFile);
        if (pkeyFile.exists() == false) {
            Log::error(Log::Categories["Core"], "Private Key File does not exist.", ToChr(newFile));
            exit(1);
        } else {
            if (pkeyFile.open(QFile::ReadOnly) == false) {
                Log::error(Log::Categories["Core"], "Could not open the Private Key file: {0}", pkeyFile.errorString().toUtf8().data());
                exit(1);
            } else {
                QSslKey pkey(pkeyFile.readAll(), QSsl::Rsa);
                pkeyFile.close();
                if (pkey.isNull()) {
                    Log::error(Log::Categories["Core"], "The Private Key file specified is not a valid PEM-encoded RSA private key.");
                    exit(1);
                } else {
                    privateKey = pkey;
                }
            }
        }
    }

    QString newCertFile = ServerSettings::get("CertificatePath");
    Log::info(Log::Categories["Core"], "Loading certificate from file: {0}", ToChr(newCertFile));
    QFile certFile(newCertFile);
    if (certFile.exists() == false) {
        Log::error(Log::Categories["Core"], "Certificate File does not exist.", ToChr(newCertFile));
        exit(1);
    } else {
        if (certFile.open(QFile::ReadOnly) == false) {
            Log::error(Log::Categories["Core"], "Could not open the Certificate file: {0}", certFile.errorString().toUtf8().data());
            exit(1);
        } else {
            QSslCertificate cert(certFile.readAll());
            certFile.close();
            if (cert.isNull()) {
                Log::error(Log::Categories["Core"], "The Certificate file you specified is not a valid PEM-encoded certificate.");
                exit(1);
            } else {
                certificate = cert;
            }
        }
    }

    configDB = getConfDB();
    if (!configDB.isValid()) {
        Log::error(Log::Categories["Core"], "Cannot get the configuration DB while starting server. Exiting.");
        exit(1);
        return;
    }

    sockThread.start();
}

void SSNFSServer::incomingConnection(qintptr socketDescriptor)
{
    qDebug() << QThread::currentThread();
    SSNFSWorker *thread = new SSNFSWorker(socketDescriptor, this);
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();
}
