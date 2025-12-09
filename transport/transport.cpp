// transport.cpp

// Функция 1 которая получает на вход сообщение и сериализует его для отправки
// Функция 2, которая получает на вход сокет и сообщение, сереализует с помощью
// функции 1 и отправляет в сокет.

#include <winsock2.h>
// порядок подключения
#include <windows.h>
// специализированные заголовки Windows
#include <VersionHelpers.h>
#include <winioctl.h>

#include <cstdint>
#include <iostream>
#include <string>

#include "../const.h"
#include "nlohmann/json.hpp"
#include "transport.h"

using json = nlohmann::json;
using namespace std;

Message GetMess(Message& msg) {
    if (msg.header.agent_id == 0) {
        msg.header.type = "whoami";
    } else if (msg.header.type != "error") {
        msg.header.type = "SimplePCReport";
    }

    msg.payload.disks = GetDisks();
    msg.payload.ram = GetRam();
    msg.payload.cpu = GetCpu();
    msg.payload.system = GetOs();
    msg.payload.hardware = GetHardware();
    msg.payload.ping_m = GetPing();

    return msg;
}

string Message::toJson() const {
    json j;

    // Заголовок
    j["header"] = {{"agent_id", header.agent_id}, {"type", header.type}};

    // payload в случае ошибки
    if (header.type == "error") {
        j["payload"] = {{"error", payload.error_text}};
        return j.dump();
    }

    j["payload"] = {
        {"ram",
         {{"total", payload.ram.total},
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

    for (const auto& disk : payload.disks) {
        j["payload"]["disks"].push_back({{"total", disk.total},
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
