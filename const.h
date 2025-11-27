// const.h

#pragma once

const char kIpAddr[] = "127.0.0.1";
const int kPort = 1117;    // порт 
const int kQueue = 3;    // количество сокет в очереди на подключение к серверу
const uint32_t kMaxBuf = 1024;
const uint32_t kNumChar = 5; // количество разрядов kMaxBuf + 1 (на \0)