/*
 * SSNFS Client v0.1
 *
 * Available under the license(s) specified at https://github.com/MDTech-us-MAN/SSNFS.
 *
 * Copyright 2017 Maxwell Dreytser
 */

#include "ssnfsserver.h"
#include "log.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <stdio.h>

#include <QEventLoop>
#include <QDir>
#include <QStorageInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlResult>

#include <limits>
#include <errno.h>

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)
#define HELLO_STR "SSNFS server version " STR(_SERVER_VERSION)

// For now the values are hardcoded. TODO: Config.

SSNFSServer::SSNFSServer(QObject *parent)
    : QTcpServer(parent), testBase("/home/maxwell/fuse-test-base"), // This is the directory that we will serve up.
      privateKeyPath("/home/maxwell/CLionProjects/SSNFS/SSNFSd/test.key"),
      certPath("/home/maxwell/CLionProjects/SSNFS/SSNFSd/test.crt")
{

}

void SSNFSServer::incomingConnection(qintptr socketDescriptor)
{
    QSslSocket *socket = new QSslSocket();

    socket->setPeerVerifyMode(QSslSocket::VerifyNone);
    socket->setLocalCertificate(certPath);
    socket->setPrivateKey(privateKeyPath);
    //socket->setProtocol(QSsl::TlsV1SslV3);

    if (!socket->setSocketDescriptor(socketDescriptor)) {
        // TODO: Log socket.error()
        qDebug() << socket->errorString();
        qDebug() << socket->sslErrors();
        return;
    }

    socket->startServerEncryption();

    socket->setReadBufferSize(1024000);

    SSNFSClient *currClient = new SSNFSClient();
    currClient->socket = socket;
    currClient->status = WaitingForHello;
    clients.append(currClient);

    connect(socket, static_cast<void(QSslSocket::*)(const QList<QSslError> &)>(&QSslSocket::sslErrors),
            [this, currClient](QList<QSslError> errors) {
        this->sslErrorsOccurred(currClient, errors);
    });
    connect(socket, static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),
            [this, currClient](QAbstractSocket::SocketError socketError) {
        this->socketError(currClient, socketError);
    });
    connect(socket, &QSslSocket::disconnected, [this, currClient]() {
        this->socketDisconnected(currClient);
    });
    connect(socket, &QSslSocket::readyRead, [this, currClient]() {
        this->ReadyToRead(currClient);
    });
}

void SSNFSServer::sslErrorsOccurred(SSNFSClient *sender, const QList<QSslError> &errors)
{
    (void) sender;
    // TODO: Log client data.
    qDebug() << errors;
}

void SSNFSServer::socketError(SSNFSClient *sender, QAbstractSocket::SocketError socketError)
{
    (void) socketError;

    qDebug() << sender->socket->errorString();

    if (sender->socket->isOpen()) {
        sender->socket->close();
    } else {
        sender->socket->deleteLater();

        clients.removeAll(sender);

        delete sender;
    }
}

void SSNFSServer::socketDisconnected(SSNFSClient *sender)
{
    qDebug() << "Socket has disconnected.";

    sender->socket->deleteLater();

    clients.removeAll(sender);

    delete sender;
}

