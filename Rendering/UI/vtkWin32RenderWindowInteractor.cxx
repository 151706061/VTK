// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <atomic> // for std::atomic
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map> // for std::map

#ifndef WINVER
#define WINVER 0x0601 // for touch support, 0x0601 means target Windows 7 or later
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601 // for touch support, 0x0601 means target Windows 7 or later
#endif

#include "vtkHardwareWindow.h"
#include "vtkRenderWindow.h"
#include "vtkStringArray.h"
#include "vtkWin32RenderWindowInteractor.h"
#include "vtkWindows.h"
#include "vtksys/Encoding.hxx"

#include <shellapi.h> // for drag and drop
#include <winuser.h>  // for touch support

// Mouse wheel support
// In an ideal world we would just have to include <zmouse.h>, but it is not
// always available with all compilers/headers
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif // WM_MOUSEWHEEL
#ifndef GET_WHEEL_DELTA_WPARAM
#define GET_WHEEL_DELTA_WPARAM(wparam) ((short)HIWORD(wparam))
#endif // GET_WHEEL_DELTA_WPARAM

// MSVC does the right thing without the forward declaration when it
// sees it in the friend decl in vtkWin32RenderWindowInteractor, but
// GCC needs to see the declaration beforehand. It has to do with the
// CALLBACK attribute.
VTK_ABI_NAMESPACE_BEGIN
VTKRENDERINGUI_EXPORT LRESULT CALLBACK vtkHandleMessage(HWND, UINT, WPARAM, LPARAM);
VTKRENDERINGUI_EXPORT LRESULT CALLBACK vtkHandleMessage2(
  HWND, UINT, WPARAM, LPARAM, class vtkWin32RenderWindowInteractor*);

VTK_ABI_NAMESPACE_END
#include "vtkActor.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"

#ifdef VTK_USE_TDX
#include "vtkTDxWinDevice.h"
#endif

// we hard define the touch structures we use since we cannot take them
// from the header without requiring windows 7 (stupid stupid stupid!)
// so we define them and then do a runtime checks and function pointers
// to avoid a link requirement on Windows 7
#define MOUSEEVENTF_FROMTOUCH 0xFF515700
#define WM_TOUCH 0x0240
#define TOUCH_COORD_TO_PIXEL(l) ((l) / 100)

typedef TOUCHINPUT* PTOUCHINPUT;

// #define HTOUCHINPUT ULONG
#define TOUCHEVENTF_MOVE 0x0001
#define TOUCHEVENTF_DOWN 0x0002
#define TOUCHEVENTF_UP 0x0004
typedef bool(WINAPI* RegisterTouchWindowType)(HWND, ULONG);
typedef bool(WINAPI* GetTouchInputInfoType)(HTOUCHINPUT, UINT, PTOUCHINPUT, int);
typedef bool(WINAPI* CloseTouchInputHandleType)(HTOUCHINPUT);

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkWin32RenderWindowInteractor);

void (*vtkWin32RenderWindowInteractor::ClassExitMethod)(void*) = (void (*)(void*)) nullptr;
void* vtkWin32RenderWindowInteractor::ClassExitMethodArg = nullptr;
void (*vtkWin32RenderWindowInteractor::ClassExitMethodArgDelete)(void*) = (void (*)(void*)) nullptr;

