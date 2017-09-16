#include "threadedclient.h"

#include <unistd.h>

bool ThreadedClient::SendData(const char *data, signed long long length)
{
    if (ClientStatus != ConnectionStatus::Connected) {
        return false;
    }

    // First, get the length of the data.
    unsigned long int dataLength;
    if (length < 0) {
        // + 1 is for the null terminator.
        dataLength = strlen(data);
    } else {
        dataLength = length;
    }

    // Convert it into bytes.
    unsigned char lengthBytes[4];
    lengthBytes[0] = (dataLength >> 24) & 0xFF;
    lengthBytes[1] = (dataLength >> 16) & 0xFF;
    lengthBytes[2] = (dataLength >> 8) & 0xFF;
    lengthBytes[3] = dataLength & 0xFF;

    // Write the length first.
    socket->write((char*)(&lengthBytes), 4);
    socket->waitForBytesWritten();
    // Then the actual data.
    socket->write(data);
    socket->waitForBytesWritten();

    return true;
}

char* ThreadedClient::ReadData(int timeoutMsec)
{
    if (ClientStatus != ConnectionStatus::Connected) {
        return NULL;
    }

    unsigned long int bytesRead = 0;

    char lengthBytes[4] = "";

    // Read exactly 4 bytes to get the length of the data.
    while (bytesRead < 4) {
        // Wait for data to be ready.
        qDebug() << socket->bytesAvailable();

        if (socket->bytesAvailable() == 0) {
            socket->waitForReadyRead(1000);
        }

        //QTime timeout;
        //timeout.start();
        /*while (socket->bytesAvailable() == 0 && socket->waitForReadyRead(1) == false) {
            // If there is no data for <timeoutMsec>
            //return NULL;
            if (timeout.elapsed() > timeoutMsec)
                return NULL;
        }*/

        // Read what data has come in.
        char currData[4 - bytesRead];
        int currRead = socket->read((char *)(&currData), 4 - bytesRead);
        for (int i = 0; i < currRead; i++) {
            lengthBytes[bytesRead + i] = currData[i];
        }
        bytesRead += currRead;
    }

    // Get a number back from the bytes.
    unsigned long int length = 0;
    length  = ((unsigned long int) lengthBytes[0]) << 24;
    length |= ((unsigned long int) lengthBytes[1]) << 16;
    length |= ((unsigned long int) lengthBytes[2]) << 8;
    length |= ((unsigned long int) lengthBytes[3]);

    // Clear the temp variable.
    bytesRead = 0;

    char data[length] = "";

    // Read the actual data.
    while (bytesRead < length) {
        // Wait for data to be ready.
        if (socket->bytesAvailable() == 0 && socket->waitForReadyRead(timeoutMsec) == false) {
            // If there is no data for <timeoutMsec>
            //return NULL;
        }

        // Read what data has come in.
        char currData[length - bytesRead];
        int currRead = socket->read((char *)(&currData), length - bytesRead);
        for (int i = 0; i < currRead; i++) {
            data[bytesRead + i] = currData[i];
        }
        bytesRead += currRead;
    }

    // Allocate an array on the heap so we can safely work with it outside this function.
    char *output = new char[length];

    // Copy to output array.
    for (unsigned long int i = 0; i < length; i++) {
        output[i] = data[i];
    }

    return output;
}

ThreadedClient::ThreadedClient(int ClientId, QObject *parent) : QObject(parent)
{
    this->ClientId = ClientId;
    moveToThread(workThread);
    connect(workThread, SIGNAL(started()), this, SLOT(startSocket()), Qt::QueuedConnection);
    workThread->start();
}

void ThreadedClient::sslErrorOccurred(QList<QSslError> errors) {
    foreach (QSslError err, errors) {
        qDebug() << err.errorString();
    }
}

void ThreadedClient::startSocket()
{
    socket = new QSslSocket(this);
    socket->setCaCertificates(QSslSocket::systemCaCertificates());
    socket->setPeerVerifyMode(QSslSocket::VerifyNone);
    socket->addCaCertificates("/home/maxwell/CLionProjects/SSNFS/SSNFSd/ca.crt");

    connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrorOccurred(QList<QSslError>)));

    connect(socket, &QSslSocket::encrypted, [this]() {
        ClientStatus = ConnectionStatus::Connected;

        SendData(tr("HELLO: SSNFS client version %1").arg(_CLIENT_VERSION).toUtf8().data());
        char *serverHello = ReadData();

        if (serverHello == NULL) {
            resetSocket();
            return;
        }

        QString helloStr(serverHello);

        if (!helloStr.startsWith("HELLO: ")) {
            SendData("ERROR: Invalid response to HELLO!");
            resetSocket();
            return;
        }
        // TODO: Log the HELLO msg.

        emit statusChanged(this, ClientId, ConnectionStatus::Connected);
    });

    connect(socket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));

    socket->connectToHostEncrypted("localhost", 2050);
}

void ThreadedClient::resetSocket()
{
    ClientStatus = ConnectionStatus::NotConnected;
    emit statusChanged(this, ClientId, ConnectionStatus::NotConnected);

    if (socket->isOpen())
        socket->close();

    socket->deleteLater();

    sleep(10);

    startSocket();
}

void ThreadedClient::socketDisconnected()
{
    QTimer::singleShot(0, this, [this]() {
        resetSocket();
    });
}

void ThreadedClient::socketError(QAbstractSocket::SocketError error)
{
    qDebug() << socket->errorString();

    // TODO: Log error.
    QTimer::singleShot(0, this, [this]() {
        resetSocket();
    });
}

void ThreadedClient::LockClient()
{
    ClientStatus = ConnectionStatus::InUse;
    emit statusChanged(this, ClientId, ConnectionStatus::InUse);
}

void ThreadedClient::UnlockClient()
{
    ClientStatus = ConnectionStatus::Connected;
    emit statusChanged(this, ClientId, ConnectionStatus::Connected);
}