void SSNFSServer::ReadyToRead(SSNFSClient *sender)
{
    QSslSocket *socket = sender->socket;

    switch (sender->status) {
    case WaitingForHello:
    {
        Common::ResultCode clientResult = Common::getResultFromBytes(Common::readExactBytes(socket, 1));
        if (clientResult == Common::HTTP_GET) {
            processHttpRequest(sender);
            return;
        } else if (clientResult != Common::Hello) {
            if (socket->isOpen()) {
                socket->close();
            }
            return;
        }
        int32_t serverHelloLen = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));
        QByteArray serverHello = Common::readExactBytes(socket, serverHelloLen);

        if (serverHello.isNull()) {
            if (socket->isOpen()) {
                socket->close();
            }
            return;
        }

        QString helloStr(serverHello);
        // TODO: Log this?

        socket->write(Common::getBytes(Common::Hello));
        socket->write(Common::getBytes((int32_t)strlen(HELLO_STR)));
        socket->write(HELLO_STR);

        sender->status = WaitingForOperation;
    }
        break;
    case WaitingForOperation:
    {
        Common::Operation currOperation = Common::getOperationFromBytes(Common::readExactBytes(socket, 2));

        if (currOperation == Common::InvalidOperation) {
            qDebug() << "Error! Invalid operation from client.";
            socket->close();
            return;
        }

        socket->write(Common::getBytes(Common::OK));

        sender->status = InOperation;
        sender->operation = currOperation;
        sender->operationStep = 1;
    }
        break;
    case InOperation:
    {
        if (sender->operation == Common::getattr) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                // Paths need to be int16 because in Win 10 long paths can be enabled.
                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                int res;

                struct stat stbuf;

                QString finalPath(testBase);
                finalPath.append(targetPath);

                res = lstat(finalPath.toUtf8().data(), &stbuf);

                if (res == -1)
                    res = -errno;

                socket->write(Common::getBytes(res));

                QByteArray statData;
                statData.fill('\x00', sizeof(struct stat));
                memcpy(statData.data(), &stbuf, sizeof(struct stat));

                //socket->write((char*)(&statData), sizeof(stbuf));
                socket->write(statData);

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::readdir) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                int res;

                QString finalPath(testBase);
                finalPath.append(targetPath);

                DIR *dp;
                struct dirent *de;

                dp = opendir(finalPath.toUtf8().data());
                if (dp == NULL) {
                    res = -errno;

                    socket->write(Common::getBytes(res));

                    sender->status = WaitingForOperation;
                    return;
                }
                res = 0;

                QVector<struct dirent> dirents;

                while ((de = readdir(dp)) != NULL) {
                    dirents.append(*de);
                }

                struct dirent outDirents[dirents.length()];

                for (int i = 0; i < dirents.length(); i++) {
                    outDirents[i] = dirents[i];
                }

                closedir(dp);

                socket->write(Common::getBytes(res));

                socket->write(Common::getBytes(dirents.length()));

                socket->write((char*)(&outDirents), sizeof(outDirents));

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::open) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                int res;

                QString finalPath(testBase);
                finalPath.append(targetPath);

                int oflags = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

                res = open(finalPath.toUtf8().data(), oflags);
                if (res == -1)
                    res = -errno;
                else {
                    int realFd = res;
                    int FakeFd = -1;
                    for (int i = 1; i < std::numeric_limits<int>::max(); i++) {
                        if (sender->fds.keys().contains(i) == false) {
                            FakeFd = i;
                            break;
                        }
                    }
                    if (FakeFd == -1)
                        res = -EBADFD;
                    else {
                        sender->fds.insert(FakeFd, realFd);
                        res = FakeFd;
                    }
                }

                socket->write(Common::getBytes(res));

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::read) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                sender->timer.restart();

                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                QString finalPath(testBase);
                finalPath.append(targetPath);

                // Get the internal File Descriptor.
                int fakeFd = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

                int64_t readSize = Common::getInt64FromBytes(Common::readExactBytes(socket, 8));

                int64_t readOffset = Common::getInt64FromBytes(Common::readExactBytes(socket, 8));

                int res;

                void *readBuf = malloc(readSize);
                int actuallyRead = -1;

                // Find the corresponding actual File Descriptor
                int realFd = sender->fds.value(fakeFd);

                // We will now use the proc virtual filesystem to look up info about the File Descriptor.
                QString procFdPath = tr("/proc/self/fd/%1").arg(realFd);

                struct stat procFdInfo;

                lstat(procFdPath.toUtf8().data(), &procFdInfo);

                QVector<char> actualFdFilePath;
                actualFdFilePath.fill('\x00', procFdInfo.st_size + 1);

                ssize_t r = readlink(procFdPath.toUtf8().data(), actualFdFilePath.data(), procFdInfo.st_size + 1);

                if (r < 0) {
                    res = -errno;
                } else {
                    actualFdFilePath[procFdInfo.st_size] = '\0';
                    if (r > procFdInfo.st_size) {
                        qWarning() << "The File Descriptor link in /proc increased in size" <<
                                      "between lstat() and readlink()! old size:" << procFdInfo.st_size
                                   << " new size:" << r << " readlink result:" << actualFdFilePath;
                        res = -1;
                    } else {
                        if (finalPath == actualFdFilePath.data()) {
                            actuallyRead = pread(realFd, readBuf, readSize, readOffset);
                            res = actuallyRead;
                        } else {
                            res = -EBADF;
                        }
                    }
                }

                socket->write(Common::getBytes(res));

                if (res > 0)
                    socket->write((char*)(readBuf), actuallyRead);

                sender->operationData.clear();
                sender->operationStep = 1;

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::access) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                int res;

                QString finalPath(testBase);
                finalPath.append(targetPath);

                int mask = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

                // TODO: Add additional restrictions to prevent problems across users.
                res = access(finalPath.toUtf8().data(), mask);

                socket->write(Common::getBytes(res));

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::readlink) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                int res;

                QString finalPath(testBase);
                finalPath.append(targetPath);

                int size = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

                QByteArray buf;
                buf.fill('\0', size);

                // TODO: Add additional restrictions to prevent problems across users.
                res = readlink(finalPath.toUtf8().data(), buf.data(), size - 1);

                if (res == -1)
                    socket->write(Common::getBytes(-errno));
                else {
                    // Convert paths to be relative to base.
                    QDir base(testBase);
                    QString result = base.relativeFilePath(buf.data());

                    socket->write(Common::getBytes(result.length()));

                    socket->write(result.toUtf8().data());
                }


                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::mknod) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                int res;

                QString finalPath(testBase);
                finalPath.append(targetPath);

                unsigned int mode = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

                /* On Linux this could just be 'mknod(path, mode, rdev)' but this
                   is more portable */
                if (S_ISFIFO(mode))
                    res = mkfifo(finalPath.toUtf8().data(), mode);
                else
                    res = mknod(finalPath.toUtf8().data(), mode, (dev_t) 0);
                if (res == -1)
                    res = -errno;

                socket->write(Common::getBytes(res));

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::mkdir) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                int res;

                QString finalPath(testBase);
                finalPath.append(targetPath);

                unsigned int mode = Common::getUInt32FromBytes(Common::readExactBytes(socket, 4));

                res = mkdir(finalPath.toUtf8().data(), mode);

                if (res == -1)
                    res = -errno;

                socket->write(Common::getBytes(res));

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::unlink) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                int res;

                QString finalPath(testBase);
                finalPath.append(targetPath);

                res = unlink(finalPath.toUtf8().data());

                if (res == -1)
                    res = -errno;

                socket->write(Common::getBytes(res));

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::rmdir) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                int res;

                QString finalPath(testBase);
                finalPath.append(targetPath);

                res = rmdir(finalPath.toUtf8().data());

                if (res == -1)
                    res = -errno;

                socket->write(Common::getBytes(res));

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::symlink) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                uint16_t oldPathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray oldPath = Common::readExactBytes(socket, oldPathLength);

                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                int res;

                QString finalPath(testBase);
                finalPath.append(targetPath);

                res = symlink(oldPath.data(), finalPath.toUtf8().data());

                if (res == -1)
                    res = -errno;

                socket->write(Common::getBytes(res));

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::rename) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                uint16_t FromPathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray FromPath = Common::readExactBytes(socket, FromPathLength);

                uint16_t ToPathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray ToPath = Common::readExactBytes(socket, ToPathLength);

                int res;

                QString finalFromPath(testBase);
                finalFromPath.append(FromPath);
                QString finalToPath(testBase);
                finalToPath.append(ToPath);

                res = rename(finalFromPath.toUtf8().data(), finalToPath.toUtf8().data());

                if (res == -1)
                    res = -errno;

                socket->write(Common::getBytes(res));

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::chmod) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                int res;

                QString finalPath(testBase);
                finalPath.append(targetPath);

                unsigned int mode = Common::getUInt32FromBytes(Common::readExactBytes(socket, 4));

                // TODO: This is just a temporary version. Really, this should be much more complex.
                res = chmod(finalPath.toUtf8().data(), mode);

                if (res == -1)
                    res = -errno;

                socket->write(Common::getBytes(res));

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::chown) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                int res;

                QString finalPath(testBase);
                finalPath.append(targetPath);

                unsigned int uid = Common::getUInt32FromBytes(Common::readExactBytes(socket, 4));
                unsigned int gid = Common::getUInt32FromBytes(Common::readExactBytes(socket, 4));

                // TODO: This is just a temporary version. Really, this should be much more complex.
                res = lchown(finalPath.toUtf8().data(), uid, gid);

                if (res == -1)
                    res = -errno;

                socket->write(Common::getBytes(res));

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::truncate) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                int res;

                QString finalPath(testBase);
                finalPath.append(targetPath);

                uint32_t size = Common::getUInt32FromBytes(Common::readExactBytes(socket, 4));

                res = truncate(finalPath.toUtf8().data(), size);

                if (res == -1)
                    res = -errno;

                socket->write(Common::getBytes(res));

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::utimens) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                int res;

                QString finalPath(testBase);
                finalPath.append(targetPath);

                timespec ts[2];
                ts[0].tv_sec = Common::getInt64FromBytes(Common::readExactBytes(socket, 8));
                ts[0].tv_nsec = Common::getInt64FromBytes(Common::readExactBytes(socket, 8));
                ts[1].tv_sec = Common::getInt64FromBytes(Common::readExactBytes(socket, 8));
                ts[1].tv_nsec = Common::getInt64FromBytes(Common::readExactBytes(socket, 8));

                res = utimensat(0, finalPath.toUtf8().data(), ts, AT_SYMLINK_NOFOLLOW);

                if (res == -1)
                    res = -errno;

                socket->write(Common::getBytes(res));

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::write) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                //sender->timer.restart();

                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                QString finalPath(testBase);
                finalPath.append(targetPath);

                // Get the internal File Descriptor.
                int fakeFd = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

                int64_t Size = Common::getInt64FromBytes(Common::readExactBytes(socket, 8));

                int64_t Offset = Common::getInt64FromBytes(Common::readExactBytes(socket, 8));

                qDebug() << "Got Metadata:" << sender->timer.elapsed();

                QByteArray dataToWrite;

                /*while (true) {
                    int currSize = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));
                    if (currSize == -1)
                        break;
                    dataToWrite.append(Common::readExactBytes(socket, currSize, -1, sender->timer));
                    socket->write(Common::getBytes(Common::OK));
                    socket->waitForBytesWritten(-1);
                }*/

                dataToWrite.append(Common::readExactBytes(socket, Size));//, sender->timer));

                qDebug() << "Got Data:" << sender->timer.elapsed();

                int res;

                int actuallyWritten = -1;

                // Find the corresponding actual File Descriptor
                int realFd = sender->fds.value(fakeFd);

                // We will now use the proc virtual filesystem to look up info about the File Descriptor.
                QString procFdPath = tr("/proc/self/fd/%1").arg(realFd);

                struct stat procFdInfo;

                lstat(procFdPath.toUtf8().data(), &procFdInfo);

                QVector<char> actualFdFilePath;
                actualFdFilePath.fill('\x00', procFdInfo.st_size + 1);

                ssize_t r = readlink(procFdPath.toUtf8().data(), actualFdFilePath.data(), procFdInfo.st_size + 1);

                if (r < 0) {
                    res = -errno;
                } else {
                    actualFdFilePath[procFdInfo.st_size] = '\0';
                    if (r > procFdInfo.st_size) {
                        qWarning() << "The File Descriptor link in /proc increased in size" <<
                                      "between lstat() and readlink()! old size:" << procFdInfo.st_size
                                   << " new size:" << r << " readlink result:" << actualFdFilePath;
                        res = -1;
                    } else {
                        if (finalPath == actualFdFilePath.data()) {
                            actuallyWritten = pwrite(realFd, dataToWrite.data(), Size, Offset);
                            if (actuallyWritten == -1)
                                res = -errno;
                            else
                                res = actuallyWritten;
                        } else {
                            res = -EBADF;
                        }
                    }
                }

                socket->write(Common::getBytes(res));
                qDebug() << res << "bytes, Done Writing." << sender->timer.elapsed();

                sender->operationData.clear();
                sender->operationStep = 1;

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::writebulk) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                //sender->timer.restart();

                uint32_t numOfRequests = Common::getUInt32FromBytes(Common::readExactBytes(socket, 4));

                int32_t result = 0;

                for (uint32_t i = 0; i < numOfRequests; i++) {

                    uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                    QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                    QString finalPath(testBase);
                    finalPath.append(targetPath);

                    // Get the internal File Descriptor.
                    int fakeFd = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

                    int64_t Size = Common::getInt64FromBytes(Common::readExactBytes(socket, 8));

                    int64_t Offset = Common::getInt64FromBytes(Common::readExactBytes(socket, 8));

                    qDebug() << "Got Metadata:" << sender->timer.elapsed();

                    QByteArray dataToWrite;

                    /*while (true) {
                    int currSize = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));
                    if (currSize == -1)
                        break;
                    dataToWrite.append(Common::readExactBytes(socket, currSize, -1, sender->timer));
                    socket->write(Common::getBytes(Common::OK));
                    socket->waitForBytesWritten(-1);
                }*/

                    dataToWrite.append(Common::readExactBytes(socket, Size));//, sender->timer));

                    qDebug() << "Got Data:" << sender->timer.elapsed();

                    int res;

                    int actuallyWritten = -1;

                    // Find the corresponding actual File Descriptor
                    int realFd = sender->fds.value(fakeFd);

                    // We will now use the proc virtual filesystem to look up info about the File Descriptor.
                    QString procFdPath = tr("/proc/self/fd/%1").arg(realFd);

                    struct stat procFdInfo;

                    lstat(procFdPath.toUtf8().data(), &procFdInfo);

                    QVector<char> actualFdFilePath;
                    actualFdFilePath.fill('\x00', procFdInfo.st_size + 1);

                    ssize_t r = readlink(procFdPath.toUtf8().data(), actualFdFilePath.data(), procFdInfo.st_size + 1);

                    if (r < 0) {
                        res = -errno;
                    } else {
                        actualFdFilePath[procFdInfo.st_size] = '\0';
                        if (r > procFdInfo.st_size) {
                            qWarning() << "The File Descriptor link in /proc increased in size" <<
                                          "between lstat() and readlink()! old size:" << procFdInfo.st_size
                                       << " new size:" << r << " readlink result:" << actualFdFilePath;
                            res = -1;
                        } else {
                            if (finalPath == actualFdFilePath.data()) {
                                actuallyWritten = pwrite(realFd, dataToWrite.data(), Size, Offset);
                                if (actuallyWritten == -1)
                                    res = -errno;
                                else
                                    res = actuallyWritten;
                            } else {
                                res = -EBADF;
                            }
                        }
                    }
                    qDebug() << res << "bytes, Done Writing." << sender->timer.elapsed();

                    if (res < 0)
                        result = res;
                }

                socket->write(Common::getBytes(result));

                sender->operationData.clear();
                sender->operationStep = 1;

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::release) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                QString finalPath(testBase);
                finalPath.append(targetPath);

                int res;

                int32_t fakeFd = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

                int realFd = sender->fds.value(fakeFd);

                // We will now use the proc virtual filesystem to look up info about the File Descriptor.
                QString procFdPath = tr("/proc/self/fd/%1").arg(realFd);

                struct stat procFdInfo;

                lstat(procFdPath.toUtf8().data(), &procFdInfo);

                QVector<char> actualFdFilePath;
                actualFdFilePath.fill('\x00', procFdInfo.st_size + 1);

                ssize_t r = readlink(procFdPath.toUtf8().data(), actualFdFilePath.data(), procFdInfo.st_size + 1);

                if (r < 0) {
                    res = -errno;
                } else {
                    actualFdFilePath[procFdInfo.st_size] = '\0';
                    if (r > procFdInfo.st_size) {
                        qWarning() << "The File Descriptor link in /proc increased in size" <<
                                      "between lstat() and readlink()! old size:" << procFdInfo.st_size
                                   << " new size:" << r << " readlink result:" << actualFdFilePath;
                        res = -1;
                    } else {
                        if (finalPath == actualFdFilePath.data()) {
                            res = ::close(realFd);
                            if (res == -1)
                                res = -errno;
                        } else {
                            res = -EBADF;
                        }
                    }
                }

                socket->write(Common::getBytes(res));

                sender->fds.remove(fakeFd);

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        } else if (sender->operation == Common::statfs) {
            switch (sender->operationStep) {
            case 1:
            {
                if (sender->working)
                    return;
                sender->working = true;

                uint16_t pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength);

                QString finalPath(testBase);
                finalPath.append(targetPath);

                int res = 0;

                struct statvfs fsInfo;

                // No need to expose any info about what the system drive looks like.
                // Just make stuff up.
                fsInfo.f_namemax = 255;
                fsInfo.f_bsize = fsInfo.f_frsize = 4096;
                fsInfo.f_files = fsInfo.f_ffree = fsInfo.f_favail = 1000000000;

                QStorageInfo storage(finalPath);
                if (storage.isValid() && storage.isReady()) {
                    fsInfo.f_blocks = storage.bytesTotal() / fsInfo.f_frsize;
                    fsInfo.f_bfree = fsInfo.f_bavail = storage.bytesAvailable() /fsInfo.f_bsize;
                    res = 0;
                } else {
                    res = -EFAULT;
                }

                QByteArray fsInfoData;
                fsInfoData.fill('\x00', sizeof(struct statvfs));
                memcpy(fsInfoData.data(), &fsInfo, sizeof(struct statvfs));

                socket->write(fsInfoData);

                socket->write(Common::getBytes(res));

                sender->status = WaitingForOperation;

                sender->working = false;
            }
                break;
            }
        }
    }
        break;
    }
}

