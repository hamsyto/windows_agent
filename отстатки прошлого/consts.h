// consts.h

#ifndef CONST_H
#define CONST_H

#pragma once

#include <cstdint>  // для uint32_t

inline constexpr const char* kFileName = "id_file.txt";
inline constexpr const char* kEnvFile = ".env";

const uint32_t kMaxBuf = 4096;

// Шифрование
const unsigned int kNonceSize = 12;  // количество байт для шума
const unsigned int kTagSize = 16;    // количество байт для тега
const unsigned int kKeySize = 32;    // количество байт для ключа

// Архивирование
namespace agent::compression {
constexpr int kLZ4CompressionLevel = 1;                    // Быстрая компрессия
constexpr size_t kMaxDecompressedSize = 10 * 1024 * 1024;  // 10 MB максимум
}  // namespace agent::compression

#endif