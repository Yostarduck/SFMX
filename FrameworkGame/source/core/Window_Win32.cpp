#include "core/Window_Win32.h"

namespace sfmx
{

namespace
{

// Win32 window style bits for a given configuration, used for the runtime
// resize/decoration toggles that SFML can only express at creation time.
DWORD
windowStyle(const bool resizable, const bool decorated)
{
  if (!decorated) {
    return WS_POPUP;
  }

  DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
  if (resizable) {
    style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
  }
  return style;
}

}

void*
Win32Window::getNativeHandle() const
{
  return isCreated() ? static_cast<void*>(nativeHandle()) : nullptr;
}

#pragma region Window state
// Maximize / minimize / restore have no SFML API, so they go through the
// native window handle.
void
Win32Window::maximize()
{
  if (isCreated()) {
    ShowWindow(nativeHandle(), SW_MAXIMIZE);
  }
}

void
Win32Window::minimize()
{
  if (isCreated()) {
    ShowWindow(nativeHandle(), SW_MINIMIZE);
  }
}

void
Win32Window::restore()
{
  if (isCreated()) {
    ShowWindow(nativeHandle(), SW_RESTORE);
  }
}

bool
Win32Window::isMaximized() const
{
  return isCreated() && IsZoomed(nativeHandle()) != FALSE;
}

bool
Win32Window::isMinimized() const
{
  return isCreated() && IsIconic(nativeHandle()) != FALSE;
}
#pragma endregion

void
Win32Window::applyStyle()
{
  if (!isCreated()) {
    return;
  }

  const DWORD style = windowStyle(isResizable(), isDecorated());
  const HWND  hwnd  = nativeHandle();

  SetWindowLongPtrW(hwnd, GWL_STYLE, static_cast<LONG_PTR>(style));
  SetWindowPos(hwnd,
               nullptr,
               0,
               0,
               0,
               0,
               SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

HWND
Win32Window::nativeHandle() const
{
  return static_cast<HWND>(getRenderWindow().getNativeHandle());
}

}
