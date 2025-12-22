#include "test.h"

#include <iostream>

#include "../models/message.h"

using namespace std;

bool testConnection(IConnection& connection) {
  try {
    connection.Connect();
    connection.Send("Hello World!");
    connection.Disconnect();
  } catch (const exception& e) {
    cout << "Send error: " << e.what() << endl;
    return false;
  }
  return true;
}
bool testCollector(ICollector& collector) {
  try {
    Payload payload = collector.GetPayload();
    Message message = {Header{-123, "test_header"}, payload};
    cout << message.toJson(4) << endl;

  } catch (const exception& e) {
    cout << "Send error: " << e.what() << endl;
    return false;
  }
  return true;
}