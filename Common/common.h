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
#include <QStringList>

namespace Common {
    const quint8 MAX_RESULTCODE = 3;
    enum ResultCode : quint8 {
        Hello = 0,
        OK = 1,
        Error = 2,
        Share = 3,

        HTTP_GET = 'G',

        InvalidResult = UINT8_MAX
    };

    const quint16 MAX_OPERATION = 20;
    enum Operation : quint16 {
        getattr = 0,
        readdir = 1,
        open = 2,
        read = 3,
        access = 4,
        readlink = 5,
        mknod = 6,
        mkdir = 7,
        unlink = 8,
        rmdir = 9,
        symlink = 10,
        rename = 11,
        chmod = 12,
        chown = 13,
        truncate = 14,
        utimens = 15,
        write = 16,
        writebulk = 17,
        release = 18,
        statfs = 19,
        UpdateLocalUsers = 20,

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

        if (result > MAX_RESULTCODE && result != 'G')
            return ResultCode::InvalidResult;
        else
            return (ResultCode) result;
    }
    inline Operation getOperationFromBytes(QByteArray input) {
        quint16 result = getUInt16FromBytes(input);
        if (result > MAX_OPERATION)
            return Operation::InvalidOperation;
        else
            return (Operation) result;
    }

    /*inline QByteArray readExactBytes(QByteArray &input, std::atomic<bool> &locker, int length, int timeout = 1, QTime timer = QTime::currentTime()) {
        QByteArray result;
        while (result.length() < length) {
            //while (locker) {}
            locker = true;
            if (input.isEmpty()) {
                locker = false;
                usleep(1000);
            } else {
                QByteArray currData = input.left(length - result.length());
                result.append(currData);
                input.remove(0, currData.length());
                locker = false;
            }
        }
        return result;
    }*/

    inline QByteArray readExactBytes(QAbstractSocket *input, int length, bool write = false, int timeout = 1, QTime timer = QTime::currentTime()) {
        QTime timer1;
        if (write)
            timer1.start();
        QByteArray result;
        while (result.length() < length) {
            if (write)
                qInfo() << "Mark1:" << timer1.elapsed();
            if (input->bytesAvailable() == 0) {
                input->waitForReadyRead(timeout);
            }
            if (write)
                qInfo() << "Mark1.5:" << timer1.elapsed();
            result.append(input->read(length - result.length()));
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

        return SystemStr;
    }
}


#endif // COMMON_H
