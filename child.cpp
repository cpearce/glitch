#include <Windows.h>
#include <D2d1.h>
#include "util.h"
#include "receiver.h"

#pragma comment(lib, "d2d1.lib")


#define WM_IPC_MSG (WM_USER + 100)

using namespace std;

static ID2D1DCRenderTarget* sRenderTarget = nullptr;
static ID2D1Factory* sD2DFactory = nullptr;
static ID2D1SolidColorBrush* sBlueBrush = nullptr;
static ID2D1SolidColorBrush* sRedBrush = nullptr;

// Should this not be static?
static PAINTSTRUCT sPaintStruct;
static HDC sHDC;

static HWND sParentHWND;

static bool Equals(const RECT& a, const RECT& b) {
  return a.left == b.left && a.right == b.right && a.top == b.top && a.bottom == b.bottom;
}

static void OnPaint(HWND hwnd, WPARAM wParam, LPARAM lParam) {

  //RECT prc;
  //GetClientRect(sParentHWND, &prc);
  //RECT crc;
  //GetClientRect(hwnd, &crc);
  //cp_log("OnPaint parent=%d,%d,%d,%d child=%d,%d,%d,%d",
  //  prc.left, prc.top, prc.right, prc.bottom,
  //  crc.left, crc.top, crc.right, crc.bottom
  //);
  //if (!Equals(prc, crc)) {
  //  SetWindowPos(hwnd, NULL, 0, 0, prc.right - prc.left, prc.bottom - prc.top,
  //    SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOOWNERZORDER | SWP_NOZORDER);
  //}

  sHDC = ::BeginPaint(hwnd, &sPaintStruct);
  sRenderTarget->BindDC(sPaintStruct.hdc, &sPaintStruct.rcPaint);
  sRenderTarget->BeginDraw();

  sRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
  sRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::HotPink));

  D2D1_SIZE_F rtSize = sRenderTarget->GetSize();
  int width = static_cast<int>(rtSize.width);
  int height = static_cast<int>(rtSize.height);
  const int size = 25;
  for (int y = 0; y < height; y += size) {
    for (int x = 0; x < width; x += size) {
      D2D1_RECT_F bounds = D2D1::RectF(FLOAT(x),
                                       FLOAT(y),
                                       FLOAT(x + size),
                                       FLOAT(y + size));
      auto brush = (((x & 1) ^ (y & 1)) == 1) ? sBlueBrush : sRedBrush;
      sRenderTarget->FillRectangle(&bounds, brush);
    }
  }

  if (sRenderTarget->EndDraw() == D2DERR_RECREATE_TARGET) {
    ErrorExit("EndDraw() returned D2DERR_RECREATE_TARGET");
  }
  ::EndPaint(hwnd, &sPaintStruct);

}

static HWND sHWND = NULL;

static void CheckWindowSize(HWND hwnd) {
  RECT prc;
  GetClientRect(sParentHWND, &prc);
  RECT crc;
  GetClientRect(hwnd, &crc);
  cp_log("Child resize in parent detected, parent=%d,%d,%d,%d child=%d,%d,%d,%d",
    prc.left, prc.top, prc.right, prc.bottom,
    crc.left, crc.top, crc.right, crc.bottom
  );
  if (!Equals(prc, crc)) {
    SetWindowPos(hwnd, NULL, 0, 0, prc.right - prc.left, prc.bottom - prc.top,
      SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOOWNERZORDER | SWP_NOZORDER);
  }
  InvalidateRect(hwnd, NULL, FALSE);
}

