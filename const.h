// const.h

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
const char kIpAddr[] = "127.0.0.1";
const int kPort = 8817;    // порт 
const uint32_t kMaxBuf = 1024;
const uint32_t kNumChar = 5; // количество разрядов kMaxBuf + 1 (на \0)