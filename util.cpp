#include "windows.h"
#include "util.h"
#include <string>

using namespace std;

void MOZ_CRASH(const char* msg) {
  fprintf(stderr, "%s\n", msg);
  exit(-1);
}

void cp_log(const char* fmt, ...) {
  const DWORD tempDirLen = MAX_PATH+1;
  WCHAR tmpDir[tempDirLen] = { 0 };
  DWORD pathLen = GetTempPathW(tempDirLen, tmpDir);
  if (pathLen == 0) {
    MOZ_CRASH("cp_log can't get temp dir");
  }

  std::wstring path(tmpDir);
  path += L"cp_log.txt";

  HANDLE h = CreateFileW(
    path.c_str(), // lpFileName
    GENERIC_WRITE, // dwDesiredAccess
    FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
    nullptr, // lpSecurityAttributes
    OPEN_ALWAYS, // dwCreationDisposition
    0, // dwFlagsAndAttributes
    nullptr); // hTemplateFile

  if (h == INVALID_HANDLE_VALUE) {
    MOZ_CRASH("cp_log failed with invalid handle");
  }

  OVERLAPPED overlapped;
  memset(&overlapped, 0, sizeof(OVERLAPPED));
  HRESULT hr = LockFileEx(h, LOCKFILE_EXCLUSIVE_LOCK, 0, !0, !0, &overlapped);
  if (FAILED(hr)) {
    MOZ_CRASH("cp_log lock failed");
  }

  // Seek to EOF.
  LARGE_INTEGER li;
  memset(&li, 0, sizeof(LARGE_INTEGER));
  SetFilePointerEx(h, li, nullptr, FILE_END);

  DWORD bytesWritten = 0;
  const size_t len = 5 * 1024;
  char buf[len];
  va_list ap;
  va_start(ap, fmt);
  int bytesToWrite = vsnprintf(buf, len, fmt, ap);
  va_end(ap);
  BOOL ok = WriteFile(h,
                      buf,
                      bytesToWrite,
                      &bytesWritten,
                      NULL);

  if (ok == FALSE || bytesWritten != bytesToWrite) {
    MOZ_CRASH("cp_log write failed");
  }

  const char eol[] = "\n";
  ok = WriteFile(h,
                 eol,
                 strlen(eol),
                 &bytesWritten,
                 NULL);
  if (ok == FALSE || bytesWritten != strlen(eol)) {
    MOZ_CRASH("cp_log write failed to write eol");
  }

  //printf("%s\n", buf);

  UnlockFileEx(h, 0, !0, !0, &overlapped);

  CloseHandle(h);
}

void ErrorExit(const char* msg) 
{ 
  cp_log(msg);
  ExitProcess(1);
}

vector<string>
tokenize(const string& str, const string& delimiters)
{
  vector<string> tokens;
  // Skip delimiters at beginning.
  string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  string::size_type pos     = str.find_first_of(delimiters, lastPos);
  while (string::npos != pos || string::npos != lastPos) {
    // Found a token, add it to the vector.
    tokens.push_back(str.substr(lastPos, pos - lastPos));
    // Skip delimiters.  Note the "not_of"
    lastPos = str.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = str.find_first_of(delimiters, lastPos);
  }
  return tokens;
}
