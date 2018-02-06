//
// Created by maxwell on 2/1/18.
//
#include <openssl/ssl.h>
#include <cstdio>
#include <cstdint>
#include <vector>

#ifndef SSNFS_CLIENT_COMMON_H
#define SSNFS_CLIENT_COMMON_H

class Common {
public:

	static const uint8_t MAX_RESULTCODE = 2;
	enum ResultCode : uint8_t {
		Hello = 0,
		OK = 1,
		Error = 2,

		InvalidResult = UINT8_MAX
	};

	static const uint16_t MAX_OPERATION = 16;
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
		chmod = 12,
		chown = 13,
		truncate = 14,
		utimens = 15,
		write = 16,

		InvalidOperation = UINT16_MAX
	};

	static std::vector<char> getBytes(uint8_t input);

	static uint16_t getUInt16FromBytes(char *input);

	static std::vector<char> getBytes(uint16_t input);

	static int32_t getInt32FromBytes(char *input);

	static std::vector<char> getBytes(int32_t input);

	static uint32_t getUInt32FromBytes(char *input);

	static std::vector<char> getBytes(uint32_t input);

	static int64_t getInt64FromBytes(char *input);

	static std::vector<char> getBytes(int64_t input);

	static uint64_t getUInt64FromBytes(char *input);

	static std::vector<char> getBytes(uint64_t input);

	static std::vector<char> getBytes(ResultCode code);

	static ResultCode getResultFromBytes(char *input);

	static std::vector<char> getBytes(Operation code);

	static Operation getOperationFromBytes(char *input);
};

#endif //SSNFS_CLIENT_COMMON_H
