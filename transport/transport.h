// transport.h

// Функция 1 которая получает на вход сообщение и сериализует его для отправки
// Функция 2, которая получает на вход сокет и сообщение, сереализует с помощью
// функции 1 и отправляет в сокет.

#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <VersionHelpers.h>
#include <winbase.h>
#include <windows.h>
#include <winioctl.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#include "../collector/collectorr.h"
#include "../collector/commands_coll.h"
#include "../const.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;
using namespace std;

// сборка всех даыннх в одно сообщение
Message GetMess(Message& mess);
// отправляет сначала длину потом само сообщение
void SendMessageAndMessageSize(SOCKET& client_socket,
                               const std::string& jsonStr);

Message GetMess(Message& mess) {
    bool IAmNotOK = false;
    if (IAmNotOK) {
        strcpy_s(mess.header.type, "IAmNotOK");
    } else if (mess.header.agent_id == 0) {
        strcpy_s(mess.header.type, "whoami");
    } else {
        strcpy_s(mess.header.type, "SimplePCReport");
    }

    mess.payload.disks = GetDisks();
    mess.payload.ram = GetRam();
    mess.payload.cpu = GetCpu();
    mess.payload.system = GetOs();
    mess.payload.hardware = GetHardware();
    mess.payload.ping_m = GetPing();

    return mess;
}

string Message::toJson() const {
    json j;

    // Заголовок
    j["header"] = {
        {"agent_id", header.agent_id},
        {"type", string(header.type)}  // char[64] → string
    };

    j["payload"] = {
        {"ram",
         {{"total_mb", payload.ram.total_mb},
          {"free_mb", payload.ram.free_mb}}},
        {"cpu", {{"cores", payload.cpu.cores}, {"usage", payload.cpu.usage}}},
        {"system",
         {{"hostname", payload.system.hostname},
          {"domain", payload.system.domain},
          {"version", payload.system.version},
          {"timestamp", payload.system.timestamp}}},
        {"disks", json::array()},
        {"hardware",
         {{"bios", payload.hardware.bios},
          {"cpu", payload.hardware.cpu},
          {"mac", json::array()},
          {"video", json::array()}}},
        {"ping", payload.ping_m.ping_millisec}};

    // Диски
    for (const auto& disk : payload.disks) {
        j["payload"]["disks"].push_back({{"total_mb", disk.total_mb},
                                         {"free_mb", disk.free_mb},
                                         {"is_hdd", disk.is_hdd}});
    }

    for (const auto& mac : payload.hardware.mac) {
        j["payload"]["hardware"]["mac"].push_back(mac);
    }

    for (const auto& video : payload.hardware.video) {
        j["payload"]["hardware"]["video"].push_back(video);
    }

    return j.dump();
}

void SendMessageAndMessageSize(SOCKET& client_socket, const string& jsonStr) {
    if ((jsonStr.size()) > kMaxBuf) {
        cout << "Message length exceeds the allowed sending length. Message "
                "cannot be sent."
             << endl;
        return;
    }

    // 1. Подготовка длины (в network byte order)
    uint32_t len = static_cast<uint32_t>(jsonStr.size());
    uint32_t lenNetwork = htonl(len);  // host → network (big-endian)

    // 2. Отправка длины
    if (send(client_socket, reinterpret_cast<const char*>(&lenNetwork),
             sizeof(lenNetwork), 0) == SOCKET_ERROR) {
        cout << "send(char_size_message) failed with error: "
             << WSAGetLastError() << endl;
        return;
    }

    if (send(client_socket, jsonStr.c_str(), (int)jsonStr.size(), 0) ==
        SOCKET_ERROR) {
        cout << "send() failed with error: " << WSAGetLastError() << endl;
        return;
    }
}

#endif