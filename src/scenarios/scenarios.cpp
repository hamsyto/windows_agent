#include "scenarios.h"

using namespace std;

int Registrate(Payload payload, IConnection* connection,
               const Settings& settings) {
  if (!connection) return -1;

  try {
    // 1. Инициализируем соединение, если оно еще не установлено
    if (!connection->Connect()) {
      cout << "Reg error: Could not establish connection to server" << endl;
      return -1;
    }

    Message message = {Header{settings.agent_id, "whoami"}, payload};
    string json_message = message.toJson();

    Transport transport(settings, *connection);

    // 2. Отправка данных
    transport.SendData(json_message);
    // 3. Получение ID (4 байта)
    string result_bytes = transport.RecvData();

    if (result_bytes.size() != sizeof(int32_t)) {
      cout << "Reg error: Invalid response size from server" << endl;
      return -1;
    }

    int32_t network_id;
    memcpy(&network_id, result_bytes.data(), sizeof(int32_t));
    int host_id = static_cast<int>(ntohl(network_id));
    connection->Disconnect();
    return host_id;

  } catch (const exception& e) {
    cout << "Reg error: " << e.what() << endl;
    connection->Disconnect();
    return -1;
  }
}

void SendReport(Payload payload, IConnection* connection,
                const Settings& settings) {
  if (!connection) return;

  try {
    // 1. Проверяем/устанавливаем соединение перед отправкой
    if (!connection->Connect()) {
      cout << "Send error: Server is unreachable" << endl;
      return;
    }

    Message message = {Header{settings.agent_id, "SimplePCReport"}, payload};
    string json_message = message.toJson();

    Transport transport(settings, *connection);
    transport.SendData(json_message);

    cout << "Report successfully sent for agent_id: " << settings.agent_id
         << endl;

  } catch (const exception& e) {
    cout << "Send error: " << e.what() << endl;
  }
  connection->Disconnect();
}

int Work(Settings& settings) {
  // connection и collector - это unique_ptr
  auto connection = CreateConnection(settings);
  auto collector = CreateCollector(settings);

  // 1. Регистрация
  if (settings.agent_id < 1) {
    int newID = Registrate(collector->GetPayload(), connection.get(), settings);
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
}