namespace
{
//-------------------------------------------------------------
// Virtual Key Code to Unix KeySym Conversion
//-------------------------------------------------------------

// clang-format off
// this unicode code to keysym table is meant to provide keysym similar to X Window System's XLookupString(),
// for Basic Latin and Latin1 unicode blocks.
// Generated from xlib/X11/keysymdef.h
// Duplicated in Rendering/OpenGL2/vtkCocoaGLView.mm
static const char* UnicodeToKeySymTable[256] = {
  // Basic Latin
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  "space", "exclam", "quotedbl", "numbersign", "dollar", "percent", "ampersand", "apostrophe", "parenleft", "parenright", "asterisk", "plus", "comma", "minus", "period", "slash",
  "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "colon", "semicolon", "less", "equal", "greater", "question", "at",
  "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P",
  "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "bracketleft", "backslash", "bracketright", "asciicircum", "underscore", "grave",
  "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p",
  "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "braceleft", "bar", "braceright", "asciitilde", nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,

  // Latin1
  "nobreakspace", "exclamdown", "cent", "sterling", "currency", "yen", "brokenbar", "section", "diaeresis", "copyright", "ordfeminine", "guillemotleft", "notsign", "hyphen", "registered", "macron",
  "degree", "plusminus", "twosuperior", "threesuperior", "acute", "mu", "paragraph", "periodcentered", "cedilla", "onesuperior", "masculine", "guillemotright", "onequarter", "onehalf", "threequarters", "questiondown",
  "Agrave", "Aacute", "Acircumflex", "Atilde", "Adiaeresis", "Aring", "AE", "Ccedilla", "Egrave", "Eacute", "Ecircumflex", "Ediaeresis", "Igrave", "Iacute", "Icircumflex", "Idiaeresis",
  "ETH", "Ntilde", "Ograve", "Oacute", "Ocircumflex", "Otilde", "Odiaeresis", "multiply", "Ooblique", "Ugrave", "Uacute", "Ucircumflex", "Udiaeresis", "Yacute", "THORN", "ssharp",
  "agrave", "aacute", "acircumflex", "atilde", "adiaeresis", "aring", "ae", "ccedilla", "egrave", "eacute", "ecircumflex", "ediaeresis", "igrave", "iacute", "icircumflex", "idiaeresis",
  "eth", "ntilde", "ograve", "oacute", "ocircumflex", "otilde", "odiaeresis", "division", "oslash", "ugrave", "uacute", "ucircumflex", "udiaeresis", "yacute", "thorn", "ydiaeresis"
};

// This table is meant to provide keysym similar to X Window System's XLookupString() from Windows VKeys (Winuser.h)
// that are not mapped in the unicode table above.
static const char* VKeyCodeToKeySymTable[256] = {
  nullptr, nullptr, nullptr, "Cancel", nullptr, nullptr, nullptr, nullptr, "BackSpace", "Tab", nullptr, nullptr, "Clear", "Return", nullptr, nullptr,
  "Shift_L", "Control_L", "Alt_L", "Pause", "Caps_Lock", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, "Escape", nullptr, nullptr, nullptr, nullptr,
  "space", "Prior", "Next", "End", "Home", "Left", "Up", "Right", "Down", "Select", nullptr, "Execute", "Snapshot", "Insert", "Delete", "Help",
  "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o",
  "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "Win_L", "Win_R", "App", nullptr, nullptr,
  "KP_0", "KP_1", "KP_2", "KP_3", "KP_4", "KP_5", "KP_6", "KP_7", "KP_8", "KP_9", "asterisk", "plus", "bar", "minus", "period", "slash",
  "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "F13", "F14", "F15", "F16",
  "F17", "F18", "F19", "F20", "F21", "F22", "F23", "F24", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  "Num_Lock", "Scroll_Lock", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
};
// clang-format on

void RecoverModifiersStatus(int& ctrl, int& shift, int& alt)
{
  ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
  shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
  alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
}

void RecoverKeyEventInformation(
  UINT vCode, UINT nFlags, int& ctrl, int& shift, int& alt, char& keyCode, const char*& keySym)
{
  RecoverModifiersStatus(ctrl, shift, alt);
  // WORD is unsigned short
  WORD nChar = 0;
  WORD nCharWithoutMod = 0;
  {
#ifndef _WIN32_WCE
    BYTE keyState[256];
    GetKeyboardState(keyState);
    if (ToAscii(vCode, nFlags & 0xff, keyState, &nChar, 0) == 0)
    {
      nChar = 0;
    }
    nCharWithoutMod = nChar;
    if (ctrl != 0 || alt != 0)
    {
      // When using modifiers, recover a keyCode without modifiers
      // except Shift in order to unsure behavior consistency with
      // other OSes.
      keyState[VK_CONTROL] = 0;
      keyState[VK_MENU] = 0;
      if (ToAscii(vCode, nFlags & 0xff, keyState, &nCharWithoutMod, 0) == 0)
      {
        nCharWithoutMod = 0;
      }
    }
#endif
  }

  // keyCode is the modified one except when it is 0
  // in that case, fallback on the version without modifiers
  keyCode = static_cast<char>(nChar);
  if (keyCode == 0)
  {
    keyCode = static_cast<char>(nCharWithoutMod);
  }

  // Try to recover any valid keySym
  if (nChar < 256)
  {
    keySym = UnicodeToKeySymTable[nChar];
  }
  if (keySym == nullptr && nCharWithoutMod < 256)
  {
    keySym = UnicodeToKeySymTable[nCharWithoutMod];
  }
  if (keySym == nullptr && vCode < 256)
  {
    keySym = VKeyCodeToKeySymTable[vCode];
  }
  if (keySym == nullptr)
  {
    keySym = "None";
  }
}

}

class vtkWin32RenderWindowInteractor::vtkInternals
{
public:
  // this structure is used in the callback internally
  // instances have to live after calling InternalCreateTimer so we store them
  // in a map until InternalDestroyTimer is called
  struct TimerContext
  {
    HWND WindowId;
    int TimerId;
    int TimerIdForPost;
    HANDLE PlatformId;
    std::atomic<bool> Posted{ false };
  };
  std::map<int, std::unique_ptr<TimerContext>> TimerContextMap;
  bool IsRunning = false;

  static void OnTimerFired(PVOID lpParameter, BOOLEAN)
  {
    auto* timerContext = static_cast<TimerContext*>(lpParameter);
    // Do not post another message for the same timer if already posted
    // to avoid flooding the message queue
    if (!timerContext->Posted.exchange(true))
    {
      PostMessage(timerContext->WindowId, WM_TIMER, timerContext->TimerId, 0);
    }
  }

  static void OnTimerMessageReceived(TimerContext* timerContext) { timerContext->Posted = false; }
};

//------------------------------------------------------------------------------
// Construct object so that light follows camera motion.
vtkWin32RenderWindowInteractor::vtkWin32RenderWindowInteractor()
  : Internals(new vtkInternals())
{
  this->WindowId = 0;
  this->InstallMessageProc = 1;
  this->MouseInWindow = 0;
  this->StartedMessageLoop = 0;

#ifdef VTK_USE_TDX
  this->Device = vtkTDxWinDevice::New();
#endif
}

