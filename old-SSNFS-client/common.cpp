//
// Created by maxwell on 2/1/18.
//

#include "common.h"

std::vector<char> Common::getBytes(uint8_t input) {
	std::vector<char> result;
	result.push_back(static_cast<char>(input & 0xFF));
	return result;
}

uint16_t Common::getUInt16FromBytes(char *input) {
	uint16_t result = 0;
	result |= ((uint16_t)(uint8_t) input[0]) << 8;
	result |= ((uint16_t)(uint8_t) input[1]);
	return result;
}

std::vector<char> Common::getBytes(uint16_t input) {
	std::vector<char> result;
	result.push_back(static_cast<char>((input >> 8) & 0xFF));
	result.push_back(static_cast<char>(input & 0xFF));
	return result;
}

int32_t Common::getInt32FromBytes(char *input) {
	int32_t result = 0;
	result |= ((int32_t) (uint8_t) input[0]) << 24;
	result |= ((int32_t) (uint8_t) input[1]) << 16;
	result |= ((int32_t) (uint8_t) input[2]) << 8;
	result |= ((int32_t) (uint8_t) input[3]);
	return result;
}

std::vector<char> Common::getBytes(int32_t input) {
	std::vector<char> result;
	result.push_back(static_cast<char>((input >> 24) & 0xFF));
	result.push_back(static_cast<char>((input >> 16) & 0xFF));
	result.push_back(static_cast<char>((input >> 8) & 0xFF));
	result.push_back(static_cast<char>(input & 0xFF));
	return result;
}

uint32_t Common::getUInt32FromBytes(char *input) {
	uint32_t result = 0;
	result |= ((uint32_t)(uint8_t) input[0]) << 24;
	result |= ((uint32_t)(uint8_t) input[1]) << 16;
	result |= ((uint32_t)(uint8_t) input[2]) << 8;
	result |= ((uint32_t)(uint8_t) input[3]);
	return result;
}

std::vector<char> Common::getBytes(uint32_t input) {
	std::vector<char> result;
	result.push_back(static_cast<char>((input >> 24) & 0xFF));
	result.push_back(static_cast<char>((input >> 16) & 0xFF));
	result.push_back(static_cast<char>((input >> 8) & 0xFF));
	result.push_back(static_cast<char>(input & 0xFF));
	return result;
}

int64_t Common::getInt64FromBytes(char *input) {
	int64_t result = 0;
	result |= ((int64_t) (uint8_t) input[0]) << 56;
	result |= ((int64_t) (uint8_t) input[1]) << 48;
	result |= ((int64_t) (uint8_t) input[2]) << 40;
	result |= ((int64_t) (uint8_t) input[3]) << 32;
	result |= ((int64_t) (uint8_t) input[4]) << 24;
	result |= ((int64_t) (uint8_t) input[5]) << 16;
	result |= ((int64_t) (uint8_t) input[6]) << 8;
	result |= ((int64_t) (uint8_t) input[7]);
	return result;
}

std::vector<char> Common::getBytes(int64_t input) {
	std::vector<char> result;
	result.push_back(static_cast<char>((input >> 56) & 0xFF));
	result.push_back(static_cast<char>((input >> 48) & 0xFF));
	result.push_back(static_cast<char>((input >> 40) & 0xFF));
	result.push_back(static_cast<char>((input >> 32) & 0xFF));
	result.push_back(static_cast<char>((input >> 24) & 0xFF));
	result.push_back(static_cast<char>((input >> 16) & 0xFF));
	result.push_back(static_cast<char>((input >> 8) & 0xFF));
	result.push_back(static_cast<char>(input & 0xFF));
	return result;
}

uint64_t Common::getUInt64FromBytes(char *input) {
	uint64_t result = 0;
	result |= ((uint64_t)(uint8_t) input[0]) << 56;
	result |= ((uint64_t)(uint8_t) input[1]) << 48;
	result |= ((uint64_t)(uint8_t) input[2]) << 40;
	result |= ((uint64_t)(uint8_t) input[3]) << 32;
	result |= ((uint64_t)(uint8_t) input[4]) << 24;
	result |= ((uint64_t)(uint8_t) input[5]) << 16;
	result |= ((uint64_t)(uint8_t) input[6]) << 8;
	result |= ((uint64_t)(uint8_t) input[7]);
	return result;
}

std::vector<char> Common::getBytes(uint64_t input) {
	std::vector<char> result;
	result.push_back(static_cast<char>((input >> 56) & 0xFF));
	result.push_back(static_cast<char>((input >> 48) & 0xFF));
	result.push_back(static_cast<char>((input >> 40) & 0xFF));
	result.push_back(static_cast<char>((input >> 32) & 0xFF));
	result.push_back(static_cast<char>((input >> 24) & 0xFF));
	result.push_back(static_cast<char>((input >> 16) & 0xFF));
	result.push_back(static_cast<char>((input >> 8) & 0xFF));
	result.push_back(static_cast<char>(input & 0xFF));
	return result;
}

std::vector<char> Common::getBytes(Common::ResultCode code) {
	uint8_t input = (uint8_t) code;
	return getBytes(input);
}

Common::ResultCode Common::getResultFromBytes(char *input) {
	uint8_t result = 0;
	result |= ((uint8_t) input[0]);

	if (result > MAX_RESULTCODE)
		return ResultCode::InvalidResult;
	else
		return (ResultCode) result;
}

std::vector<char> Common::getBytes(Common::Operation code) {
	return getBytes((uint16_t)code);
}

Common::Operation Common::getOperationFromBytes(char *input) {
	uint16_t result = getUInt16FromBytes(input);
	if (result > MAX_OPERATION)
		return Operation::InvalidOperation;
	else
		return (Operation) result;
}
