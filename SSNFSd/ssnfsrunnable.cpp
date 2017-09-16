#include "ssnfsrunnable.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <stdio.h>

SSNFSRunnable::SSNFSRunnable(int socketDescriptor)
    : QRunnable() , socketDescriptor(socketDescriptor)
{ }

bool SendData(QSslSocket *socket, const char *data, signed long long length = -1)
{
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
    // Then the actual data.
    socket->write(data);
    socket->waitForBytesWritten();

    return true;
}

char* ReadData(QSslSocket *socket, int timeoutMsec = -1)
{
    unsigned long int bytesRead = 0;

    char lengthBytes[4] = "";

    // Read exactly 4 bytes to get the length of the data.
    while (bytesRead < 4) {
        // Wait for data to be ready.
        if (socket->bytesAvailable() == 0 && socket->waitForReadyRead(timeoutMsec) == false) {
            // If there is no data for <timeoutMsec>
            return NULL;
        }

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


void SSNFSRunnable::run()
{
    socket = new QSslSocket();

    socket->setPeerVerifyMode(QSslSocket::VerifyNone);
    socket->addCaCertificates("/home/maxwell/CLionProjects/SSNFS/SSNFSd/ca.crt");
    socket->setLocalCertificate("/home/maxwell/CLionProjects/SSNFS/SSNFSd/test.crt");
    socket->setPrivateKey("/home/maxwell/CLionProjects/SSNFS/SSNFSd/test.key");
    //socket->setProtocol(QSsl::TlsV1SslV3);

    if (!socket->setSocketDescriptor(socketDescriptor)) {
        // TODO: Log socket.error()
        qDebug() << socket->errorString();
        qDebug() << socket->sslErrors();
        return;
    }

    socket->startServerEncryption();

    if (!socket->waitForReadyRead(-1)) {
        // TODO: Log socket.error()
        qDebug() << socket->errorString();
        qDebug() << socket->sslErrors();
        return;
    }

    char *hello = ReadData(socket);
    if (hello == NULL) {
        // TODO: Log read error
        if (socket->isOpen())
            socket->close();
        return;
    }

    if (QString(hello).startsWith("HELLO: ") == false) {
        // TODO: Log invalid hello.
        if (socket->isOpen())
            socket->close();
        return;
    }

    if (SendData(socket, QString("HELLO: SSNFS server version %1").arg(_SERVER_VERSION).toUtf8().data()) == false) {
        // TODO: Log write error
        if (socket->isOpen())
            socket->close();
        return;
    }

    while (socket->isOpen()) {
        if (!socket->waitForReadyRead(-1)) {
            continue;
        }

        char *dataRecvd = ReadData(socket);
        if (dataRecvd == NULL) {
            // TODO: Log read error
            if (socket->isOpen())
                socket->close();
            return;
        }

        QString recvd = QString(dataRecvd);
        QStringList operParts = recvd.split(": ");

        if (operParts[0] != "OPERATION") {
            // TODO: Log no operation.
            if (socket->isOpen())
                socket->close();
            return;
        }

        if (operParts[1] == "getaddr") {
            SendData(socket, "OPERATION OK");

            char *targetPath = ReadData(socket);

            int res;

            struct stat stbuf;

            QString finalPath(testBase);
            finalPath.append(targetPath);

            res = lstat(finalPath.toUtf8().data(), &stbuf);

            if (res == -1)
                res = -errno;

            char resultVal[18] = "";
            snprintf(resultVal, 18, "RETURN VALUE: %d", res);
            SendData(socket, (char*)(&resultVal));

            SendData(socket, "RETURN PARAM: stbuf");

            char statData[sizeof(stbuf)];
            memcpy(&statData, &stbuf, sizeof(stbuf));

            SendData(socket, (char*)(&statData));
        }
    }

    // TODO: Log socket.error()
    qDebug() << socket->errorString();
    qDebug() << socket->sslErrors();
    return;

    socket->close();
    socket->waitForDisconnected(0);
    socket->deleteLater();
}
