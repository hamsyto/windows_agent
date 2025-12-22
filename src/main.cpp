#include <fstream>
#include <iostream>

#include "collector/collector_fabric.h"
#include "connection/connection_fabric.h"
#include "consts.h"
#include "scenarios/scenarios.h"
#include "transport/transport.h"

using namespace std;

bool InitIalize() {
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
    if (!InitIalize()) return 1;

    Settings settings = LoadEnvSettings(kFileName);

    // connection и collector - это unique_ptr
    auto connection = CreateConnection(settings);
    auto collector = CreateCollector(settings);

    // 1. Регистрация
    if (settings.agent_id < 1) {
        int newID =
            Registrate(collector->GetPayload(), connection.get(), settings);
        if (newID != -1) {
            settings.agent_id = newID;
            cout << "New Agent ID: " << settings.agent_id << endl;
            UpdateAgentIdInEnv(".env", settings.agent_id);
        } else {
            return 1;
        }
    }

    // 2. Цикл отчетов
    while (true) {
        SendReport(collector->GetPayload(), connection.get(), settings);
        cout << "Report sent" << endl;
        Sleep(settings.idle_time * 1000);
    }

    Shutdown();
    return 0;
}
