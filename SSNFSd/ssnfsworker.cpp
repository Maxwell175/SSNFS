/*
 * SSNFS Client v0.1
 *
 * Available under the license(s) specified at https://github.com/MDTech-us-MAN/SSNFS.
 *
 * Copyright 2018 Maxwell Dreytser
 */

#include "ssnfsworker.h"
#include <ssnfsserver.h>
#include <log.h>
#include <serversettings.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <QStorageInfo>
#include <QElapsedTimer>

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)
#define HELLO_STR "SSNFS server version " STR(_SERVER_VERSION)

#define ToChr(x) x.toUtf8().data()

SSNFSWorker::SSNFSWorker(int socketDescriptor, QObject *parent)
    : QThread(parent), socketDescriptor(socketDescriptor)
{
    knownResultCodes.insert(201, "Created");
    knownResultCodes.insert(202, "Accepted");
    knownResultCodes.insert(203, "Non-Authoritative Information");
    knownResultCodes.insert(204, "No Content");
    knownResultCodes.insert(205, "Reset Content");
    knownResultCodes.insert(206, "Partial Content");
    knownResultCodes.insert(300, "Multiple Choices");
    knownResultCodes.insert(301, "Moved Permanently");
    knownResultCodes.insert(302, "Found");
    knownResultCodes.insert(303, "See Other");
    knownResultCodes.insert(304, "Not Modified");
    knownResultCodes.insert(307, "Temporary Redirect");
    knownResultCodes.insert(308, "Permanent Redirect");
    knownResultCodes.insert(400, "Bad Request");
    knownResultCodes.insert(401, "Unauthorized");
    knownResultCodes.insert(403, "Forbidden");
    knownResultCodes.insert(404, "Not Found");
    knownResultCodes.insert(405, "Method Not Allowed");
    knownResultCodes.insert(406, "Not Acceptable");
    knownResultCodes.insert(407, "Proxy Authentication Required");
    knownResultCodes.insert(408, "Request Timeout");
    knownResultCodes.insert(409, "Conflict");
    knownResultCodes.insert(410, "Gone");
    knownResultCodes.insert(411, "Length Required");
    knownResultCodes.insert(412, "Precondition Failed");
    knownResultCodes.insert(413, "Payload Too Large");
    knownResultCodes.insert(414, "URI Too Long");
    knownResultCodes.insert(415, "Unsupported Media Type");
    knownResultCodes.insert(416, "Range Not Satisfiable");
    knownResultCodes.insert(417, "Expectation Failed");
    knownResultCodes.insert(418, "I'm a teapot");
    knownResultCodes.insert(426, "Upgrade Required");
    knownResultCodes.insert(428, "Precondition Required");
    knownResultCodes.insert(429, "Too Many Requests");
    knownResultCodes.insert(431, "Request Header Fields Too Large");
    knownResultCodes.insert(451, "Unavailable For Legal Reasons");
    knownResultCodes.insert(500, "Internal Server Error");
    knownResultCodes.insert(501, "Not Implemented");
    knownResultCodes.insert(502, "Bad Gateway");
    knownResultCodes.insert(503, "Service Unavailable");
    knownResultCodes.insert(504, "Gateway Timeout");
    knownResultCodes.insert(505, "HTTP Version Not Supported");
    knownResultCodes.insert(511, "Network Authentication Required");

    configDB = QSqlDatabase::cloneDatabase(getConfDB(), tr("SSNFSWorker(%1, %2)").arg(socketDescriptor).arg(QUuid::createUuid().toString()));
}

void SSNFSWorker::run()
{
    SSNFSServer *parent = ((SSNFSServer*)this->parent());
    configDB.open();

    socket = new QSslSocket();

    socket->setPeerVerifyMode(QSslSocket::VerifyPeer);
    socket->setLocalCertificate(parent->certificate);
    socket->setPrivateKey(parent->privateKey);
    //socket->setProtocol(QSsl::TlsV1SslV3);

    QList<QSslError> errorsThatCanBeIgnored;

    errorsThatCanBeIgnored<<QSslError(QSslError::HostNameMismatch);
    errorsThatCanBeIgnored<<QSslError(QSslError::SelfSignedCertificate);

    socket->ignoreSslErrors(errorsThatCanBeIgnored);

    if (!socket->setSocketDescriptor(socketDescriptor)) {
        // TODO: Log socket.error()
        qInfo() << socket->errorString();
        qInfo() << socket->sslErrors();
        return;
    }

    socket->startServerEncryption();

    connect(socket, static_cast<void(QSslSocket::*)(const QList<QSslError> &)>(&QSslSocket::sslErrors),
            [this](QList<QSslError> errors) {
        socket->ignoreSslErrors(errors);
    });

    socket->waitForEncrypted(-1);
    qInfo() << socket->errorString();
    qInfo() << socket->sslErrors();
    socket->flush();

    while (socket->isOpen() && socket->isEncrypted()) {
        //socket->waitForReadyRead(-1);
        ReadyToRead();
        socket->flush();
        if (socket->bytesToWrite() > 0) {
            socket->waitForBytesWritten(-1);
        }
    }

    //deleteLater();
    this->quit();
}

