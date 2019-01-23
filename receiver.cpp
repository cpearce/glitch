#include "receiver.h"
#include "util.h"

DWORD WINAPI RunReceiverThread(LPVOID lpParam) {
  Receiver* receiver = (Receiver*)(lpParam);
  receiver->run();
  return 0;
}

Receiver::Receiver(HANDLE _pipe, HWND _hwnd)
  : pipe(_pipe)
  , hwnd(_hwnd)
{
  thread = CreateThread(
    NULL, 0, RunReceiverThread, this, 0, &threadId
  );
  if (!thread) {
    ErrorExit("Child can't create receiver thread");
  }
}

Receiver::~Receiver() {
  close();
}

void Receiver::close() {
  CloseHandle(pipe);
  WaitForSingleObject(thread, INFINITE);
}

#define WM_IPC_MSG (WM_USER + 100)

void Receiver::run() {
  while (true) {
    char ch;
    DWORD bytesRead = 0;
    BOOL ok = ReadFile(pipe, &ch, 1, &bytesRead, NULL);
    if (!ok || bytesRead != 1) {
      cp_log("Receiver failed to receive");
      break;
    }
    ok = PostMessage(hwnd, WM_IPC_MSG, (UINT)ch, NULL);
    if (!ok) {
        cp_log("Child failed to PostMessage, err=%d", GetLastError());
    }
  }
}
