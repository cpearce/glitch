#pragma once

#include <windows.h>

using namespace std;

class Receiver {
public:
  Receiver(HANDLE pipe, HWND hwnd);
  ~Receiver();

  void close();

  void run();

private:
  HANDLE pipe;
  HANDLE thread;
  DWORD threadId;
  HWND hwnd;
};
