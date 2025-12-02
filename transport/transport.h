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
#include "nlohmann/json.hpp"
using json = nlohmann::json;

using namespace std;

Message GetMess(Message& mess) {
    mess = {};

    if (mess.header.agent_id == 0) {
        strcpy_s(mess.header.type, "whoami");
    } else {
        strcpy_s(mess.header.type, "SimplePCReport");
    }

    mess.payload.disks = GetDisks();
    mess.payload.ram = GetRam();
    mess.payload.cpu = GetCpu();
    mess.payload.system = GetOs();

    return mess;
}

string Message::toJson() const {
    json j;
    /*string agent_id_json;
        if (header.agent_id > 0) {
            agent_id_json = to_string(header.agent_id);
        } else {
            agent_id_json = "null";
        } */

    // Заголовок
    j["header"] = {
        {"agent_id", header.agent_id},
        {"type", string(header.type)}  // char[64] → string
    };

    // Для "whoami" — пустой payload
    /*
    if (string(header.type) == "whoami") {
        j["payload"] = json::object();  // пустой словарь
        return j.dump();
    }
    if (string(header.type) == "SimplePCReport") {*/
    j["payload"] = {
        {"ram",
         {{"total_mb", payload.ram.total_mb},
          {"free_mb", payload.ram.free_mb}}},
        {"cpu", {{"cores", payload.cpu.cores}, {"usage", payload.cpu.usage}}},
        {"system",
         {{"hostname", payload.system.hostname},
          {"version", payload.system.version},
          {"timestamp", payload.system.timestamp}}},
        {"disks", json::array()}};

    // Диски
    for (const auto& disk : payload.disks) {
        j["payload"]["disks"].push_back({{"total_mb", disk.total_mb},
                                         {"free_mb", disk.free_mb},
                                         {"is_hdd", disk.is_hdd}});
    }

    return j.dump();
}

#endif 