// message.cpp

#include "message.h"

#include <json.hpp>

#include "../collector/windows/collector.h"

using namespace std;
using json = nlohmann::json;

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
      {"ram", {{"total", payload.ram.total}, {"used", payload.ram.used}}},
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
    j["payload"]["disks"].push_back({{"number", disk.number},
                                     {"mount", disk.mount},
                                     {"total", disk.total},
                                     {"used", disk.used},
                                     {"type", disk.type}});
  }

  for (const auto& mac : payload.hardware.mac) {
    j["payload"]["hardware"]["mac"].push_back(mac);
  }

  for (const auto& video : payload.hardware.video) {
    j["payload"]["hardware"]["video"].push_back(video);
  }

  return j.dump();
}
