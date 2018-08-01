/*
 * SSNFS Client v0.1
 *
 * Available under the license(s) specified at https://github.com/MDTech-us-MAN/SSNFS.
 *
 * Copyright 2018 Maxwell Dreytser
 */

#ifndef COMMON_H
#define COMMON_H

#include <cstdint>
#include <QByteArray>
#include <QAbstractSocket>
#include <QTime>
#include <QFile>
#include <QList>
#include <QHostInfo>
#include <QThread>
#include <unistd.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>
#include <QStringList>

#include <fastpbkdf2.h>

#define ToChr(x) x.toUtf8().data()

namespace Common {
const quint8 MAX_RESULTCODE = 4;
enum ResultCode : quint8 {
    Null = 0,
    Hello = 1,
    OK = 2,
    Error = 3,
    Share = 4,

    HTTP_GET = 'G',
    HTTP_POST = 'P',

    InvalidResult = UINT8_MAX
};

const quint16 MAX_OPERATION = 21;
enum Operation : quint16 {
    getattr = 1,
    readdir = 2,
    open = 3,
    read = 4,
    access = 5,
    readlink = 6,
    mknod = 7,
    mkdir = 8,
    unlink = 9,
    rmdir = 10,
    symlink = 11,
    rename = 12,
    chmod = 13,
    chown = 14,
    truncate = 15,
    utimens = 16,
    write = 17,
    writebulk = 18,
    release = 19,
    statfs = 20,
    UpdateLocalUsers = 21,

