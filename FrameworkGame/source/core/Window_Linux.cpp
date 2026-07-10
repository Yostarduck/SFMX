#include "core/Window_Linux.h"

#if SFMX_PLATFORM_LINUX

#include <cstdint>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

// X11 leaks a `Window` typedef (and macros like None / Success) into the global
// namespace; the engine's own Window base is sfmx::Window, so every X11 window
// is spelled ::Window here to stay unambiguous.

namespace sfmx
{

namespace
{

// RAII wrapper around a private X11 connection. SFML keeps its own display
// connection internal, so native operations open a short-lived one; window
// manager messages and property reads are server-side, so a separate
// connection sees the same window state.
class XDisplayGuard
{
 public:
  XDisplayGuard()
    : m_display(XOpenDisplay(nullptr)) {}

  ~XDisplayGuard() {
    if (nullptr != m_display) {
      XCloseDisplay(m_display);
    }
  }

  XDisplayGuard(const XDisplayGuard&) = delete;
  XDisplayGuard& operator=(const XDisplayGuard&) = delete;

  Display*
  get() const { return m_display; }

  explicit operator bool() const { return nullptr != m_display; }

 private:
  Display* m_display;
};

// Add or remove a pair of _NET_WM_STATE atoms on a window via the EWMH client
// message the window manager listens for on the root window.
void
sendNetWmState(Display* display,
               ::Window window,
               const bool add,
               const Atom first,
               const Atom second)
{
  const Atom wmState = XInternAtom(display, "_NET_WM_STATE", False);

  XEvent event {};
  event.type                 = ClientMessage;
  event.xclient.window       = window;
  event.xclient.message_type = wmState;
  event.xclient.format       = 32;
  event.xclient.data.l[0]    = add ? 1 : 0;  // _NET_WM_STATE_ADD / _REMOVE
  event.xclient.data.l[1]    = static_cast<long>(first);
  event.xclient.data.l[2]    = static_cast<long>(second);
  event.xclient.data.l[3]    = 1;            // source indication: application
  event.xclient.data.l[4]    = 0;

  XSendEvent(display,
             DefaultRootWindow(display),
             False,
             SubstructureRedirectMask | SubstructureNotifyMask,
             &event);
  XFlush(display);
}

} // namespace

NODISCARD
void*
LinuxWindow::getNativeHandle() const
{
  return isCreated()
    ? reinterpret_cast<void*>(static_cast<std::uintptr_t>(nativeHandle()))
    : nullptr;
}
// -- Window state ------------------------------------------------------
// Maximize / minimize / restore have no SFML API, so they are driven through
// the window manager (EWMH) on the native X11 window.
void
LinuxWindow::maximize()
{
  if (!isCreated()) {
    return;
  }

  XDisplayGuard display;
  if (!display) {
    return;
  }

  Display* const d   = display.get();
  const ::Window win = static_cast<::Window>(nativeHandle());
  const Atom maxH = XInternAtom(d, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
  const Atom maxV = XInternAtom(d, "_NET_WM_STATE_MAXIMIZED_VERT", False);

  sendNetWmState(d, win, true, maxH, maxV);
}

void
LinuxWindow::minimize()
{
  if (!isCreated()) {
    return;
  }

  XDisplayGuard display;
  if (!display) {
    return;
  }

  Display* const d   = display.get();
  const ::Window win = static_cast<::Window>(nativeHandle());

  XIconifyWindow(d, win, DefaultScreen(d));
  XFlush(d);
}

void
LinuxWindow::restore()
{
  if (!isCreated()) {
    return;
  }

  XDisplayGuard display;
  if (!display) {
    return;
  }

  Display* const d   = display.get();
  const ::Window win = static_cast<::Window>(nativeHandle());
  const Atom maxH = XInternAtom(d, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
  const Atom maxV = XInternAtom(d, "_NET_WM_STATE_MAXIMIZED_VERT", False);

  // Drop the maximized state and re-map so an iconified window is shown again.
  sendNetWmState(d, win, false, maxH, maxV);
  XMapWindow(d, win);
  XFlush(d);
}
 

NODISCARD
bool
LinuxWindow::isMaximized() const
{
  if (!isCreated()) {
    return false;
  }

  XDisplayGuard display;
  if (!display) {
    return false;
  }

  Display* const d   = display.get();
  const ::Window win = static_cast<::Window>(nativeHandle());
  const Atom wmState = XInternAtom(d, "_NET_WM_STATE", False);
  const Atom maxH = XInternAtom(d, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
  const Atom maxV = XInternAtom(d, "_NET_WM_STATE_MAXIMIZED_VERT", False);

  Atom          actualType   = None;
  int           actualFormat = 0;
  unsigned long itemCount     = 0;
  unsigned long bytesAfter    = 0;
  unsigned char* data          = nullptr;

  bool horizontal = false;
  bool vertical   = false;

  if (Success == XGetWindowProperty(d, win, wmState, 0, 1024, False, XA_ATOM,
                                    &actualType, &actualFormat, &itemCount,
                                    &bytesAfter, &data)
      && nullptr != data) {
    const Atom* atoms = reinterpret_cast<const Atom*>(data);
    for (unsigned long i = 0; i < itemCount; ++i) {
      if (atoms[i] == maxH) { horizontal = true; }
      if (atoms[i] == maxV) { vertical   = true; }
    }
    XFree(data);
  }

  return horizontal && vertical;
}

NODISCARD
bool
LinuxWindow::isMinimized() const
{
  if (!isCreated()) {
    return false;
  }

  XDisplayGuard display;
  if (!display) {
    return false;
  }

  Display* const d   = display.get();
  const ::Window win = static_cast<::Window>(nativeHandle());
  const Atom wmStateAtom = XInternAtom(d, "WM_STATE", True);
  if (None == wmStateAtom) {
    return false;
  }

  Atom          actualType   = None;
  int           actualFormat = 0;
  unsigned long itemCount     = 0;
  unsigned long bytesAfter    = 0;
  unsigned char* data          = nullptr;

  bool iconic = false;

  if (Success == XGetWindowProperty(d, win, wmStateAtom, 0, 2, False,
                                    wmStateAtom, &actualType, &actualFormat,
                                    &itemCount, &bytesAfter, &data)
      && nullptr != data) {
    if (itemCount > 0) {
      // WM_STATE stores the state as the first 32-bit value (IconicState == 3).
      const long state = *reinterpret_cast<const long*>(data);
      iconic = (IconicState == state);
    }
    XFree(data);
  }

  return iconic;
}

void
LinuxWindow::applyStyle()
{
  if (!isCreated()) {
    return;
  }

  XDisplayGuard display;
  if (!display) {
    return;
  }

  Display* const d   = display.get();
  const ::Window win = static_cast<::Window>(nativeHandle());

  // Decorations: toggle the title bar / border through Motif window hints.
  struct MotifWmHints
  {
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
    long          inputMode;
    unsigned long status;
  };
  constexpr unsigned long kMwmHintsDecorations = (1UL << 1);

  const Atom motifHints = XInternAtom(d, "_MOTIF_WM_HINTS", False);
  MotifWmHints hints {};
  hints.flags       = kMwmHintsDecorations;
  hints.decorations = isDecorated() ? 1UL : 0UL;

  XChangeProperty(d, win, motifHints, motifHints, 32, PropModeReplace,
                  reinterpret_cast<unsigned char*>(&hints),
                  sizeof(hints) / sizeof(long));

  // Resizable: constrain the WM size hints to the current size when locked.
  XSizeHints* sizeHints = XAllocSizeHints();
  if (nullptr != sizeHints) {
    long supplied = 0;
    XGetWMNormalHints(d, win, sizeHints, &supplied);

    if (isResizable()) {
      sizeHints->flags &= ~(PMinSize | PMaxSize);
    } else {
      uint32_t width  = 0;
      uint32_t height = 0;
      getSize(width, height);

      sizeHints->flags |= (PMinSize | PMaxSize);
      sizeHints->min_width  = sizeHints->max_width  = static_cast<int>(width);
      sizeHints->min_height = sizeHints->max_height = static_cast<int>(height);
    }

    XSetWMNormalHints(d, win, sizeHints);
    XFree(sizeHints);
  }

  XFlush(d);
}

sf::WindowHandle
LinuxWindow::nativeHandle() const
{
  return getRenderWindow().getNativeHandle();
}

} // namespace sfmx

#endif  // SFMX_PLATFORM_LINUX