#ifndef SSNFSTHREAD_H
#define SSNFSTHREAD_H

#include <QThread>
#include <QTcpSocket>
#include <QSslSocket>

class SSNFSThread : public QObject
{
Q_OBJECT

public:
    SSNFSThread(int socketDescriptor, QObject *parent);

signals:
    //void error(QTcpSocket::SocketError socketError);

private:
    QThread myThread;

    QString testBase;

    int socketDescriptor;

    QSslSocket *socket;

private slots:
    void run();

    void sslErrorOccurred(QList<QSslError> errors);
    void socketError(QAbstractSocket::SocketError error);

};

#endif // SSNFSTHREAD_H