LRESULT CALLBACK ChildWndProc(HWND hwnd, UINT msg, 
    WPARAM wParam, LPARAM lParam)
{
  static bool isPainting = false;
  sHWND = hwnd;
  //cp_log("ChildWndProc msg=%u %lx", msg, msg);
  switch (msg) {

    case WM_IPC_MSG: {
      cp_log("WM_IPC_MSG hwnd=%x %c", hwnd, char(wParam));
      char ch = char(wParam);
      if (ch == '0') {
        PostQuitMessage(0);
        return 0;
      }
      if (ch == '1') {
        OnPaint(hwnd, 0, 0);
        return 0;
      }
      if (ch == '2') {
        CheckWindowSize(hwnd);
      }
      break;
    }

    //case WM_NCCALCSIZE: {
    //  RECT* clientRect =
    //      wParam ? &(reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam))->rgrc[0]
    //              : (reinterpret_cast<RECT*>(lParam));
    //  break;
    //};

    case WM_KEYDOWN: {
      if (wParam == VK_ESCAPE) {
        PostQuitMessage(0);
      }
      break;
    }

    case WM_PAINT: {
      if (isPainting) {
        return 0;
      }
      isPainting = true;
      CheckWindowSize(hwnd);
      OnPaint(hwnd, wParam, lParam);
      isPainting = false;
      return 0;
    }

    case WM_NCDESTROY: {
      PostQuitMessage(0);
      break;
    }

    case WM_DESTROY: {
      PostQuitMessage(0);
      break;
    }
  };
  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void InitD2D(HWND hwnd)
{
  cp_log("Child process trying to initialize D2D");
  HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                                 &sD2DFactory);
  if (FAILED(hr)) {
    ErrorExit("Failed to create D2D factory");
  }

  RECT rc;
  GetClientRect(hwnd, &rc);

  // Create a DC render target.
  D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
      D2D1_RENDER_TARGET_TYPE_DEFAULT,
      D2D1::PixelFormat(
          DXGI_FORMAT_B8G8R8A8_UNORM,
          D2D1_ALPHA_MODE_IGNORE),
      0,
      0,
      D2D1_RENDER_TARGET_USAGE_NONE,
      D2D1_FEATURE_LEVEL_DEFAULT
      );

  hr = sD2DFactory->CreateDCRenderTarget(&props, &sRenderTarget);
  if (FAILED(hr)) {
    ErrorExit("CreateDCRenderTarget failed");
  }

  hr = sRenderTarget->CreateSolidColorBrush(
    D2D1::ColorF(D2D1::ColorF::Blue),
    &sBlueBrush);
  if (FAILED(hr)) {
    ErrorExit("CreateSolidColorBrush (blue) failed");
  }

  // Create a blue brush.
  hr = sRenderTarget->CreateSolidColorBrush(
    D2D1::ColorF(D2D1::ColorF::Red),
    &sRedBrush);
  if (FAILED(hr)) {
    ErrorExit("CreateSolidColorBrush (red) failed");
  }

  cp_log("Child process initialized D2D");
}

void ShutdownD2D() {
  SAFE_RELEASE(sRedBrush);
  SAFE_RELEASE(sBlueBrush);
  SAFE_RELEASE(sRedBrush);
  SAFE_RELEASE(sRenderTarget);
  SAFE_RELEASE(sD2DFactory);
}


int WINAPI ChildWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
  PWSTR pCmdLine, int nCmdShow, HWND parent)
{
  cp_log("Child process started");

  AutoComInit autoCom;

  sParentHWND = parent;

  WNDCLASSW wc;
  wc.style = CS_OWNDC;
  wc.lpfnWndProc = ChildWndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = GetModuleHandle(nullptr);
  wc.hIcon = nullptr;
  wc.hCursor = nullptr;
  wc.hbrBackground = nullptr;
  wc.lpszMenuName = nullptr;
  wc.lpszClassName = L"GlitchCompositorWindow";
  RegisterClassW(&wc);

  RECT rc;
  GetClientRect(parent, &rc);

  HWND hwnd = ::CreateWindowEx(
            WS_EX_NOPARENTNOTIFY, "GlitchCompositorWindow", nullptr,
            WS_CHILDWINDOW | WS_DISABLED | WS_VISIBLE,
            0, 0, rc.right-rc.left, rc.bottom - rc.top, parent,
            0, GetModuleHandle(nullptr), 0);

  InitD2D(hwnd);

  ShowWindow(hwnd, nCmdShow);
  UpdateWindow(hwnd);

  Receiver receiver(GetStdHandle(STD_INPUT_HANDLE), hwnd);

  MSG  msg;    
  while (GetMessage(&msg, NULL, 0, 0)) {
    DispatchMessage(&msg);
  }

  ShutdownD2D();

  return (int) msg.wParam;
}