//------------------------------------------------------------------------------
vtkWin32RenderWindowInteractor::~vtkWin32RenderWindowInteractor()
{
  vtkRenderWindow* tmp;

  // we need to release any hold we have on a windows event loop
  if (this->WindowId && this->Enabled && this->InstallMessageProc)
  {
    vtkRenderWindow* ren = this->RenderWindow;
    tmp = (vtkRenderWindow*)(vtkGetWindowLong(this->WindowId, sizeof(vtkLONG)));
    // watch for odd conditions
    if ((tmp != ren) && (ren != nullptr))
    {
      // OK someone else has a hold on our event handler
      // so lets have them handle this stuff
      // well send a USER message to the other
      // event handler so that it can properly
      // call this event handler if required
      CallWindowProc(this->OldProc, this->WindowId, WM_USER + 14, 28, (intptr_t)this->OldProc);
    }
    else
    {
      vtkSetWindowLong(this->WindowId, vtkGWL_WNDPROC, (intptr_t)this->OldProc);
    }
    this->Enabled = 0;
  }
#ifdef VTK_USE_TDX
  this->Device->Delete();
#endif
}

//------------------------------------------------------------------------------
void vtkWin32RenderWindowInteractor::ProcessEvents()
{
  // No need to do anything if this is a 'mapped' interactor
  if (!this->Enabled || !this->InstallMessageProc)
  {
    return;
  }

  /**
   * NOTE:
   * Defer processing the timer in next iteration because
   * a WM_LBUTTONUP or other INPUT event may be wired up to
   * a callback that destroys a timer. By exiting this loop,
   * we give a chance to that input event's callback to run
   * in the next invocation of `ProcessEvents()`.
   * In this example, the left button down event is wired to a callback
   * that creates a repeating timer for 10ms. This is a real use case in
   * vtkInteractorStyle.cxx in the vtkInteractorStyle::StartState and
   * vtkInteractorStyle::StopState methods.
   * This diagram illustrates a problem with a single PeekMessage loop
   * when a WM_TIMER was posted (via `PostMessage`) rather than using
   * `SetTimer`.
   *
   * Time  |      Main thread            |  Timer thread
   *       |Handle WM_LBUTTONDOWN        |
   *   0ms | ->CreateRepeatingTimer(10ms)|
   *       |                             |
   *       |                             |
   *       |                             |
   *       |                             |
   *   10ms|                             | PostMessage(WM_TIMER, ...)
   *       |Handle WM_TIMER              |
   *   0ms |  ->OnTimer()                |
   *       |                             |
   *       |                             |
   *       |                             |
   *       |                             |
   *   10ms|                             | PostMessage(WM_TIMER, ...)
   *       |Handle WM_TIMER              |
   *   0ms |  ->OnTimer()                |
   *       |                             |
   *       |                             |
   *       |                             |
   *       |                             |
   *   10ms|Handle WM_TIMER              | PostMessage(WM_TIMER, ...)
   *   0ms |  ->OnTimer()                |
   *       |                             |
   *   4ms |New WM_LBUTTONUP generated   |
   *       |  Cannot handle it because   |
   *       |  we are seeing lots of      |
   *       |  WM_TIMER                   |
   *       |                             |
   *   10ms|Handle WM_TIMER              | PostMessage(WM_TIMER, ...)
   *   0ms |  ->OnTimer()                |
   *
   * For this reason, we split the PeekMessage loop into four sub-loops.
   * 1. In the first pass, only process INPUT and PAINT events with the PM_QS_INPUT | PM_QS_PAINT
   * mask.
   * 2. In the second pass, process all messages that were posted via `PostMessage()`. This includes
   * WM_TIMER. When this loop sees a WM_TIMER, it breaks immediately in order to give equal chance
   * to other events that may be connected to callbacks which destroy timers.
   * Also handle INPUT event to avoid potential hangs when window goes out of focus.
   * 3. In the final pass, process all messages that were sent via `SendMessage()`. This interactor
   * does not use `SendMessage`, however, it is provided in case a custom app uses it.
   */
  auto& internals = (*this->Internals);
  MSG msg;
  // Process input events first
  while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE | PM_QS_INPUT | PM_QS_PAINT))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  // Process posted messages (which includes timer) and input
  while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE | PM_QS_POSTMESSAGE | PM_QS_INPUT))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
    if (msg.message == WM_QUIT)
    {
      internals.IsRunning = false;
    }
    if (msg.message == WM_TIMER)
    {
      // defer to next execution of `vtkWin32RenderWindowInteractor::ProcessEvents()`
      break;
    }
  }
  // Process sent messages
  while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE | PM_QS_SENDMESSAGE))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}

//------------------------------------------------------------------------------
void vtkWin32RenderWindowInteractor::StartEventLoop()
{
  // No need to do anything if this is a 'mapped' interactor
  if (!this->Enabled || !this->InstallMessageProc)
  {
    return;
  }

  this->StartedMessageLoop = 1;
  do
  {
    this->Internals->IsRunning = true;
    this->ProcessEvents();
  } while (this->Internals->IsRunning);
}

//------------------------------------------------------------------------------
// Begin processing keyboard strokes.
void vtkWin32RenderWindowInteractor::Initialize()
{
  vtkRenderWindow* ren;
  int* size;

  // make sure we have a RenderWindow and camera
  if (!this->RenderWindow)
  {
    vtkErrorMacro(<< "No renderer defined!");
    return;
  }
  if (this->Initialized)
  {
    return;
  }
  this->Initialized = 1;
  // get the info we need from the RenderingWindow
  ren = this->RenderWindow;
  ren->Start();
  ren->End();
  size = ren->GetSize();
  ren->GetPosition();

  this->WindowId = (HWND)(ren->GetGenericWindowId());
  if (this->HardwareWindow)
  {
    this->WindowId = (HWND)(this->HardwareWindow->GetGenericWindowId());
    size = this->HardwareWindow->GetSize();
    vtkSetWindowLong(this->WindowId, sizeof(vtkLONG), (intptr_t)ren);
  }
  else
  {
    this->WindowId = (HWND)(ren->GetGenericWindowId());
  }

  this->Enable();
  this->Size[0] = size[0];
  this->Size[1] = size[1];
}

