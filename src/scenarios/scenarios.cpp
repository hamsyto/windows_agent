#include "scenarios.h"

int Registrate(SimplePCReport payload, IConnection* connection,
               const Settings& settings) {
  if (!connection) return -1;
  try {
    Message message = {Header{settings.agentID, "whoami"}, payload};
    string json_message = message.toJson();

    Transport transport(settings, *connection);
    transport.SendData(json_message);

    string result_bytes = transport.RecvData();

    if (result_bytes.size() != sizeof(int32_t)) {
      cout << "Invalid response size" << endl;
      return -1;
    }

    int32_t network_id;
    memcpy(&network_id, result_bytes.data(), sizeof(int32_t));
    return static_cast<int>(ntohl(network_id));

  } catch (const exception& e) {
    cout << "Reg error: " << e.what() << endl;
    return -1;
  }
}

void SendReport(SimplePCReport payload, IConnection* connection,
                const Settings& settings) {
  if (!connection) return;
  try {
    Message message = {Header{settings.agentID, "SimplePCReport"}, payload};
    Transport transport(settings, *connection);
    transport.SendData(message.toJson());
  } catch (const exception& e) {
    cout << "Send error: " << e.what() << endl;
  }
}
