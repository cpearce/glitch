#include <windows.h>
#include "util.h"
#include <initializer_list>
#include "sender.h"

using namespace std;


LRESULT CALLBACK ParentWndProc(HWND hwnd, UINT msg, 
    WPARAM wParam, LPARAM lParam)
{
  switch (msg) {

    //case WM_NCCALCSIZE: {
    //  RECT* clientRect =
    //      wParam ? &(reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam))->rgrc[0]
    //              : (reinterpret_cast<RECT*>(lParam));
    //  break;
    //};

    case WM_ERASEBKGND: {
      return 1;
    }

    case WM_KEYDOWN: {
      if (wParam == VK_ESCAPE) {
        cp_log("Parent WM_KEYDOWN VK_ESCAPE");

        Sender* sender = (Sender*)GetWindowLongPtr(hwnd, 0);
        if (sender) {
          sender->send('0');
        }
        else {
          ErrorExit("Can't send shutdown");
        }

        PostQuitMessage(0);
      }
      break;
    }

    case WM_PAINT: {
      Sender* sender = (Sender*)GetWindowLongPtr(hwnd, 0);
      if (sender) {
        sender->send('1');
      }
      else {
        //ErrorExit("Can't send paint");
      }
      break;
    }

    case WM_WINDOWPOSCHANGING: {
      Sender* sender = (Sender*)GetWindowLongPtr(hwnd, 0);
      if (sender) {
        sender->send('2');
      }
      break;
    }

    case WM_DESTROY: {
      cp_log("Parent WM_DESTROY");
      Sender* sender = (Sender*)GetWindowLongPtr(hwnd, 0);
      if (sender) {
        sender->send('0');
      }
      else {
        ErrorExit("Can't send shutdown");
      }
      PostQuitMessage(0);
      break;
    }


  };
  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

struct ChildPipes {
  HANDLE stdInRead = NULL;
  HANDLE stdInWrite = NULL;
  HANDLE stdOutRead = NULL;
  HANDLE stdOutWrite = NULL;

  void Close() {
    for (HANDLE h : {stdInRead, stdInWrite, stdOutRead, stdOutWrite}) {
      if (h) {
        CloseHandle(h);
      }
    }
  }
};

ChildPipes CreatePipes() {
  ChildPipes pipes = {0};
  SECURITY_ATTRIBUTES saAttr = {0};
  saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
  saAttr.bInheritHandle = TRUE; 
  saAttr.lpSecurityDescriptor = NULL; 

  // Create a pipe for the child process's STDOUT. 
  if (!CreatePipe(&pipes.stdOutRead, &pipes.stdOutWrite, &saAttr, 0)) {
    ErrorExit("StdoutRd CreatePipe");
  }

  // Ensure the read handle to the pipe for STDOUT is not inherited.
  if (!SetHandleInformation(pipes.stdOutRead, HANDLE_FLAG_INHERIT, 0)) {
    ErrorExit("Stdout SetHandleInformation");
  }

  // Create a pipe for the child process's STDIN. 
  if (!CreatePipe(&pipes.stdInRead, &pipes.stdInWrite, &saAttr, 0)) {
    ErrorExit("Stdin CreatePipe"); 
  }

  // Ensure the write handle to the pipe for STDIN is not inherited. 
  if (!SetHandleInformation(pipes.stdInWrite, HANDLE_FLAG_INHERIT, 0)) {
    ErrorExit("Stdin SetHandleInformation"); 
  }

  return pipes;
}

void CreateChildProcess(const ChildPipes* pipes, HWND parent) {
  TCHAR modPath[MAX_PATH] = {0};
  DWORD rv = GetModuleFileName(NULL, modPath, MAX_PATH);
  if (rv == 0) {
    cp_log("GetModuleFileNameW in parent process failed");
    exit(-1);
  }

  TCHAR cmdLine[MAX_PATH] = {0};
  sprintf_s(cmdLine, "%s %s %u", modPath, "child", reinterpret_cast<uint32_t>(parent));
  PROCESS_INFORMATION piProcInfo; 
  STARTUPINFO siStartInfo;

  ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );
 
  // Set up members of the STARTUPINFO structure. 
  // This structure specifies the STDIN and STDOUT handles for redirection.
 
  ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
  siStartInfo.cb = sizeof(STARTUPINFO); 
  siStartInfo.hStdError = pipes->stdOutWrite;
  siStartInfo.hStdOutput = pipes->stdOutWrite;
  siStartInfo.hStdInput = pipes->stdInRead;
  siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

  BOOL ok = CreateProcess(
      NULL,          // module name
      cmdLine,       // command line 
      NULL,          // process security attributes 
      NULL,          // primary thread security attributes 
      TRUE,          // handles are inherited 
      0,             // creation flags 
      NULL,          // use parent's environment 
      NULL,          // use parent's current directory 
      &siStartInfo,  // STARTUPINFO pointer 
      &piProcInfo);  // receives PROCESS_INFORMATION 
   
  if (!ok) {
    ErrorExit("CreateProcess failed");
  }
 }



int WINAPI ParentWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
  PWSTR pCmdLine, int nCmdShow)
{   
  cp_log("Parent process started");

  MSG  msg;    
  HWND hwnd;
  WNDCLASSW wc;

  wc.style         = CS_HREDRAW | CS_VREDRAW;
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = sizeof(Sender*);
  wc.lpszClassName = L"GlitchParentWindow";
  wc.hInstance     = hInstance;
  wc.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
  wc.lpszMenuName  = NULL;
  wc.lpfnWndProc   = ParentWndProc;
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
  
  RegisterClassW(&wc);
  hwnd = CreateWindowW(wc.lpszClassName, L"Parent Window",
              WS_SIZEBOX | WS_VISIBLE,
              100, 100, 350, 250, NULL, NULL, hInstance, NULL);  

  ShowWindow(hwnd, nCmdShow);
  UpdateWindow(hwnd);

  ChildPipes pipes = CreatePipes();
  CreateChildProcess(&pipes, hwnd);
  Sender sender(pipes.stdInWrite);
  // Make the sender findable by the windproc.
  SetWindowLongPtr(hwnd, 0, LONG_PTR(&sender));

  while (GetMessage(&msg, NULL, 0, 0)) {
    DispatchMessage(&msg);
  }

  sender.close();

  return (int) msg.wParam;
}

