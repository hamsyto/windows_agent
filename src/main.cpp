#include <fstream>
#include <iostream>

#include "collector/collector_fabric.h"
#include "connection/connection_fabric.h"
#include "consts.h"
#include "scenarios/scenarios.h"
#include "scenarios/tests.h"
#include "transport/transport.h"

using namespace std;

bool Initialize() {
#ifdef _WIN32
  WSADATA wsData;
  if (WSAStartup(MAKEWORD(2, 2), &wsData) != 0) {
    cout << "WSAStartup failed" << endl;
    return false;
  }
#endif
  return true;
}

bool Shutdown() {
#ifdef _WIN32
  WSACleanup();
#endif
  return true;
}

int main() {
  if (!Initialize()) return 1;

  Settings settings = LoadEnvSettings(kEnvFile);
  if (!settings.validate()) {
    std::cerr << "Не удалось загрузить настройки.\n";
    return 1;
  }
  cout << settings.ip_server << ":" << settings.port_server << endl;

  TestCollector(settings);
  // TestConnection(settings);
  // Work(settings);
  Shutdown();
  return 0;
}
