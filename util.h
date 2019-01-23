#pragma once

#include <string>
#include <vector>

void cp_log(const char* fmt, ...);
void ErrorExit(const char* lpszFunction);

inline std::string narrow(std::wstring& wide) {
  std::string ns(wide.begin(), wide.end());
  return ns;
}

inline std::wstring wide(std::string& narrow) {
  std::wstring ws(narrow.begin(), narrow.end());
  return ws;
}

#define ARRAY_LENGTH(array_) \
  (sizeof(array_)/sizeof(array_[0]))

std::vector<std::string>
tokenize(const std::string& str, const std::string& delimiters = " ");

#define SAFE_RELEASE(ptr) { if (ptr) { ptr->Release(); ptr = nullptr; } }

class AutoComInit {
public:
  AutoComInit() {
    if (FAILED(CoInitializeEx(0, COINIT_MULTITHREADED))) {
      exit(-1);
    }
  }
  ~AutoComInit() {
    CoUninitialize();
  }
};