//------------------------------------------------------------------------------
void vtkWin32RenderWindowInteractor::Enable()
{
  vtkRenderWindow* ren;
  vtkRenderWindow* tmp;
  if (this->Enabled)
  {
    return;
  }
  if (this->InstallMessageProc)
  {
    // add our callback
    ren = this->RenderWindow;
    this->OldProc = (WNDPROC)vtkGetWindowLong(this->WindowId, vtkGWL_WNDPROC);
    tmp = (vtkRenderWindow*)vtkGetWindowLong(this->WindowId, sizeof(vtkLONG));
    // watch for odd conditions
    if (tmp != ren)
    {
      // OK someone else has a hold on our event handler
      // so lets have them handle this stuff
      // well send a USER message to the other
      // event handler so that it can properly
      // call this event handler if required
      CallWindowProc(this->OldProc, this->WindowId, WM_USER + 12, 24, (intptr_t)vtkHandleMessage);
    }
    else
    {
      vtkSetWindowLong(this->WindowId, vtkGWL_WNDPROC, (intptr_t)vtkHandleMessage);
    }

    // Check for windows multitouch support at runtime
    RegisterTouchWindowType RTW = (RegisterTouchWindowType)GetProcAddress(
      GetModuleHandle(TEXT("user32")), "RegisterTouchWindow");
    if (RTW != nullptr)
    {
      RTW(this->WindowId, 0);
    }

#ifdef VTK_USE_TDX
    if (this->UseTDx)
    {
      this->Device->SetInteractor(this);
      this->Device->Initialize();
      this->Device->StartListening();
    }
#endif

    // enable drag and drop events
    DragAcceptFiles(this->WindowId, TRUE);

    // in case the size of the window has changed while we were away
    int* size;
    size = ren->GetSize();
    this->Size[0] = size[0];
    this->Size[1] = size[1];
  }
  this->Enabled = 1;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkWin32RenderWindowInteractor::Disable()
{
  vtkRenderWindow* tmp;
  if (!this->Enabled)
  {
    return;
  }

  if (this->InstallMessageProc && this->Enabled && this->WindowId)
  {
    // we need to release any hold we have on a windows event loop
    vtkRenderWindow* ren = this->RenderWindow;
    tmp = (vtkRenderWindow*)vtkGetWindowLong(this->WindowId, sizeof(vtkLONG));
    // watch for odd conditions
    if ((tmp != ren) && (ren != nullptr))
    {
      // OK someone else has a hold on our event handler
      // so lets have them handle this stuff
      // well send a USER message to the other
      // event handler so that it can properly
      // call this event handler if required
      CallWindowProc(this->OldProc, this->WindowId, WM_USER + 14, 28, (intptr_t)this->OldProc);
    }
    else
    {
      vtkSetWindowLong(this->WindowId, vtkGWL_WNDPROC, (intptr_t)this->OldProc);
    }
#ifdef VTK_USE_TDX
    if (this->Device->GetInitialized())
    {
      this->Device->Close();
    }
#endif
  }
  this->Enabled = 0;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkWin32RenderWindowInteractor::TerminateApp(void)
{
  if (this->Done)
  {
    return;
  }

  this->Done = true;

  // Only post a quit message if Start was called...
  //
  if (this->StartedMessageLoop)
  {
    PostQuitMessage(0);
  }
}

//------------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::InternalCreateTimer(
  int timerId, int timerType, unsigned long duration)
{
  auto& internals = (*this->Internals);

  std::unique_ptr<vtkInternals::TimerContext> timerContext(new vtkInternals::TimerContext);
  timerContext->WindowId = this->WindowId;
  timerContext->TimerId = timerId;

  if (timerType == vtkRenderWindowInteractor::RepeatingTimer)
  {
    CreateTimerQueueTimer(&timerContext->PlatformId, nullptr, vtkInternals::OnTimerFired,
      timerContext.get(), duration, duration, WT_EXECUTEDEFAULT);
  }
  else
  {
    CreateTimerQueueTimer(&timerContext->PlatformId, nullptr, vtkInternals::OnTimerFired,
      timerContext.get(), duration, 0, WT_EXECUTEONLYONCE);
  }

  internals.TimerContextMap[timerId] = std::move(timerContext);

  return timerId;
}

//------------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::InternalDestroyTimer(int platformTimerId)
{
  auto& internals = (*this->Internals);
  auto it = internals.TimerContextMap.find(platformTimerId);
  if (it != internals.TimerContextMap.end())
  {
    BOOL ret = DeleteTimerQueueTimer(nullptr, it->second->PlatformId, nullptr);
    internals.TimerContextMap.erase(it);
    return ret;
  }
  return 0;
}

//-------------------------------------------------------------
// Event loop handlers
//-------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnMouseMove(HWND hWnd, UINT nFlags, int X, int Y)
{
  if (!this->Enabled)
  {
    return 0;
  }

  // touch events handled by WM_TOUCH
  if ((GetMessageExtraInfo() & MOUSEEVENTF_FROMTOUCH) == MOUSEEVENTF_FROMTOUCH)
  {
    return 0;
  }

  this->SetEventInformationFlipY(X, Y, nFlags & MK_CONTROL, nFlags & MK_SHIFT);
  this->SetAltKey((GetKeyState(VK_MENU) & 0x8000) != 0);
  if (!this->MouseInWindow && (X >= 0 && X < this->Size[0] && Y >= 0 && Y < this->Size[1]))
  {
    this->InvokeEvent(vtkCommand::EnterEvent, nullptr);
    this->MouseInWindow = 1;
    // request WM_MOUSELEAVE generation
    TRACKMOUSEEVENT tme;
    tme.cbSize = sizeof(TRACKMOUSEEVENT);
    tme.dwFlags = TME_LEAVE;
    tme.hwndTrack = hWnd;
    TrackMouseEvent(&tme);
  }

  return this->InvokeEvent(vtkCommand::MouseMoveEvent, nullptr);
}

//------------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnNCMouseMove(HWND, UINT nFlags, int X, int Y)
{
  if (!this->Enabled || !this->MouseInWindow)
  {
    return 0;
  }

  const int* pos = this->RenderWindow->GetPosition();
  this->SetEventInformationFlipY(X - pos[0], Y - pos[1], nFlags & MK_CONTROL, nFlags & MK_SHIFT);
  this->SetAltKey((GetKeyState(VK_MENU) & 0x8000) != 0);
  const int ret = this->InvokeEvent(vtkCommand::LeaveEvent, nullptr);
  this->MouseInWindow = 0;
  return ret;
}

//------------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnMouseWheelForward(HWND, UINT nFlags, int X, int Y)
{
  if (!this->Enabled)
  {
    return 0;
  }
  this->SetEventInformationFlipY(X, Y, nFlags & MK_CONTROL, nFlags & MK_SHIFT);
  this->SetAltKey((GetKeyState(VK_MENU) & 0x8000) != 0);
  return this->InvokeEvent(vtkCommand::MouseWheelForwardEvent, nullptr);
}

//------------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnMouseWheelBackward(HWND, UINT nFlags, int X, int Y)
{
  if (!this->Enabled)
  {
    return 0;
  }
  this->SetEventInformationFlipY(X, Y, nFlags & MK_CONTROL, nFlags & MK_SHIFT);
  this->SetAltKey((GetKeyState(VK_MENU) & 0x8000) != 0);
  return this->InvokeEvent(vtkCommand::MouseWheelBackwardEvent, nullptr);
}

//------------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnLButtonDown(HWND wnd, UINT nFlags, int X, int Y, int repeat)
{
  if (!this->Enabled)
  {
    return 0;
  }

  // touch events handled by WM_TOUCH
  if ((GetMessageExtraInfo() & MOUSEEVENTF_FROMTOUCH) == MOUSEEVENTF_FROMTOUCH)
  {
    return 0;
  }

  SetFocus(wnd);
  SetCapture(wnd);
  this->SetEventInformationFlipY(X, Y, nFlags & MK_CONTROL, nFlags & MK_SHIFT, 0, repeat);
  this->SetAltKey((GetKeyState(VK_MENU) & 0x8000) != 0);
  return this->InvokeEvent(vtkCommand::LeftButtonPressEvent, nullptr);
}

//------------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnLButtonUp(HWND, UINT nFlags, int X, int Y)
{
  if (!this->Enabled)
  {
    return 0;
  }
  // touch events handled by WM_TOUCH
  if ((GetMessageExtraInfo() & MOUSEEVENTF_FROMTOUCH) == MOUSEEVENTF_FROMTOUCH)
  {
    return 0;
  }
  this->SetEventInformationFlipY(X, Y, nFlags & MK_CONTROL, nFlags & MK_SHIFT);
  this->SetAltKey((GetKeyState(VK_MENU) & 0x8000) != 0);
  const int ret = this->InvokeEvent(vtkCommand::LeftButtonReleaseEvent, nullptr);
  ReleaseCapture();
  return ret;
}

//------------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnMButtonDown(HWND wnd, UINT nFlags, int X, int Y, int repeat)
{
  if (!this->Enabled)
  {
    return 0;
  }
  SetFocus(wnd);
  SetCapture(wnd);
  this->SetEventInformationFlipY(X, Y, nFlags & MK_CONTROL, nFlags & MK_SHIFT, 0, repeat);
  this->SetAltKey((GetKeyState(VK_MENU) & 0x8000) != 0);
  return this->InvokeEvent(vtkCommand::MiddleButtonPressEvent, nullptr);
}

//------------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnMButtonUp(HWND, UINT nFlags, int X, int Y)
{
  if (!this->Enabled)
  {
    return 0;
  }
  this->SetEventInformationFlipY(X, Y, nFlags & MK_CONTROL, nFlags & MK_SHIFT);
  this->SetAltKey((GetKeyState(VK_MENU) & 0x8000) != 0);
  const int ret = this->InvokeEvent(vtkCommand::MiddleButtonReleaseEvent, nullptr);
  ReleaseCapture();
  return ret;
}

//------------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnRButtonDown(HWND wnd, UINT nFlags, int X, int Y, int repeat)
{
  if (!this->Enabled)
  {
    return 0;
  }
  SetFocus(wnd);
  SetCapture(wnd);
  this->SetEventInformationFlipY(X, Y, nFlags & MK_CONTROL, nFlags & MK_SHIFT, 0, repeat);
  this->SetAltKey((GetKeyState(VK_MENU) & 0x8000) != 0);
  return this->InvokeEvent(vtkCommand::RightButtonPressEvent, nullptr);
}

//------------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnRButtonUp(HWND, UINT nFlags, int X, int Y)
{
  if (!this->Enabled)
  {
    return 0;
  }
  this->SetEventInformationFlipY(X, Y, nFlags & MK_CONTROL, nFlags & MK_SHIFT);
  this->SetAltKey((GetKeyState(VK_MENU) & 0x8000) != 0);
  const int ret = this->InvokeEvent(vtkCommand::RightButtonReleaseEvent, nullptr);
  ReleaseCapture();
  return ret;
}

//------------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnSize(HWND, UINT, int X, int Y)
{
  this->UpdateSize(X, Y);
  if (this->Enabled)
  {
    return this->InvokeEvent(vtkCommand::ConfigureEvent, nullptr);
  }
  return 0;
}

//------------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnTimer(HWND, UINT timerId)
{
  auto& internals = (*this->Internals);
  if (!this->Enabled)
  {
    return 0;
  }
  int tid = static_cast<int>(timerId);
  auto it = internals.TimerContextMap.find(timerId);
  if (it != internals.TimerContextMap.end())
  {
    vtkInternals::OnTimerMessageReceived(it->second.get());
  }

  return this->InvokeEvent(vtkCommand::TimerEvent, &tid);
}

//------------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnKeyDown(HWND, UINT vCode, UINT nRepCnt, UINT nFlags)
{
  if (!this->Enabled)
  {
    return 0;
  }
  int ctrl, shift, alt;
  char keyCode;
  const char* keySym;
  ::RecoverKeyEventInformation(vCode, nFlags, ctrl, shift, alt, keyCode, keySym);
  this->SetKeyEventInformation(ctrl, shift, keyCode, nRepCnt, keySym);
  this->SetAltKey(alt);
  return this->InvokeEvent(vtkCommand::KeyPressEvent, nullptr);
}

//------------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnKeyUp(HWND, UINT vCode, UINT nRepCnt, UINT nFlags)
{
  if (!this->Enabled)
  {
    return 0;
  }
  int ctrl, shift, alt;
  char keyCode;
  const char* keySym;
  ::RecoverKeyEventInformation(vCode, nFlags, ctrl, shift, alt, keyCode, keySym);
  this->SetKeyEventInformation(ctrl, shift, keyCode, nRepCnt, keySym);
  this->SetAltKey(alt);
  return this->InvokeEvent(vtkCommand::KeyReleaseEvent, nullptr);
}

//------------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnChar(HWND, UINT nChar, UINT nRepCnt, UINT)
{
  if (!this->Enabled)
  {
    return 0;
  }
  int ctrl, shift, alt;
  ::RecoverModifiersStatus(ctrl, shift, alt);
  this->SetKeyEventInformation(ctrl, shift, nChar, nRepCnt);
  this->SetAltKey(alt);
  return this->InvokeEvent(vtkCommand::CharEvent, nullptr);
}

//------------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnFocus(HWND, UINT)
{
  if (!this->Enabled)
  {
    return 0;
  }

#ifdef VTK_USE_TDX
  if (this->Device->GetInitialized() && !this->Device->GetIsListening())
  {
    this->Device->StartListening();
    return 1;
  }
#endif

  return 0;
}

//------------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnKillFocus(HWND, UINT)
{
  if (!this->Enabled)
  {
    return 0;
  }
#ifdef VTK_USE_TDX
  if (this->Device->GetInitialized() && this->Device->GetIsListening())
  {
    this->Device->StopListening();
    return 1;
  }
#endif

  return 0;
}

//------------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnTouch(HWND hWnd, UINT wParam, UINT lParam)
{
  if (!this->Enabled)
  {
    return 0;
  }

  int ret(0);
  UINT cInputs = LOWORD(wParam);
  PTOUCHINPUT pInputs = new TOUCHINPUT[cInputs];
  if (pInputs)
  {
    int ctrl, shift, alt;
    ::RecoverModifiersStatus(ctrl, shift, alt);
    this->SetAltKey(alt);
    GetTouchInputInfoType GTII =
      (GetTouchInputInfoType)GetProcAddress(GetModuleHandle(TEXT("user32")), "GetTouchInputInfo");
    if (GTII((HTOUCHINPUT)lParam, cInputs, pInputs, sizeof(TOUCHINPUT)))
    {
      POINT ptInput;
      for (UINT i = 0; i < cInputs; i++)
      {
        TOUCHINPUT ti = pInputs[i];
        int index = this->GetPointerIndexForContact(ti.dwID);
        if (ti.dwID != 0 && index < VTKI_MAX_POINTERS)
        {
          // Do something with your touch input handle
          ptInput.x = TOUCH_COORD_TO_PIXEL(ti.x);
          ptInput.y = TOUCH_COORD_TO_PIXEL(ti.y);
          ScreenToClient(hWnd, &ptInput);
          this->SetEventInformationFlipY(ptInput.x, ptInput.y, ctrl, shift, 0, 0, 0, index);
        }
      }
      bool didUpOrDown = false;
      for (UINT i = 0; i < cInputs; i++)
      {
        TOUCHINPUT ti = pInputs[i];
        int index = this->GetPointerIndexForContact(ti.dwID);
        if (ti.dwID != 0 && index < VTKI_MAX_POINTERS)
        {
          if (ti.dwFlags & TOUCHEVENTF_UP)
          {
            this->SetPointerIndex(index);
            didUpOrDown = true;
            this->InvokeEvent(vtkCommand::LeftButtonReleaseEvent, nullptr);
            this->ClearPointerIndex(index);
          }
          if (ti.dwFlags & TOUCHEVENTF_DOWN)
          {
            this->SetPointerIndex(index);
            didUpOrDown = true;
            this->InvokeEvent(vtkCommand::LeftButtonPressEvent, nullptr);
          }
          this->SetPointerIndex(index);
        }
      }
      if (!didUpOrDown)
      {
        ret = this->InvokeEvent(vtkCommand::MouseMoveEvent, nullptr);
      }
      else
      {
        ret = 1;
      }
    }
    CloseTouchInputHandleType CTIH = (CloseTouchInputHandleType)GetProcAddress(
      GetModuleHandle(TEXT("user32")), "CloseTouchInputHandle");
    CTIH((HTOUCHINPUT)lParam);
    delete[] pInputs;
  }

  return ret;
}

//------------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnDropFiles(HWND, WPARAM wParam)
{
  if (!this->Enabled)
  {
    return 0;
  }

  HDROP hdrop = reinterpret_cast<HDROP>(wParam);

  POINT pt;
  if (DragQueryPoint(hdrop, &pt))
  {
    double location[2];
    location[0] = static_cast<double>(pt.x);
    location[1] = static_cast<double>(this->Size[1] - pt.y - 1); // flip Y

    int ctrl, shift, alt;
    ::RecoverModifiersStatus(ctrl, shift, alt);
    this->SetEventInformationFlipY(location[0], location[1], ctrl, shift);
    this->SetAltKey(alt);
    this->InvokeEvent(vtkCommand::UpdateDropLocationEvent, location);
  }

  UINT cFiles = DragQueryFileW(hdrop, 0xFFFFFFFF, nullptr, 0);

  int ret = 0;
  if (cFiles > 0)
  {
    vtkNew<vtkStringArray> filePaths;
    filePaths->Allocate(cFiles);

    for (UINT i = 0; i < cFiles; i++)
    {
      wchar_t file[MAX_PATH];
      UINT cch = DragQueryFileW(hdrop, i, file, MAX_PATH);
      if (cch > 0 && cch < MAX_PATH)
      {
        filePaths->InsertNextValue(vtksys::Encoding::ToNarrow(file));
        ret = 1;
      }
    }
    this->InvokeEvent(vtkCommand::DropFilesEvent, filePaths);
  }

  return ret;
}

//------------------------------------------------------------------------------
// This is only called when InstallMessageProc is true
LRESULT CALLBACK vtkHandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  LRESULT res = 0;
  vtkRenderWindow* ren;
  vtkWin32RenderWindowInteractor* me = 0;

  ren = (vtkRenderWindow*)vtkGetWindowLong(hWnd, sizeof(vtkLONG));

  if (ren)
  {
    me = (vtkWin32RenderWindowInteractor*)ren->GetInteractor();
  }

  if (me && me->GetReferenceCount() > 0)
  {
    me->Register(me);
    res = vtkHandleMessage2(hWnd, uMsg, wParam, lParam, me);
    me->UnRegister(me);
  }

  return res;
}

