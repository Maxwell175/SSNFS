#include <cstdint>
#include <QByteArray>
#include <QIODevice>

namespace Common {
    const uint8_t MAX_RESULTCODE = 1;
    enum ResultCode : uint8_t {
        Hello = 0,
        OK = 1,
        Error = 2,

        InvalidResult = UINT8_MAX
    };

    const uint16_t MAX_OPERATION = 11;
    enum Operation : uint16_t {
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

        InvalidOperation = UINT16_MAX
    };

    inline uint16_t getUInt16FromBytes(QByteArray input) {
        uint16_t result = 0;
        result |= ((uint16_t)(uint8_t)input[0]) << 8;
        result |= ((uint16_t)(uint8_t)input[1]);
        return result;
    }
    inline QByteArray getBytes(uint16_t input) {
        QByteArray resultBytes;
        resultBytes.append((input >> 8) & 0xFF);
        resultBytes.append(input & 0xFF);
        return resultBytes;
    }
    inline int32_t getInt32FromBytes(QByteArray input) {
        int32_t result = 0;
        result |= ((int32_t)(uint8_t)input[0]) << 24;
        result |= ((int32_t)(uint8_t)input[1]) << 16;
        result |= ((int32_t)(uint8_t)input[2]) << 8;
        result |= ((int32_t)(uint8_t)input[3]);
        return result;
    }
    inline uint32_t getUInt32FromBytes(QByteArray input) {
        uint32_t result = 0;
        result |= ((uint32_t)(uint8_t)input[0]) << 24;
        result |= ((uint32_t)(uint8_t)input[1]) << 16;
        result |= ((uint32_t)(uint8_t)input[2]) << 8;
        result |= ((uint32_t)(uint8_t)input[3]);
        return result;
    }
    inline QByteArray getBytes(int32_t input) {
        QByteArray resultBytes;
        resultBytes.append((input >> 24) & 0xFF);
        resultBytes.append((input >> 16) & 0xFF);
        resultBytes.append((input >> 8) & 0xFF);
        resultBytes.append(input & 0xFF);
        return resultBytes;
    }
    inline QByteArray getBytes(uint32_t input) {
        QByteArray resultBytes;
        resultBytes.append((input >> 24) & 0xFF);
        resultBytes.append((input >> 16) & 0xFF);
        resultBytes.append((input >> 8) & 0xFF);
        resultBytes.append(input & 0xFF);
        return resultBytes;
    }

    inline QByteArray getBytes(ResultCode result) {
        uint8_t input = (uint8_t)result;
        QByteArray resultBytes;
        resultBytes.append(input & 0xFF);
        return resultBytes;
    }
    inline QByteArray getBytes(Operation oper) {
        return getBytes((uint16_t)oper);
    }
    inline ResultCode getResultFromBytes(QByteArray input) {
        uint8_t result = 0;
        result |= ((uint8_t)input[0]);

        if (result > MAX_RESULTCODE)
            return ResultCode::InvalidResult;
        else
            return (ResultCode) result;
    }
    inline Operation getOperationFromBytes(QByteArray input) {
        uint16_t result = getUInt16FromBytes(input);
        if (result > MAX_OPERATION)
            return Operation::InvalidOperation;
        else
            return (Operation) result;
    }

    inline QByteArray readExactBytes(QIODevice *input, int length, int timeout = 1) {
        QByteArray result;
        while (result.length() < length) {
            if (input->bytesAvailable() == 0) {
                input->waitForReadyRead(timeout);
            }
            result.append(input->read(length - result.length()));
        }
        return result;
    }
}
