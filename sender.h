#pragma once

#include <windows.h>
#include <mutex>
#include <condition_variable>
#include <queue>

using namespace std;


class Sender {
public:
  Sender(HANDLE _out);
  ~Sender();

  void send(char ch);
  void close();

private:

  static void static_run(Sender* self);
  void run();

  HANDLE out;
  mutex m;
  condition_variable cv;
  queue<char> outgoing;
  bool shutdown = false;
  thread t;
};
