// const.h

#ifndef CONST_H
#define CONST_H

#pragma once

#include <cstdint> // для uint32_t

inline constexpr const char* kFileName = "id_file.txt";
inline constexpr const char* kEnvFile = ".env";

const uint32_t kMaxBuf = 1024;
const uint32_t kNumChar = 5; // количество разрядов kMaxBuf + 1 (на \0)

#endif