void SSNFSServer::processHttpRequest(SSNFSClient *sender)
{
    while (sender->socket->canReadLine() == false) {
        sender->socket->waitForReadyRead(-1);
    }
    QString RequestLine = sender->socket->readLine();
    QString RequestPath = RequestLine.split(" ")[1];
    QMap<QString, QString> Headers;
    while (true) {
        while (sender->socket->canReadLine() == false) {
            sender->socket->waitForReadyRead(-1);
        }
        QString HeaderLine = sender->socket->readLine();
        HeaderLine = HeaderLine.trimmed();
        if (HeaderLine.isEmpty())
            break;
        int HeaderNameLen = HeaderLine.indexOf(": ");
        Headers.insert(HeaderLine.mid(0, HeaderNameLen), HeaderLine.mid(HeaderNameLen + 2));
    }
    sender->socket->write("HTTP/1.1 200 OK\r\n");
    sender->socket->write("Content-Type: text/html\r\n");
    // Let the browser know we plan to close this conenction.
    sender->socket->write("Connection: close\r\n\r\n");
    sender->socket->write(tr("<html><body><h1>Hello World</h1><h3>You requested: %1</h3><h4>Headers:</h4><pre>").arg(RequestPath).toUtf8());
    QDebug(sender->socket) << Headers;
    sender->socket->write("</pre></body></html>");
    sender->socket->close();
}