QString SSNFSWorker::getPerms(QString path, qint32 uid) {
    if (path.endsWith('/'))
        path.truncate(path.length() - 1);
    if (path == "") {
        return defaultPerms;
    }

    QSqlQuery getItemPerms(configDB);
    getItemPerms.prepare(R"(
        SELECT `Perms`
        FROM `User_Share_Perms` usp
        LEFT JOIN `Client_Users`cu
        ON usp.`Client_User_Key` = cu.`Client_User_Key`
        WHERE usp.`Share_Key` = ? AND usp.`Client_Key` = ? AND usp.`Path` = ? AND (cu.`User_UID` = ? OR cu.`User_UID` IS NULL)
        ORDER BY `User_UID` DESC
        LIMIT 1; )");
    getItemPerms.addBindValue(shareKey);
    getItemPerms.addBindValue(clientKey);
    getItemPerms.addBindValue(path);
    getItemPerms.addBindValue(uid);
    if (getItemPerms.exec() == false) {
        Log::error(Log::Categories["File System"], "Client {0}, Share {1}: Error retrieving permissions from DB: (2)",
                ToChr(clientName), ToChr(shareName), getItemPerms.lastError().text().toUtf8().data());
        return QString();
    }
    if (getItemPerms.next() == false) {
        return getPerms(path.mid(0, path.lastIndexOf('/')), uid);
    } else {
        return getItemPerms.value(0).toString();
    }
}

