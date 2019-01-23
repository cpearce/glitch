#include "sender.h"
#include "util.h"

Sender::Sender(HANDLE _out)
: out(_out)
, t(static_run, this)
{
}

Sender::~Sender() {
  if (!shutdown) {
    close();
  }
}

void Sender::send(char ch) {
  unique_lock<mutex> lk(m);
  outgoing.push(ch);
  lk.unlock();
  cv.notify_all();
}

void Sender::close() {
  unique_lock<mutex> lk(m);
  shutdown = true;
  lk.unlock();
  cv.notify_all();
  t.join();
}

/* static */ void Sender::static_run(Sender* self) {
  self->run();
}

void Sender::run() {
  while (true) {
    queue<char> q;
    {
      std::unique_lock<std::mutex> lk(m);
      cv.wait(lk, [this]{return !outgoing.empty() || shutdown;});
      outgoing.swap(q);
    }

    while (!q.empty()) {
      char ch = q.front();
      q.pop();
      DWORD bytesWritten = 0;
      BOOL ok = WriteFile(out, &ch, 1, &bytesWritten, NULL);
      if (!ok) {
        ErrorExit("Failed to write");
      }
      else {
        cp_log("Sender sent %c", ch);
      }
    }

    if (shutdown) {
      CloseHandle(out);
      return;
    }
  }
}
