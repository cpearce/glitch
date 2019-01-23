#include <windows.h>
#include "util.h"

using namespace std;

int WINAPI ParentWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
  PWSTR pCmdLine, int nCmdShow);

int WINAPI ChildWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
  PWSTR pCmdLine, int nCmdShow, HWND parent);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
  PWSTR pCmdLine, int nCmdShow)
{
  vector<string> args(tokenize(narrow(wstring(pCmdLine))));
  if (args.size() >= 2 && args[0] == "child") {
    HWND parent = reinterpret_cast<HWND>(stoul(args[1]));
    return ChildWinMain(hInstance, hPrevInstance, pCmdLine, nCmdShow, parent);
  }
  return ParentWinMain(hInstance, hPrevInstance, pCmdLine, nCmdShow);
}