void SSNFSWorker::ReadyToRead()
{
    if (working)
        return;

    switch (status) {
    case WaitingForHello:
    {
        working = true;
        Common::ResultCode clientResult = Common::getResultFromBytes(Common::readExactBytes(socket, 1));
        if (clientResult == Common::HTTP_GET || clientResult == Common::HTTP_POST) {
            processHttpRequest((uint8_t)clientResult);
            socket->flush();
            if (socket->bytesToWrite() > 0) {
                socket->waitForBytesWritten(-1);
            }
            socket->close();
            return;
        } else if (clientResult == Common::Hello) {
            if (socket->peerCertificate().isNull()) {
                Log::warn(Log::Categories["Authentication"], "Client from IP {0} didn't provide a client certificate.", socket->peerAddress().toString().toUtf8().data());
                socket->close();
                return;
            }
        } else {
            if (socket->isOpen()) {
                socket->close();
            }
            Log::warn(Log::Categories["Authentication"], "Client from IP {0} sent an invalid Hello code.", socket->peerAddress().toString().toUtf8().data());
            return;
        }
        qint32 clientHelloLen = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));
        QByteArray clientHello = Common::readExactBytes(socket, clientHelloLen);

        if (clientHello.isNull()) {
            if (socket->isOpen()) {
                socket->close();
            }
            Log::warn(Log::Categories["Authentication"], "Client from IP {0} sent an invalid Hello message.", socket->peerAddress().toString().toUtf8().data());
            return;
        }

        Log::info(Log::Categories["Authentication"], "Client from IP {0} connecting with Hello message: {1}", socket->peerAddress().toString().toUtf8().data(), clientHello.data());

        //qInfo() << socket->peerCertificate().toPem();
        QSqlQuery getUserKey(configDB);
        QString clientCert = socket->peerCertificate().toPem();
        getUserKey.prepare(R"(
            SELECT cc.Client_Key, cc.Client_Info, c.Client_Name
            FROM Client_Certs cc
            JOIN Clients c
            ON cc.Client_Key = c.Client_Key
            WHERE cc.Client_Cert = ?; )");
        getUserKey.addBindValue(clientCert);
        if (getUserKey.exec() == false) {
            qInfo() << getUserKey.executedQuery();
            Log::error(Log::Categories["Authentication"], "Unable to get the user cert from the DB: {0}", getUserKey.lastError().text().toUtf8().data());
            socket->close();
            return;
        }
        if (getUserKey.next() == false) {
            Log::error(Log::Categories["Authentication"], "The cerificate provided by the client at IP {0} does not match any existing users.", socket->peerAddress().toString().toUtf8().data());
            socket->readAll();
            socket->write(Common::getBytes(Common::Error));
            QByteArray errorMsg = "Invalid authentication. Either you provided a bad certificate or your system info has changed. You may need to register your certificate.";
            socket->write(Common::getBytes((qint32)errorMsg.length()));
            socket->write(errorMsg);
            socket->close();
            return;
        }
        clientKey = getUserKey.value(0).toLongLong();
        clientName = getUserKey.value(2).toString();

        qint32 clientSysInfoLen = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));
        QByteArray clientSysInfo = Common::readExactBytes(socket, clientSysInfoLen);

        if (clientSysInfo.isNull()) {
            if (socket->isOpen()) {
                socket->close();
            }
            Log::warn(Log::Categories["Authentication"], "Client from IP {0} sent an invalid SysInfo message.", socket->peerAddress().toString().toUtf8().data());
            return;
        }

        QString clientSysInfoStr(clientSysInfo);
        if (clientSysInfoStr != getUserKey.value(1).toString()) {
            Log::error(Log::Categories["Authentication"], "The system info provided by the client at IP {0} does not match the certificate used.", socket->peerAddress().toString().toUtf8().data());
            socket->readAll();
            socket->write(Common::getBytes(Common::Error));
            QByteArray errorMsg = "Invalid authentication. Either you provided a bad certificate or your system info has changed. You may need to register your certificate.";
            socket->write(Common::getBytes((qint32)errorMsg.length()));
            socket->write(errorMsg);
            socket->close();
            return;
        }

        socket->write(Common::getBytes(Common::Hello));
        socket->write(Common::getBytes((qint32)strlen(HELLO_STR)));
        socket->write(HELLO_STR);

        status = WaitingForShare;
        working = false;
    }
        break;
    case WaitingForShare:
    {
        working = true;
        Common::ResultCode clientResult = Common::getResultFromBytes(Common::readExactBytes(socket, 1));
        if (clientResult != Common::Share) {
            if (socket->isOpen()) {
                socket->close();
            }
            Log::warn(Log::Categories["Authentication"], "Client from IP {0} sent an invalid Share code.", socket->peerAddress().toString().toUtf8().data());
            return;
        }
        qint32 clientShareLen = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));
        QByteArray clientShare = Common::readExactBytes(socket, clientShareLen);

        if (clientShare.isNull()) {
            if (socket->isOpen()) {
                socket->close();
            }
            Log::warn(Log::Categories["Authentication"], "Client from IP {0} sent an invalid Share.", socket->peerAddress().toString().toUtf8().data());
            return;
        }

        QSqlQuery getShare(configDB);
        getShare.prepare(R"(
            SELECT s.`Share_Key`, s.`Local_Path`, cs.`Default_Perms`
            FROM `Client_Shares` cs
            JOIN `Shares` s
            ON cs.`Client_Key` = ? AND s.`Share_Name` = ? AND cs.`Share_Key` = s.`Share_Key`; )");
        getShare.addBindValue(clientKey);
        getShare.addBindValue(QString(clientShare));

        if (getShare.exec() == false) {
            Log::error(Log::Categories["Authentication"], "Unable to get the share from the DB: {0}", getShare.lastError().text().toUtf8().data());
            socket->close();
            return;
        }
        if (getShare.next() == false) {
            Log::error(Log::Categories["Authentication"], "The client at IP {0} requested a share that is either unavailable to them or does not exist: {1}", socket->peerAddress().toString().toUtf8().data(), clientShare.data());
            socket->readAll();
            socket->write(Common::getBytes(Common::Error));
            QByteArray errorMsg = "Invalid Share specified. Either it is unavailable to you or it does not exist.";
            socket->write(Common::getBytes((qint32)errorMsg.length()));
            socket->write(errorMsg);
            socket->close();
            return;
        }

        shareKey = getShare.value(0).toLongLong();
        localPath = getShare.value(1).toString();
        defaultPerms = getShare.value(2).toString();
        shareName = clientShare;

        Log::info(Log::Categories["Authentication"], "Client from IP {0} subscribed to Share: {1}", socket->peerAddress().toString().toUtf8().data(), clientShare.data());
        socket->write(Common::getBytes(Common::OK));
        status = WaitingForOperation;
        working = false;
    }
        break;
    case WaitingForOperation:
    {
        working = true;
        Common::Operation currOperation = Common::getOperationFromBytes(Common::readExactBytes(socket, 2));

        if (currOperation == Common::InvalidOperation) {
            qDebug() << "Error! Invalid operation from client.";
            socket->close();
            deleteLater();
            return;
        }

        socket->write(Common::getBytes(Common::OK));

        status = InOperation;
        operation = currOperation;
        working = false;
    }
        break;
    case InOperation:
    {
        working = true;
        if (operation == Common::UpdateLocalUsers) {
            while (true) {
                qint32 usernameLength = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));
                if (usernameLength == -1) {
                    break;
                }
                QString username = Common::readExactBytes(socket, usernameLength);
                quint32 uid = Common::getUInt32FromBytes(Common::readExactBytes(socket, 4));

                QSqlQuery addUser(configDB);
                addUser.prepare(R"(
                    INSERT OR IGNORE INTO Client_Users (Client_Key, User_UID, User_Name)
                    VALUES (?, ?, ?); )");
                addUser.addBindValue(clientKey);
                addUser.addBindValue(uid);
                addUser.addBindValue(username);
                if (addUser.exec() == false) {
                    Log::error(Log::Categories["Authentication"], "Client {0} Share {1}: Failed to add local user {2} to the database: {3}",
                            ToChr(clientName), ToChr(shareName), ToChr(username), ToChr(addUser.lastError().text()));
                }
            }

            socket->write(Common::getBytes(Common::OK));

            status = WaitingForOperation;
        } else if (operation == Common::getattr) {
            working = true;

            qint32 uid = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

            quint32 pathLength = Common::getUInt32FromBytes(Common::readExactBytes(socket, 4));

            QByteArray targetPath = Common::readExactBytes(socket, pathLength);

            int res;

            struct stat stbuf;

            QString finalPath(localPath);
            finalPath.append(targetPath);

            res = lstat(ToChr(finalPath), &stbuf);

            long decimalnum, quotient;
            int octalNumber = 0;

            quotient = decimalnum = stbuf.st_mode;
            for (int i = 0; quotient != 0; i++)
            {
                octalNumber += quotient % 8 * pow(10, i);
                quotient = quotient / 8;
            }

            if (octalNumber <= 7777777 && octalNumber >= 0) {
                int newFileMode = 0;
                QString perms = getPerms(targetPath, uid);
                if (perms.contains('x'))
                    newFileMode += 1;
                if (perms.contains('w'))
                    newFileMode += 2;
                if (perms.contains('r'))
                    newFileMode += 4;

                octalNumber = floor((double)octalNumber / 1000) * 1000 + newFileMode * 100;

                decimalnum = 0;
                for (int i = 0; octalNumber != 0; i++)
                {
                    decimalnum =  decimalnum +(octalNumber % 10)* pow(8, i);
                    octalNumber = octalNumber / 10;
                }

                stbuf.st_mode = decimalnum;
            }

            if (res == -1)
                res = -errno;

            socket->write(Common::getBytes(res));

            QByteArray statData;
            statData.fill('\x00', sizeof(struct stat));
            memcpy(statData.data(), &stbuf, sizeof(struct stat));

            //socket->write((char*)(&statData), sizeof(stbuf));
            socket->write(statData);

            status = WaitingForOperation;

            working = false;
        } else if (operation == Common::readdir) {
            working = true;

            qint32 uid = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

            quint16 pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

            QByteArray targetPath = Common::readExactBytes(socket, pathLength);

            int res;

            QString perms = getPerms(targetPath, uid);
            if (!perms.contains('r')) {
                Log::info(Log::Categories["File System"], "Client {0} Share {1}: User tried to read directory {2} but didn't have permission.",
                        ToChr(clientName), ToChr(shareName), targetPath.data());

                res = -EACCES;

                socket->write(Common::getBytes(res));

                status = WaitingForOperation;

                working = false;
                return;
            }

            QString finalPath(localPath);
            finalPath.append(targetPath);

            DIR *dp;
            struct dirent *de;

            dp = opendir(ToChr(finalPath));
            if (dp == NULL) {
                res = -errno;

                socket->write(Common::getBytes(res));

                status = WaitingForOperation;

                working = false;
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

            status = WaitingForOperation;

            working = false;
        } else if (operation == Common::open) {
            working = true;

            qint32 uid = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

            quint16 pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

            QByteArray targetPath = Common::readExactBytes(socket, pathLength);

            int res;

            QString finalPath(localPath);
            finalPath.append(targetPath);

            qint32 oflags = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

            if ((oflags & O_CREAT) && !QFile::exists(finalPath)) {
                QString perms = getPerms(targetPath.mid(0, targetPath.lastIndexOf('/')), uid);
                if (!perms.contains('w')) {
                    Log::info(Log::Categories["File System"], "Client {0} Share {1}: User tried to create file {2} but didn't have permission.",
                            ToChr(clientName), ToChr(shareName), targetPath.data());

                    res = -EACCES;

                    socket->write(Common::getBytes(res));

                    status = WaitingForOperation;

                    working = false;
                    return;
                }
            }

            QString perms = getPerms(targetPath, uid);
            if ((oflags & O_TRUNC) || (oflags & O_WRONLY) || (oflags & O_RDWR)) {
                if (!perms.contains('w')) {
                    Log::info(Log::Categories["File System"], "Client {0} Share {1}: User tried to open file {2} for writing but didn't have permission.",
                            ToChr(clientName), ToChr(shareName), targetPath.data());

                    res = -EACCES;

                    socket->write(Common::getBytes(res));
                    status = WaitingForOperation;
                    working = false;
                    return;
                }
            }
            if ((oflags & O_RDONLY) || (oflags & O_RDWR)) {
                if (!perms.contains('r')) {
                    Log::info(Log::Categories["File System"], "Client {0} Share {1}: User tried to open file {2} for reading but didn't have permission.",
                            ToChr(clientName), ToChr(shareName), targetPath.data());

                    res = -EACCES;

                    socket->write(Common::getBytes(res));
                    status = WaitingForOperation;
                    working = false;
                    return;
                }
            }

            res = open(ToChr(finalPath), oflags);
            if (res == -1)
                res = -errno;
            else {
                int realFd = res;
                int FakeFd = -1;
                for (int i = 1; i < std::numeric_limits<int>::max(); i++) {
                    if (fds.keys().contains(i) == false) {
                        FakeFd = i;
                        break;
                    }
                }
                if (FakeFd == -1)
                    res = -EBADFD;
                else {
                    fds.insert(FakeFd, realFd);
                    res = FakeFd;
                }
            }

            socket->write(Common::getBytes(res));

            status = WaitingForOperation;

            working = false;
        } else if (operation == Common::read) {
            working = true;

            qint32 uid = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));
            (void) uid;

            quint16 pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

            QByteArray targetPath = Common::readExactBytes(socket, pathLength);

            QString finalPath(localPath);
            finalPath.append(targetPath);

            // Get the internal File Descriptor.
            qint32 fakeFd = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

            qint64 readSize = Common::getInt64FromBytes(Common::readExactBytes(socket, 8));

            qint64 readOffset = Common::getInt64FromBytes(Common::readExactBytes(socket, 8));

            int res;

            QByteArray readBuf;
            readBuf.fill('\x00', readSize);
            int actuallyRead = -1;

            // Find the corresponding actual File Descriptor
            int realFd = fds.value(fakeFd);

            // We will now use the proc virtual filesystem to look up info about the File Descriptor.
            QString procFdPath = tr("/proc/self/fd/%1").arg(realFd);

            struct stat procFdInfo;

            lstat(ToChr(procFdPath), &procFdInfo);

            QVector<char> actualFdFilePath;
            actualFdFilePath.fill('\x00', procFdInfo.st_size + 1);

            ssize_t r = readlink(ToChr(procFdPath), actualFdFilePath.data(), procFdInfo.st_size + 1);

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
                        actuallyRead = pread(realFd, readBuf.data(), readSize, readOffset);
                        res = actuallyRead;
                    } else {
                        res = -EBADF;
                    }
                }
            }

            socket->write(Common::getBytes(res));

            if (res > 0) {
                readBuf.resize(actuallyRead);
                socket->write(readBuf);
            }

            status = WaitingForOperation;

            working = false;
        } else if (operation == Common::access) {
            working = true;

            qint32 uid = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

            quint16 pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

            QByteArray targetPath = Common::readExactBytes(socket, pathLength);

            int res;

            QString finalPath(localPath);
            finalPath.append(targetPath);

            int mask = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

            res = access(ToChr(finalPath), mask);

            if (res == 0) {
                QString perms = getPerms(targetPath, uid);
                if (mask & W_OK) {
                    if (!perms.contains('w')) {
                        // Since this is just a permission check, there is no reason to log a violation.

                        res = -EACCES;
                    }
                }
                if (mask & R_OK) {
                    if (!perms.contains('r')) {
                        // Since this is just a permission check, there is no reason to log a violation.

                        res = -EACCES;
                    }
                }
                if (mask & X_OK) {
                    if (!perms.contains('x')) {
                        // Since this is just a permission check, there is no reason to log a violation.

                        res = -EACCES;
                    }
                }
            }

            socket->write(Common::getBytes(res));

            status = WaitingForOperation;

            working = false;
        } else if (operation == Common::readlink) {
            working = true;

            qint32 uid = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

            quint16 pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

            QByteArray targetPath = Common::readExactBytes(socket, pathLength);

            int res;

            QString finalPath(localPath);
            finalPath.append(targetPath);

            int size = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

            QString perms = getPerms(targetPath, uid);
            if (!perms.contains('r')) {
                socket->write(Common::getBytes(-EACCES));
            } else {
                QByteArray buf;
                buf.fill('\0', size);

                res = readlink(ToChr(finalPath), buf.data(), size - 1);

                if (res == -1)
                    socket->write(Common::getBytes(-errno));
                else {
                    QString result = buf.data();

                    socket->write(Common::getBytes(result.length()));

                    socket->write(ToChr(result));
                }
            }

            status = WaitingForOperation;

            working = false;
        } else if (operation == Common::mknod) {
            working = true;

            qint32 uid = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

            quint16 pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

            QByteArray targetPath = Common::readExactBytes(socket, pathLength);

            int res;

            QString finalPath(localPath);
            finalPath.append(targetPath);

            unsigned int mode = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

            QString perms = getPerms(targetPath.mid(0, targetPath.lastIndexOf('/')), uid);
            if (!perms.contains('w')) {
                Log::info(Log::Categories["File System"], "Client {0} Share {1}: User tried to create file {2} but didn't have permission.",
                        ToChr(clientName), ToChr(shareName), targetPath.data());

                res = -EACCES;

                socket->write(Common::getBytes(res));
                status = WaitingForOperation;
                working = false;
                return;
            }

            long decimalnum, quotient;
            int octalNumber = 0;

            quotient = decimalnum = mode;
            for (int i = 0; quotient != 0; i++)
            {
                octalNumber += quotient % 8 * pow(10, i);
                quotient = quotient / 8;
            }

            if (octalNumber <= 7777777 && octalNumber > 0) {
                int newFileMode = 0;

                QString newFileModeStr = ServerSettings::get("NewFileMode");
                bool isint = false;
                if (newFileModeStr.isNull()) {
                    Log::error(Log::Categories["File System"], "No default file mode for local files has been set. Denying file creation.");
                    res = -EACCES;
                } else if ((newFileMode = newFileModeStr.toUInt(&isint)) > 777 || !isint) {
                    Log::error(Log::Categories["File System"], "Invalid default file mode for local files has been set. Denying file creation.");
                    res = -EACCES;
                } else {
                    // Chop off the old modes and set new ones.
                    octalNumber = floor((double)octalNumber / 1000) * 1000 + newFileMode;

                    decimalnum = 0;
                    for (int i = 0; octalNumber != 0; i++)
                    {
                        decimalnum =  decimalnum + (octalNumber % 10) * pow(8, i);
                        octalNumber = octalNumber / 10;
                    }

                    mode = decimalnum;

                    if (S_ISFIFO(mode))
                        res = mkfifo(ToChr(finalPath), mode);
                    else if (S_ISREG(mode) || S_ISSOCK(mode)) // We will not support creating directories or symlinks here.
                        res = mknod(ToChr(finalPath), mode, (dev_t) 0);
                    else
                        res = -EPERM;

                    if (res == -1)
                        res = -errno;
                }
            } else {
                res = -EINVAL;
            }

            socket->write(Common::getBytes(res));

            status = WaitingForOperation;

            working = false;
        } else if (operation == Common::mkdir) {
            working = true;

            qint32 uid = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

            quint16 pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

            QByteArray targetPath = Common::readExactBytes(socket, pathLength);

            int res;

            QString finalPath(localPath);
            finalPath.append(targetPath);

            unsigned int mode = Common::getUInt32FromBytes(Common::readExactBytes(socket, 4));

            QString perms = getPerms(targetPath.mid(0, targetPath.lastIndexOf('/')), uid);
            if (!perms.contains('w')) {
                Log::info(Log::Categories["File System"], "Client {0} Share {1}: User tried to create folder {2} but didn't have permission.",
                        ToChr(clientName), ToChr(shareName), targetPath.data());

                res = -EACCES;
            } else {
                res = mkdir(ToChr(finalPath), mode);

                if (res == -1)
                    res = -errno;
            }

            socket->write(Common::getBytes(res));

            status = WaitingForOperation;

            working = false;
        } else if (operation == Common::unlink) {
            working = true;

            qint32 uid = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

            quint16 pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

            QByteArray targetPath = Common::readExactBytes(socket, pathLength);

            int res;

            QString finalPath(localPath);
            finalPath.append(targetPath);

            QString parentPerms = getPerms(targetPath.mid(0, targetPath.lastIndexOf('/')), uid);
            if (!parentPerms.contains('w')) {
                Log::info(Log::Categories["File System"], "Client {0} Share {1}: User tried to delete file {2} but didn't have permission.",
                        ToChr(clientName), ToChr(shareName), targetPath.data());

                res = -EACCES;
            } else {
                res = unlink(ToChr(finalPath));

                if (res == -1)
                    res = -errno;
                else {
                    QSqlQuery delOldPerms(configDB);
                    delOldPerms.prepare(R"(
                        DELETE FROM `User_Share_Perms`
                        WHERE `Path` = ?; )");
                    delOldPerms.addBindValue(QString(targetPath));
                    if (delOldPerms.exec() == false) {
                        Log::warn(Log::Categories["File System"], "Error deleting perms from DB after removing file {0}: {1}",
                                targetPath.data(), delOldPerms.lastError().text().toUtf8().data());
                        // We shouldn't bother the user with this problem, especially since this may not even be an issue...
                    }
                }
            }

            socket->write(Common::getBytes(res));

            status = WaitingForOperation;

            working = false;
        } else if (operation == Common::rmdir) {
            working = true;

            qint32 uid = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

            quint16 pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

            QByteArray targetPath = Common::readExactBytes(socket, pathLength);

            int res;

            QString finalPath(localPath);
            finalPath.append(targetPath);

            QString parentPerms = getPerms(targetPath.mid(0, targetPath.lastIndexOf('/')), uid);
            if (!parentPerms.contains('w')) {
                Log::info(Log::Categories["File System"], "Client {0} Share {1}: User tried to delete directory {2} but didn't have permission.",
                        ToChr(clientName), ToChr(shareName), targetPath.data());

                res = -EACCES;
            } else {
                res = rmdir(ToChr(finalPath));

                if (res == -1)
                    res = -errno;
                else {
                    QSqlQuery delOldPerms(configDB);
                    delOldPerms.prepare(R"(
                                        DELETE FROM `User_Share_Perms`
                                        WHERE `Path` = ?; )");
                    delOldPerms.addBindValue(QString(targetPath));
                    if (delOldPerms.exec() == false) {
                        Log::warn(Log::Categories["File System"], "Error deleting perms from DB after removing directory {0}: {1}",
                                targetPath.data(), delOldPerms.lastError().text().toUtf8().data());
                        // We shouldn't bother the user with this problem, especially since this may not even be an issue...
                    }
                }
            }

            socket->write(Common::getBytes(res));

            status = WaitingForOperation;

            working = false;
        } else if (operation == Common::symlink) {
            working = true;

            qint32 uid = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

            quint16 targetPathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

            QByteArray targetPath = Common::readExactBytes(socket, targetPathLength);

            quint16 linkPathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

            QByteArray linkPath = Common::readExactBytes(socket, linkPathLength);

            int res;

            QString finalPath(localPath);
            finalPath.append(linkPath);

            QString parentPerms = getPerms(linkPath.mid(0, linkPath.lastIndexOf('/')), uid);
            if (!parentPerms.contains('w')) {
                Log::info(Log::Categories["File System"], "Client {0} Share {1}: User tried create a symlink {2} but didn't have permission.",
                        ToChr(clientName), ToChr(shareName), targetPath.data());

                res = -EACCES;
            } else {
                res = symlink(targetPath.data(), ToChr(finalPath));

                if (res == -1)
                    res = -errno;
            }

            socket->write(Common::getBytes(res));

            status = WaitingForOperation;

            working = false;
        } else if (operation == Common::rename) {
            working = true;

            qint32 uid = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

            quint16 FromPathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

            QByteArray FromPath = Common::readExactBytes(socket, FromPathLength);

            quint16 ToPathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

            QByteArray ToPath = Common::readExactBytes(socket, ToPathLength);

            int res;

            QString finalFromPath(localPath);
            finalFromPath.append(FromPath);
            QString finalToPath(localPath);
            finalToPath.append(ToPath);

            QString fromParentPerms = getPerms(FromPath.mid(0, FromPath.lastIndexOf('/')), uid);
            QString toParentPerms = getPerms(ToPath.mid(0, ToPath.lastIndexOf('/')), uid);
            if (!fromParentPerms.contains('w')) {
                Log::info(Log::Categories["File System"], "Client {0} Share {1}: User tried create move file {2} to {3} but didn't have permission.",
                        ToChr(clientName), ToChr(shareName), FromPath.data(), ToPath.data());

                res = -EACCES;
            } else if (!toParentPerms.contains('w')) {
                Log::info(Log::Categories["File System"], "Client {0} Share {1}: User tried create move file {2} to {3} but didn't have permission.",
                        ToChr(clientName), ToChr(shareName), FromPath.data(), ToPath.data());

                res = -EACCES;
            } else {
                res = rename(ToChr(finalFromPath), ToChr(finalToPath));

                if (res == -1)
                    res = -errno;
                else {
                    QSqlQuery moveOldPerms(configDB);
                    moveOldPerms.prepare(R"(
                        UPDATE `User_Share_Perms`
                        SET `Path` = ?
                        WHERE `Path` = ?; )");
                    moveOldPerms.addBindValue(QString(ToPath));
                    moveOldPerms.addBindValue(QString(FromPath));
                    if (moveOldPerms.exec() == false) {
                        Log::warn(Log::Categories["File System"], "Error moving perms in DB after moving file {0}: {1}",
                                FromPath.data(), moveOldPerms.lastError().text().toUtf8().data());
                        // We shouldn't bother the user with this problem, especially since this may not even be an issue...
                    }
                }
            }

            socket->write(Common::getBytes(res));

            status = WaitingForOperation;

            working = false;
        } else if (operation == Common::chmod) {
            working = true;

            qint32 uid = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

            quint16 pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

            QByteArray targetPath = Common::readExactBytes(socket, pathLength);

            int res;

            QString finalPath(localPath);
            finalPath.append(targetPath);

            unsigned int mode = Common::getUInt32FromBytes(Common::readExactBytes(socket, 4));
            (void) mode;

            if (!QFile::exists(finalPath)) {
                res = -EEXIST;
            } else {
                QString perms = getPerms(targetPath, uid);
                if (!perms.contains('o')) {
                    Log::info(Log::Categories["File System"], "Client {0} Share {1}: User tried to change permissions on file {2} without being the file's owner.",
                            ToChr(clientName), ToChr(shareName), targetPath.data());

                    res = -EACCES;
                } else {
                    // Do nothing.
                    // No actual permission changes are allowed using system commands!
                    res = 0;
                }
            }

            socket->write(Common::getBytes(res));

            status = WaitingForOperation;

            working = false;
        } else if (operation == Common::chown) {
            working = true;

            qint32 reqUid = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

            quint16 pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

            QByteArray targetPath = Common::readExactBytes(socket, pathLength);

            int res;

            QString finalPath(localPath);
            finalPath.append(targetPath);

            unsigned int uid = Common::getUInt32FromBytes(Common::readExactBytes(socket, 4));
            (void) uid;
            unsigned int gid = Common::getUInt32FromBytes(Common::readExactBytes(socket, 4));
            (void) gid;

            if (!QFile::exists(finalPath)) {
                res = -EEXIST;
            } else {
                QString perms = getPerms(targetPath, reqUid);
                if (!perms.contains('o')) {
                    Log::info(Log::Categories["File System"], "Client {0} Share {1}: User tried to change permissions on file {2} without being the file's owner.",
                            ToChr(clientName), ToChr(shareName), targetPath.data());

                    res = -EACCES;
                } else {
                    // Do nothing.
                    // No actual permission changes are allowed using system commands!
                    res = 0;
                }
            }

            socket->write(Common::getBytes(res));

            status = WaitingForOperation;

            working = false;
        } else if (operation == Common::truncate) {
            working = true;

            qint32 uid = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

            quint16 pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

            QByteArray targetPath = Common::readExactBytes(socket, pathLength);

            int res;

            QString finalPath(localPath);
            finalPath.append(targetPath);

            quint32 size = Common::getUInt32FromBytes(Common::readExactBytes(socket, 4));

            QString perms = getPerms(targetPath, uid);

            if (perms.contains('w')) {
                res = truncate(ToChr(finalPath), size);

                if (res == -1)
                    res = -errno;
            } else {
                res = -EACCES;
            }

            socket->write(Common::getBytes(res));

            status = WaitingForOperation;

            working = false;
        } else if (operation == Common::utimens) {
            working = true;

            qint32 uid = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

            quint16 pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

            QByteArray targetPath = Common::readExactBytes(socket, pathLength);

            int res;

            QString finalPath(localPath);
            finalPath.append(targetPath);

            timespec ts[2];
            ts[0].tv_sec = Common::getInt64FromBytes(Common::readExactBytes(socket, 8));
            ts[0].tv_nsec = Common::getInt64FromBytes(Common::readExactBytes(socket, 8));
            ts[1].tv_sec = Common::getInt64FromBytes(Common::readExactBytes(socket, 8));
            ts[1].tv_nsec = Common::getInt64FromBytes(Common::readExactBytes(socket, 8));

            QString perms = getPerms(targetPath, uid);

            if (perms.contains('w')) {
                res = utimensat(0, ToChr(finalPath), ts, AT_SYMLINK_NOFOLLOW);

                if (res == -1)
                    res = -errno;
            } else {
                res = -EACCES;
            }

            socket->write(Common::getBytes(res));

            status = WaitingForOperation;

            working = false;
        } else if (operation == Common::write) {
            working = true;

            QElapsedTimer timer;
            timer.start();

            qint32 uid = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));
            (void) uid;

            quint16 pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

            QByteArray targetPath = Common::readExactBytes(socket, pathLength);

            QString finalPath(localPath);
            finalPath.append(targetPath);

            // Get the internal File Descriptor.
            int fakeFd = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

            qint64 Size = Common::getInt64FromBytes(Common::readExactBytes(socket, 8));

            qint64 Offset = Common::getInt64FromBytes(Common::readExactBytes(socket, 8));

            qDebug() << "Got Metadata:" << timer.nsecsElapsed();

            QByteArray dataToWrite;

            dataToWrite.append(Common::readExactBytes(socket, Size));

            int res;

            int actuallyWritten = -1;

            // Find the corresponding actual File Descriptor
            int realFd = fds.value(fakeFd);

            // We will now use the proc virtual filesystem to look up info about the File Descriptor.
            QString procFdPath = tr("/proc/self/fd/%1").arg(realFd);

            struct stat procFdInfo;

            lstat(ToChr(procFdPath), &procFdInfo);

            QVector<char> actualFdFilePath;
            actualFdFilePath.fill('\x00', procFdInfo.st_size + 1);

            ssize_t r = readlink(ToChr(procFdPath), actualFdFilePath.data(), procFdInfo.st_size + 1);

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
            qDebug() << res << "bytes, Done Writing." << timer.nsecsElapsed();

            status = WaitingForOperation;

            working = false;
        } else if (operation == Common::writebulk) {
            working = true;

            QTime timer;
            timer.start();

            qint32 uid = Common::getInt32FromBytes(Common::readExactBytes(socket, 4, false));
            (void) uid;

            quint32 numOfRequests = Common::getUInt32FromBytes(Common::readExactBytes(socket, 4, false));

            qint32 result = 0;

            for (quint32 i = 0; i < numOfRequests; i++) {

                quint16 pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2, false));

                QByteArray targetPath = Common::readExactBytes(socket, pathLength, false);

                QString finalPath(localPath);
                finalPath.append(targetPath);

                // Get the internal File Descriptor.
                int fakeFd = Common::getInt32FromBytes(Common::readExactBytes(socket, 4, false));

                qint64 Size = Common::getInt64FromBytes(Common::readExactBytes(socket, 8, false));

                qint64 Offset = Common::getInt64FromBytes(Common::readExactBytes(socket, 8, false));

                qDebug() << "Got Metadata:" << timer.elapsed();

                QByteArray dataToWrite;

                dataToWrite.append(Common::readExactBytes(socket, Size, false));

                qDebug() << "Got Data:" << timer.elapsed();

                int res;

                int actuallyWritten = -1;

                // Find the corresponding actual File Descriptor
                int realFd = fds.value(fakeFd);

                // We will now use the proc virtual filesystem to look up info about the File Descriptor.
                QString procFdPath = tr("/proc/self/fd/%1").arg(realFd);

                struct stat procFdInfo;

                lstat(ToChr(procFdPath), &procFdInfo);

                QVector<char> actualFdFilePath;
                actualFdFilePath.fill('\x00', procFdInfo.st_size + 1);

                ssize_t r = readlink(ToChr(procFdPath), actualFdFilePath.data(), procFdInfo.st_size + 1);

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
                qDebug() << res << "bytes, Done Writing." << timer.elapsed();

                if (res < 0)
                    result = res;
            }

            socket->write(Common::getBytes(result));

            status = WaitingForOperation;

            working = false;
        } else if (operation == Common::release) {
            working = true;

            qint32 uid = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));
            (void) uid;

            quint16 pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

            QByteArray targetPath = Common::readExactBytes(socket, pathLength);

            QString finalPath(localPath);
            finalPath.append(targetPath);

            int res;

            qint32 fakeFd = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));

            int realFd = fds.value(fakeFd);

            // We will now use the proc virtual filesystem to look up info about the File Descriptor.
            QString procFdPath = tr("/proc/self/fd/%1").arg(realFd);

            struct stat procFdInfo;

            lstat(ToChr(procFdPath), &procFdInfo);

            QVector<char> actualFdFilePath;
            actualFdFilePath.fill('\x00', procFdInfo.st_size + 1);

            ssize_t r = readlink(ToChr(procFdPath), actualFdFilePath.data(), procFdInfo.st_size + 1);

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

            fds.remove(fakeFd);

            status = WaitingForOperation;

            working = false;
        } else if (operation == Common::statfs) {
            working = true;

            qint32 uid = Common::getInt32FromBytes(Common::readExactBytes(socket, 4));
            (void) uid;

            quint16 pathLength = Common::getUInt16FromBytes(Common::readExactBytes(socket, 2));

            QByteArray targetPath = Common::readExactBytes(socket, pathLength);

            QString finalPath(localPath);
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

            status = WaitingForOperation;

            working = false;
        }

        working = false;
    }
        break;
    }
}
