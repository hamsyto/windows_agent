#include "tests.h"

#include <iostream>

#include "../models/message.h"

using namespace std;

bool TestConnection(Settings& settings) {
  auto connection = CreateConnection(settings);
  auto instance = connection.get();
  try {
    instance->Connect();  // Проверить Timeout, если сервер выключен
    instance->Send("Hello World!");
    instance->Disconnect();
  } catch (const exception& e) {
    cout << "Connection error: " << e.what() << endl;
    return false;
  }
  return true;
}

bool TestCollector(Settings& settings) {
  auto collector = CreateCollector(settings);
  auto instance = collector.get();
  try {
    Payload payload = instance->GetPayload();
    Message message = {Header{-123, "test_header"}, payload};
    cout << message.toJson(4) << endl;

  } catch (const exception& e) {
    cout << "Collect error: " << e.what() << endl;
    return false;
  }
  return true;
}
