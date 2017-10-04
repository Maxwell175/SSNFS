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
#include <QSslSocket>
#include <QList>

enum ClientStatus {
    WaitingForHello,
    WaitingForOperation,
    InOperation
};

struct SSNFSClient {
    QSslSocket *socket;
    ClientStatus status;
    QString operation;
    int operationStep = 0;
    bool working = false;
    QMap<int, int> fds;
    QMap<QString, QVariant> operationData;
    QTime timer;

    bool operator == (const SSNFSClient &rhs) const {
        return (socket == rhs.socket) && (status == rhs.status);
    }
    bool operator != (const SSNFSClient &rhs) const {
        return (socket != rhs.socket) || (status != rhs.status);
    }
};

class SSNFSServer : public QTcpServer
{
public:
    SSNFSServer(QObject *parent = 0);

private:
    QString testBase;
    QString privateKeyPath;
    QString certPath;
    QString caCertPath;

    QList<SSNFSClient*> clients;

private slots:
    void sslErrorsOccurred(SSNFSClient *sender, const QList<QSslError> &errors);
    void socketError(SSNFSClient *sender, QAbstractSocket::SocketError socketError);
    void socketDisconnected(SSNFSClient *sender);

    void ReadyToRead(SSNFSClient *sender);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

};

#endif // SSNFSSERVER_H