#ifndef MAKEPOINTS
#define MAKEPOINTS(l) (*((POINTS FAR*)&(l)))
#endif

//------------------------------------------------------------------------------
LRESULT CALLBACK vtkHandleMessage2(
  HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, vtkWin32RenderWindowInteractor* me)
{
  if ((uMsg == WM_USER + 13) && (wParam == 26))
  {
    // someone is telling us to set our OldProc
    me->OldProc = (WNDPROC)lParam;
    return 1;
  }

  int handled(0);

  switch (uMsg)
  {
    case WM_PAINT:
    {
      const LRESULT ret(CallWindowProc(me->OldProc, hWnd, uMsg, wParam, lParam));
      me->InvokeEvent(vtkCommand::RenderEvent, nullptr);
      return ret;
    }

    case WM_SIZE:
      handled = me->OnSize(hWnd, wParam, LOWORD(lParam), HIWORD(lParam));
      break;

    case WM_LBUTTONDBLCLK:
      handled = me->OnLButtonDown(hWnd, wParam, MAKEPOINTS(lParam).x, MAKEPOINTS(lParam).y, 1);
      break;

    case WM_LBUTTONDOWN:
      handled = me->OnLButtonDown(hWnd, wParam, MAKEPOINTS(lParam).x, MAKEPOINTS(lParam).y, 0);
      break;

    case WM_LBUTTONUP:
      handled = me->OnLButtonUp(hWnd, wParam, MAKEPOINTS(lParam).x, MAKEPOINTS(lParam).y);
      break;

    case WM_MBUTTONDBLCLK:
      handled = me->OnMButtonDown(hWnd, wParam, MAKEPOINTS(lParam).x, MAKEPOINTS(lParam).y, 1);
      break;

    case WM_MBUTTONDOWN:
      handled = me->OnMButtonDown(hWnd, wParam, MAKEPOINTS(lParam).x, MAKEPOINTS(lParam).y, 0);
      break;

    case WM_MBUTTONUP:
      handled = me->OnMButtonUp(hWnd, wParam, MAKEPOINTS(lParam).x, MAKEPOINTS(lParam).y);
      break;

    case WM_RBUTTONDBLCLK:
      handled = me->OnRButtonDown(hWnd, wParam, MAKEPOINTS(lParam).x, MAKEPOINTS(lParam).y, 1);
      break;

    case WM_RBUTTONDOWN:
      handled = me->OnRButtonDown(hWnd, wParam, MAKEPOINTS(lParam).x, MAKEPOINTS(lParam).y, 0);
      break;

    case WM_RBUTTONUP:
      handled = me->OnRButtonUp(hWnd, wParam, MAKEPOINTS(lParam).x, MAKEPOINTS(lParam).y);
      break;

    case WM_MOUSELEAVE:
      me->InvokeEvent(vtkCommand::LeaveEvent, nullptr);
      me->MouseInWindow = 0;
      break;

    case WM_MOUSEMOVE:
      handled = me->OnMouseMove(hWnd, wParam, MAKEPOINTS(lParam).x, MAKEPOINTS(lParam).y);
      break;

    case WM_MOUSEWHEEL:
    {
      POINT pt;
      pt.x = MAKEPOINTS(lParam).x;
      pt.y = MAKEPOINTS(lParam).y;
      ::ScreenToClient(hWnd, &pt);
      if (GET_WHEEL_DELTA_WPARAM(wParam) > 0)
        handled = me->OnMouseWheelForward(hWnd, wParam, pt.x, pt.y);
      else
        handled = me->OnMouseWheelBackward(hWnd, wParam, pt.x, pt.y);
    }
    break;

#ifdef WM_MCVMOUSEMOVE
    case WM_NCMOUSEMOVE:
      handled = me->OnNCMouseMove(hWnd, wParam, MAKEPOINTS(lParam).x, MAKEPOINTS(lParam).y);
      break;
#endif

    case WM_CLOSE:
      me->ExitCallback();
      break;

    case WM_CHAR:
      handled = me->OnChar(hWnd, wParam, LOWORD(lParam), HIWORD(lParam));
      break;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
      handled = me->OnKeyDown(hWnd, wParam, LOWORD(lParam), HIWORD(lParam));
      break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
      handled = me->OnKeyUp(hWnd, wParam, LOWORD(lParam), HIWORD(lParam));
      break;

    case WM_TIMER:
      handled = me->OnTimer(hWnd, wParam);
      break;

    case WM_ACTIVATE:
      if (wParam == WA_INACTIVE)
      {
        handled = me->OnKillFocus(hWnd, wParam);
      }
      else
      {
        handled = me->OnFocus(hWnd, wParam);
      }
      break;

    case WM_SETFOCUS:
      // occurs when SetFocus() is called on the current window
      handled = me->OnFocus(hWnd, wParam);
      break;

    case WM_KILLFOCUS:
      // occurs when the focus was on the current window and SetFocus() is
      // called on another window.
      handled = me->OnKillFocus(hWnd, wParam);
      break;

    case WM_TOUCH:
      handled = me->OnTouch(hWnd, wParam, lParam);
      break;

    case WM_DROPFILES:
      handled = me->OnDropFiles(hWnd, wParam);
      break;

    default:
      break;
  };

  if (0 == handled)
  {
    return CallWindowProc(me->OldProc, hWnd, uMsg, wParam, lParam);
  }

  return 0;
}