    InvalidOperation = UINT16_MAX
};

inline quint16 getInt16FromBytes(QByteArray input) {
    quint16 result = 0;
    result |= ((qint16)(quint8)input[0]) << 8;
    result |= ((qint16)(quint8)input[1]);
    return result;
}
inline QByteArray getBytes(qint16 input) {
    QByteArray resultBytes;
    resultBytes.append((input >> 8) & 0xFF);
    resultBytes.append(input & 0xFF);
    return resultBytes;
}
inline quint16 getUInt16FromBytes(QByteArray input) {
    quint16 result = 0;
    result |= ((quint16)(quint8)input[0]) << 8;
    result |= ((quint16)(quint8)input[1]);
    return result;
}
inline QByteArray getBytes(quint16 input) {
    QByteArray resultBytes;
    resultBytes.append((input >> 8) & 0xFF);
    resultBytes.append(input & 0xFF);
    return resultBytes;
}
inline qint32 getInt32FromBytes(QByteArray input) {
    qint32 result = 0;
    result |= ((qint32)(quint8)input[0]) << 24;
    result |= ((qint32)(quint8)input[1]) << 16;
    result |= ((qint32)(quint8)input[2]) << 8;
    result |= ((qint32)(quint8)input[3]);
    return result;
}
inline QByteArray getBytes(qint32 input) {
    QByteArray resultBytes;
    resultBytes.append((input >> 24) & 0xFF);
    resultBytes.append((input >> 16) & 0xFF);
    resultBytes.append((input >> 8) & 0xFF);
    resultBytes.append(input & 0xFF);
    return resultBytes;
}
inline quint32 getUInt32FromBytes(QByteArray input) {
    quint32 result = 0;
    result |= ((quint32)(quint8)input[0]) << 24;
    result |= ((quint32)(quint8)input[1]) << 16;
    result |= ((quint32)(quint8)input[2]) << 8;
    result |= ((quint32)(quint8)input[3]);
    return result;
}
inline QByteArray getBytes(quint32 input) {
    QByteArray resultBytes;
    resultBytes.append((input >> 24) & 0xFF);
    resultBytes.append((input >> 16) & 0xFF);
    resultBytes.append((input >> 8) & 0xFF);
    resultBytes.append(input & 0xFF);
    return resultBytes;
}
inline qint64 getInt64FromBytes(QByteArray input) {
    qint64 result = 0;
    result |= ((qint64)(quint8)input[0]) << 56;
    result |= ((qint64)(quint8)input[1]) << 48;
    result |= ((qint64)(quint8)input[2]) << 40;
    result |= ((qint64)(quint8)input[3]) << 32;
    result |= ((qint64)(quint8)input[4]) << 24;
    result |= ((qint64)(quint8)input[5]) << 16;
    result |= ((qint64)(quint8)input[6]) << 8;
    result |= ((qint64)(quint8)input[7]);
    return result;
}
inline QByteArray getBytes(qint64 input) {
    QByteArray resultBytes;
    resultBytes.append((input >> 56) & 0xFF);
    resultBytes.append((input >> 48) & 0xFF);
    resultBytes.append((input >> 40) & 0xFF);
    resultBytes.append((input >> 32) & 0xFF);
    resultBytes.append((input >> 24) & 0xFF);
    resultBytes.append((input >> 16) & 0xFF);
    resultBytes.append((input >> 8) & 0xFF);
    resultBytes.append(input & 0xFF);
    return resultBytes;
}
inline quint64 getUInt64FromBytes(QByteArray input) {
    quint64 result = 0;
    result |= ((quint64)(quint8)input[0]) << 56;
    result |= ((quint64)(quint8)input[1]) << 48;
    result |= ((quint64)(quint8)input[2]) << 40;
    result |= ((quint64)(quint8)input[3]) << 32;
    result |= ((quint64)(quint8)input[4]) << 24;
    result |= ((quint64)(quint8)input[5]) << 16;
    result |= ((quint64)(quint8)input[6]) << 8;
    result |= ((quint64)(quint8)input[7]);
    return result;
}
inline QByteArray getBytes(quint64 input) {
    QByteArray resultBytes;
    resultBytes.append((input >> 56) & 0xFF);
    resultBytes.append((input >> 48) & 0xFF);
    resultBytes.append((input >> 40) & 0xFF);
    resultBytes.append((input >> 32) & 0xFF);
    resultBytes.append((input >> 24) & 0xFF);
    resultBytes.append((input >> 16) & 0xFF);
    resultBytes.append((input >> 8) & 0xFF);
    resultBytes.append(input & 0xFF);
    return resultBytes;
}

inline QByteArray getBytes(ResultCode result) {
    quint8 input = (quint8)result;
    QByteArray resultBytes;
    resultBytes.append(input & 0xFF);
    return resultBytes;
}
inline QByteArray getBytes(Operation oper) {
    return getBytes((quint16)oper);
}
inline ResultCode getResultFromBytes(QByteArray input) {
    quint8 result = 0;
    result |= ((quint8)input[0]);

    if (result > MAX_RESULTCODE && result != HTTP_GET && result != HTTP_POST)
        return ResultCode::InvalidResult;
    else
        return (ResultCode) result;
}
inline Operation getOperationFromBytes(QByteArray input) {
    quint16 result = getUInt16FromBytes(input);
    if (result == 0 || result > MAX_OPERATION)
        return Operation::InvalidOperation;
    else
        return (Operation) result;
}

inline QString resolveRelative(QString path) {
    QList<QString> pathParts = path.split('/');
    for (int i = 0; i < pathParts.length(); i++) {
        if (pathParts[i] == ".") {
            pathParts.removeAt(i);
            i--;
        } else if (pathParts[i] == "..") {
            pathParts.removeAt(i);
            if (i > 1)
                pathParts.removeAt(i - 1);
            i--;
            i--;
        } else if (pathParts[i] == "") {
            pathParts.removeAt(i);
            i--;
        }

        if (i < -1)
            i = -1;
    }
    QString result = QStringList(pathParts).join('/');
    if (!result.startsWith('/'))
        result.prepend('/');
    return result.trimmed();
}

inline QByteArray readExactBytes(QAbstractSocket *input, int length, bool write = false, int timeout = 5000, QTime timer = QTime::currentTime()) {
    int iterations = 0;
    QTime timer1;
    if (write)
        timer1.start();
    QByteArray result;
    while (result.length() < length) {
        iterations++;
        if (write)
            qInfo() << "Mark1:" << timer1.elapsed();
        if (input->bytesAvailable() == 0) {
            input->waitForReadyRead(timeout);
        }
        if (write)
            qInfo() << "Mark1.5:" << timer1.elapsed();
        result.append(input->read(length - result.length()));
        if (iterations > timeout) {
            return QByteArray();
        }
    }
    if (write)
        qInfo() << "Mark2:" << timer.elapsed();
    return result;
}

inline QByteArray getSystemInfo() {
    QByteArray SystemStr;
    SystemStr.append(QHostInfo::localHostName());
    SystemStr.append('|');
    QFile MBVendor("/sys/devices/virtual/dmi/id/board_vendor");
    if (MBVendor.open(QFile::ReadOnly)) {
        SystemStr.append(MBVendor.readAll());
        if (SystemStr.endsWith('\n'))
            SystemStr.truncate(SystemStr.length() - 1);
        SystemStr.append('|');
        MBVendor.close();
    }
    QFile MBName("/sys/devices/virtual/dmi/id/board_name");
    if (MBName.open(QFile::ReadOnly)) {
        SystemStr.append(MBName.readAll());
        if (SystemStr.endsWith('\n'))
            SystemStr.truncate(SystemStr.length() - 1);
        SystemStr.append('|');
        MBName.close();
    }
    QFile CPUInfo("/proc/cpuinfo");
    if (CPUInfo.open(QFile::ReadOnly)) {
        while (true) {
            QString currLine = CPUInfo.readLine();
            if (currLine.endsWith('\n'))
                currLine.truncate(currLine.length() - 1);
            if (currLine.isNull() || currLine.isEmpty())
                break;
            QList<QString> cpuInfoParts = currLine.split(": ");
            if (cpuInfoParts.length() < 2)
                continue;

            if (cpuInfoParts[0].startsWith("vendor_id")) {
                SystemStr.append(cpuInfoParts[1]);
                SystemStr.append('|');
            } else if (cpuInfoParts[0].startsWith("cpu family")) {
                SystemStr.append(cpuInfoParts[1]);
                SystemStr.append('|');
            } else if (cpuInfoParts[0].startsWith("model name")) {
                SystemStr.append(cpuInfoParts[1]);
                SystemStr.append('|');
            } else if (cpuInfoParts[0].startsWith("model")) {
                SystemStr.append(cpuInfoParts[1]);
                SystemStr.append('|');
            } else if (cpuInfoParts[0].startsWith("cpu cores")) {
                SystemStr.append(cpuInfoParts[1]);
                SystemStr.append('|');
            }
        }
        CPUInfo.close();
    }

    if (SystemStr.endsWith('|'))
        SystemStr.truncate(SystemStr.length() - 1);

    qCritical() << SystemStr;
    return SystemStr;
}

inline QString GetRandomString(const int randomStringLength = 16)
{
    const QString possibleCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!=");

    qsrand(time(0));

    QString randomString;
    for(int i=0; i<randomStringLength; ++i)
    {
        uint8_t index;
        if (RAND_bytes((unsigned char*)&index, 1) == 1) {
            index = index % possibleCharacters.length();
        } else {
            index = qrand() % possibleCharacters.length();
        }
        QChar nextChar = possibleCharacters.at(index);
        randomString.append(nextChar);
    }
    return randomString;
}

inline QString GetPasswordHash(QString password, QString salt = QString(), int iterations = 1024) {
    if (salt.isNull() || salt.isEmpty())
        salt = GetRandomString(32);

    QByteArray passwordBytes = password.toUtf8();
    QByteArray saltBytes = salt.toUtf8();

    QByteArray pbkdf2Out;
    pbkdf2Out.fill('\x00', 64);
    fastpbkdf2_hmac_sha512((const uint8_t*)passwordBytes.constData(), passwordBytes.length(),
                           (const uint8_t*)saltBytes.constData(), saltBytes.length(),
                           1024,
                           (uint8_t*)pbkdf2Out.data(), pbkdf2Out.length());

    QString finalResult;
    finalResult.append(pbkdf2Out.toBase64());
    finalResult.append('$');
    finalResult.append(saltBytes);
    finalResult.append('$');
    finalResult.append(QString::number(iterations));

    return finalResult;
}
}


#endif // COMMON_H