//------------------------------------------------------------------------------
// Specify the default function to be called when an interactor needs to exit.
// This callback is overridden by an instance ExitMethod that is defined.
void vtkWin32RenderWindowInteractor::SetClassExitMethod(void (*f)(void*), void* arg)
{
  if (f != vtkWin32RenderWindowInteractor::ClassExitMethod ||
    arg != vtkWin32RenderWindowInteractor::ClassExitMethodArg)
  {
    // delete the current arg if there is a delete method
    if ((vtkWin32RenderWindowInteractor::ClassExitMethodArg) &&
      (vtkWin32RenderWindowInteractor::ClassExitMethodArgDelete))
    {
      (*vtkWin32RenderWindowInteractor::ClassExitMethodArgDelete)(
        vtkWin32RenderWindowInteractor::ClassExitMethodArg);
    }
    vtkWin32RenderWindowInteractor::ClassExitMethod = f;
    vtkWin32RenderWindowInteractor::ClassExitMethodArg = arg;

    // no call to this->Modified() since this is a class member function
  }
}

//------------------------------------------------------------------------------
// Set the arg delete method.  This is used to free user memory.
void vtkWin32RenderWindowInteractor::SetClassExitMethodArgDelete(void (*f)(void*))
{
  if (f != vtkWin32RenderWindowInteractor::ClassExitMethodArgDelete)
  {
    vtkWin32RenderWindowInteractor::ClassExitMethodArgDelete = f;

    // no call to this->Modified() since this is a class member function
  }
}

//------------------------------------------------------------------------------
void vtkWin32RenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "InstallMessageProc: " << this->InstallMessageProc << endl;
  os << indent << "StartedMessageLoop: " << this->StartedMessageLoop << endl;
}

//------------------------------------------------------------------------------
void vtkWin32RenderWindowInteractor::ExitCallback()
{
  if (this->HasObserver(vtkCommand::ExitEvent))
  {
    this->InvokeEvent(vtkCommand::ExitEvent, nullptr);
  }
  else if (this->ClassExitMethod)
  {
    (*this->ClassExitMethod)(this->ClassExitMethodArg);
  }

  this->TerminateApp();
}
VTK_ABI_NAMESPACE_END
