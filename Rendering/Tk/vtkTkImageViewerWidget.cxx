// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTkImageViewerWidget.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTkInternals.h"
#include "vtkVersionMacros.h"

#ifdef _MSC_VER
#pragma warning(disable : 4273)
#else
#if defined(VTK_USE_COCOA)
#include "vtkCocoaRenderWindow.h"
#include "vtkCocoaTkUtilities.h"
#ifndef MAC_OSX_TK
#define MAC_OSX_TK 1
#endif
#include "tkInt.h"
#else
#include "vtkXOpenGLRenderWindow.h"
#endif
#endif

#include <algorithm>
#include <cstdlib>

#if (TCL_MAJOR_VERSION >= 9)
#define VTK_TCL_CONST const
#elif ((TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4))
#define VTK_TCL_CONST CONST84
#else
#define VTK_TCL_CONST
#endif
#ifndef offsetof
#define offsetof(type, field) ((size_t)((char*)&((type*)0)->field))
#endif

#define VTK_ALL_EVENTS_MASK                                                                        \
  KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | EnterWindowMask |          \
    LeaveWindowMask | PointerMotionMask | ExposureMask | VisibilityChangeMask | FocusChangeMask |  \
    PropertyChangeMask | ColormapChangeMask

// These are the options that can be set when the widget is created
// or with the command configure.  The only new one is "-rw" which allows
// the uses to set their own ImageViewer window.
static Tk_ConfigSpec vtkTkImageViewerWidgetConfigSpecs[] = {
  { TK_CONFIG_PIXELS, "-height", "height", "Height", "400",
    offsetof(struct vtkTkImageViewerWidget, Height), 0, nullptr },

  { TK_CONFIG_PIXELS, "-width", "width", "Width", "400",
    offsetof(struct vtkTkImageViewerWidget, Width), 0, nullptr },

  { TK_CONFIG_STRING, "-iv", "iv", "IV", "", offsetof(struct vtkTkImageViewerWidget, IV), 0,
    nullptr },

  { TK_CONFIG_END, nullptr, nullptr, nullptr, nullptr, 0, 0, nullptr }
};

// Forward prototypes
extern "C"
{
  void vtkTkImageViewerWidget_EventProc(ClientData clientData, XEvent* eventPtr);
}

static int vtkTkImageViewerWidget_MakeImageViewer(struct vtkTkImageViewerWidget* self);
extern int vtkImageViewerCommand(ClientData cd, Tcl_Interp* interp, int argc, char* argv[]);

//------------------------------------------------------------------------------
// It's possible to change with this function or in a script some
// options like width, height or the ImageViewer widget.
#if (TCL_MAJOR_VERSION >= 9)
int vtkTkImageViewerWidget_Configure(Tcl_Interp* interp, struct vtkTkImageViewerWidget* self,
  Tcl_Size objc, Tcl_Obj* const* objv, int flags)
#else
int vtkTkImageViewerWidget_Configure(Tcl_Interp* interp, struct vtkTkImageViewerWidget* self,
  int argc, VTK_TCL_CONST char* argv[], int flags)
#endif
{
  // Let Tk handle generic configure options.
#if (TCL_MAJOR_VERSION >= 9)
  if (Tk_ConfigureWidget(interp, self->TkWin, vtkTkImageViewerWidgetConfigSpecs, objc, objv,
        (void*)self, flags) == TCL_ERROR)
#else
  if (Tk_ConfigureWidget(interp, self->TkWin, vtkTkImageViewerWidgetConfigSpecs, argc, argv,
        (char*)self, flags) == TCL_ERROR)
#endif
  {
    return (TCL_ERROR);
  }

  // Get the new width and height of the widget
  Tk_GeometryRequest(self->TkWin, self->Width, self->Height);

  // Make sure the ImageViewer window has been set.  If not, create one.
  if (vtkTkImageViewerWidget_MakeImageViewer(self) == TCL_ERROR)
  {
    return TCL_ERROR;
  }

  return TCL_OK;
}

//------------------------------------------------------------------------------
// This function is called when the ImageViewer widget name is
// evaluated in a Tcl script.  It will compare string parameters
// to choose the appropriate method to invoke.
extern "C"
{
  int vtkTkImageViewerWidget_Widget(
    ClientData clientData, Tcl_Interp* interp, int argc, VTK_TCL_CONST char* argv[])
  {
    struct vtkTkImageViewerWidget* self = (struct vtkTkImageViewerWidget*)clientData;
    int result = TCL_OK;

    // Check to see if the command has enough arguments.
    if (argc < 2)
    {
      Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0], " ?options?\"", nullptr);
      return TCL_ERROR;
    }

    // Make sure the widget is not deleted during this function
#if (TCL_MAJOR_VERSION >= 9)
    Tcl_Preserve((ClientData)self);
#else
    Tk_Preserve((ClientData)self);
#endif

    // Handle render call to the widget
    if (strncmp(argv[1], "render", std::max<size_t>(1, strlen(argv[1]))) == 0 ||
      strncmp(argv[1], "Render", std::max<size_t>(1, strlen(argv[1]))) == 0)
    {
      // make sure we have a window
      if (self->ImageViewer == nullptr)
      {
        vtkTkImageViewerWidget_MakeImageViewer(self);
      }
      self->ImageViewer->Render();
    }
    // Handle configure method
    else if (!strncmp(argv[1], "configure", std::max<size_t>(1, strlen(argv[1]))))
    {
      if (argc == 2)
      {
        /* Return list of all configuration parameters */
        result = Tk_ConfigureInfo(
          interp, self->TkWin, vtkTkImageViewerWidgetConfigSpecs, (char*)self, nullptr, 0);
      }
      else if (argc == 3)
      {
        /* Return a specific configuration parameter */
        result = Tk_ConfigureInfo(
          interp, self->TkWin, vtkTkImageViewerWidgetConfigSpecs, (char*)self, argv[2], 0);
      }
      else
      {
        /* Execute a configuration change */
#if (TCL_MAJOR_VERSION >= 9)
        // Convert string arguments to Tcl_Obj for TCL 9.0
        Tcl_Obj** objv_config = (Tcl_Obj**)ckalloc((argc - 2) * sizeof(Tcl_Obj*));
        for (int i = 0; i < argc - 2; i++)
        {
          objv_config[i] = Tcl_NewStringObj(argv[i + 2], -1);
          Tcl_IncrRefCount(objv_config[i]);
        }
        result = vtkTkImageViewerWidget_Configure(
          interp, self, argc - 2, objv_config, TK_CONFIG_ARGV_ONLY);

        // Clean up the Tcl_Obj array
        for (int i = 0; i < argc - 2; i++)
        {
          Tcl_DecrRefCount(objv_config[i]);
        }
        ckfree((char*)objv_config);
#else
        result =
          vtkTkImageViewerWidget_Configure(interp, self, argc - 2, argv + 2, TK_CONFIG_ARGV_ONLY);
#endif
      }
    }
    else if (!strcmp(argv[1], "GetImageViewer"))
    { // Get ImageViewerWindow is my own method
      // Create a ImageViewerWidget if one has not been set yet.
      result = vtkTkImageViewerWidget_MakeImageViewer(self);
      if (result != TCL_ERROR)
      {
        // Return the name (Make Tcl copy the string)
        Tcl_SetResult(interp, self->IV, TCL_VOLATILE);
      }
    }
    else
    {
      // Unknown method name.
      Tcl_AppendResult(interp, "vtkTkImageViewerWidget: Unknown option: ", argv[1], "\n",
        "Try: configure or GetImageViewer\n", nullptr);
      result = TCL_ERROR;
    }

    // Unlock the object so it can be deleted.
#if (TCL_MAJOR_VERSION >= 9)
    Tcl_Release((ClientData)self);
#else
    Tk_Release((ClientData)self);
#endif
    return result;
  }
}

//------------------------------------------------------------------------------
// vtkTkImageViewerWidget_Cmd
// Called when vtkTkImageViewerWidget is executed
// - creation of a vtkTkImageViewerWidget widget.
//     * Creates a new window
//     * Creates an 'vtkTkImageViewerWidget' data structure
//     * Creates an event handler for this window
//     * Creates a command that handles this object
//     * Configures this vtkTkImageViewerWidget for the given arguments
extern "C"
{
  int vtkTkImageViewerWidget_Cmd(
    ClientData clientData, Tcl_Interp* interp, int argc, VTK_TCL_CONST char** argv)
  {
    VTK_TCL_CONST char* name;
    Tk_Window main = (Tk_Window)clientData;
    Tk_Window tkwin;
    struct vtkTkImageViewerWidget* self;

    // Make sure we have an instance name.
    if (argc <= 1)
    {
      Tcl_ResetResult(interp);
      Tcl_AppendResult(interp, "wrong # args: should be \"pathName read filename\"", nullptr);
      return (TCL_ERROR);
    }

    // Create the window.
    name = argv[1];
    // Possibly X dependent
    tkwin = Tk_CreateWindowFromPath(interp, main, name, nullptr);
    if (tkwin == nullptr)
    {
      return TCL_ERROR;
    }

    // Tcl needs this for setting options and matching event bindings.
    Tk_SetClass(tkwin, (char*)"vtkTkImageViewerWidget");

    // Create vtkTkImageViewerWidget data structure
    self = (struct vtkTkImageViewerWidget*)ckalloc(sizeof(struct vtkTkImageViewerWidget));

    self->TkWin = tkwin;
    self->Interp = interp;
    self->Width = 0;
    self->Height = 0;
    self->ImageViewer = nullptr;
    self->IV = nullptr;

    // ...
    // Create command event handler
    Tcl_CreateCommand(interp, Tk_PathName(tkwin), vtkTkImageViewerWidget_Widget, (ClientData)self,
      (void (*)(ClientData)) nullptr);
    Tk_CreateEventHandler(tkwin, ExposureMask | StructureNotifyMask,
      vtkTkImageViewerWidget_EventProc, (ClientData)self);

    // Configure vtkTkImageViewerWidget widget
#if (TCL_MAJOR_VERSION >= 9)
    // Convert string arguments to Tcl_Obj for TCL 9.0
    Tcl_Obj** objv_init = (Tcl_Obj**)ckalloc((argc - 2) * sizeof(Tcl_Obj*));
    for (int i = 0; i < argc - 2; i++)
    {
      objv_init[i] = Tcl_NewStringObj(argv[i + 2], -1);
      Tcl_IncrRefCount(objv_init[i]);
    }

    if (vtkTkImageViewerWidget_Configure(interp, self, argc - 2, objv_init, 0) == TCL_ERROR)
    {
      // Clean up before error return
      for (int i = 0; i < argc - 2; i++)
      {
        Tcl_DecrRefCount(objv_init[i]);
      }
      ckfree((char*)objv_init);

      Tk_DestroyWindow(tkwin);
      Tcl_DeleteCommand(interp, (char*)"vtkTkImageViewerWidget");
      return TCL_ERROR;
    }

    // Clean up the Tcl_Obj array
    for (int i = 0; i < argc - 2; i++)
    {
      Tcl_DecrRefCount(objv_init[i]);
    }
    ckfree((char*)objv_init);
#else
    if (vtkTkImageViewerWidget_Configure(interp, self, argc - 2, argv + 2, 0) == TCL_ERROR)
    {
      Tk_DestroyWindow(tkwin);
      Tcl_DeleteCommand(interp, (char*)"vtkTkImageViewerWidget");
      // Don't free it, if we do a crash occurs later...
      // free(self);
      return TCL_ERROR;
    }
#endif

    Tcl_AppendResult(interp, Tk_PathName(tkwin), nullptr);
    return TCL_OK;
  }
}

extern "C"
{
#if (TCL_MAJOR_VERSION >= 9)
  void vtkTkImageViewerWidget_Destroy(void* memPtr)
#else
  void vtkTkImageViewerWidget_Destroy(char* memPtr)
#endif
  {
    struct vtkTkImageViewerWidget* self = (struct vtkTkImageViewerWidget*)memPtr;

    if (self->ImageViewer)
    {
      if (self->ImageViewer->GetRenderWindow()->GetInteractor() &&
        self->ImageViewer->GetRenderWindow()->GetInteractor()->GetRenderWindow() ==
          self->ImageViewer->GetRenderWindow())
      {
        self->ImageViewer->GetRenderWindow()->GetInteractor()->SetRenderWindow(nullptr);
      }
      if (self->ImageViewer->GetRenderWindow()->GetReferenceCount() > 1)
      {
        vtkGenericWarningMacro("A TkImageViewerWidget is being destroyed before it associated "
                               "vtkImageViewer is destroyed. This is very bad and usually due to "
                               "the order in which objects are being destroyed. Always destroy the "
                               "vtkImageViewer before destroying the user interface components.");
        return;
      }
      // Squash the ImageViewer's WindowID
      self->ImageViewer->SetWindowId(nullptr);
      self->ImageViewer->UnRegister(nullptr);
      self->ImageViewer = nullptr;
      ckfree(self->IV);
    }
    ckfree(memPtr);
  }
}

//------------------------------------------------------------------------------
// This gets called to handle vtkTkImageViewerWidget wind configuration events
// Possibly X dependent
extern "C"
{
  void vtkTkImageViewerWidget_EventProc(ClientData clientData, XEvent* eventPtr)
  {
    struct vtkTkImageViewerWidget* self = (struct vtkTkImageViewerWidget*)clientData;

    switch (eventPtr->type)
    {
      case Expose:
        // let the user handle Expose events
        break;
      case ConfigureNotify:
        // if (Tk_IsMapped(self->TkWin))
        {
          self->Width = Tk_Width(self->TkWin);
          self->Height = Tk_Height(self->TkWin);
          // Tk_GeometryRequest(self->TkWin,self->Width,self->Height);
          if (self->ImageViewer)
          {
            int x = Tk_X(self->TkWin);
            int y = Tk_Y(self->TkWin);
#if defined(VTK_USE_COCOA)
            // Do not call SetSize or SetPosition until we're mapped.
            if (Tk_IsMapped(self->TkWin))
            {
              // On Cocoa, compute coordinates relative to toplevel
              for (TkWindow* curPtr = ((TkWindow*)self->TkWin)->parentPtr;
                   (nullptr != curPtr) && !(curPtr->flags & TK_TOP_LEVEL);
                   curPtr = curPtr->parentPtr)
              {
                x += Tk_X(curPtr);
                y += Tk_Y(curPtr);
              }
              self->ImageViewer->SetPosition(x, y);
              self->ImageViewer->SetSize(self->Width, self->Height);
            }
#else
            self->ImageViewer->SetPosition(x, y);
            self->ImageViewer->SetSize(self->Width, self->Height);
#endif
          }

          // vtkTkImageViewerWidget_PostRedisplay(self);
        }
        break;
      case MapNotify:
#if defined(VTK_USE_COCOA)
      {
        // On Cocoa, compute coordinates relative to the toplevel
        int x = Tk_X(self->TkWin);
        int y = Tk_Y(self->TkWin);
        for (TkWindow* curPtr = ((TkWindow*)self->TkWin)->parentPtr;
             (nullptr != curPtr) && !(curPtr->flags & TK_TOP_LEVEL); curPtr = curPtr->parentPtr)
        {
          x += Tk_X(curPtr);
          y += Tk_Y(curPtr);
        }
        self->ImageViewer->SetPosition(x, y);
        self->ImageViewer->SetSize(self->Width, self->Height);
      }
#endif
      break;
#if defined(VTK_USE_COCOA)
      case UnmapNotify:
        break;
#endif
      case DestroyNotify:
#ifdef _WIN32
        if (self->ImageViewer->GetRenderWindow()->GetGenericWindowId())
        {
          vtkSetWindowLong((HWND)self->ImageViewer->GetRenderWindow()->GetGenericWindowId(),
            vtkGWL_USERDATA, (vtkLONG)((TkWindow*)self->TkWin)->window);
          vtkSetWindowLong((HWND)self->ImageViewer->GetRenderWindow()->GetGenericWindowId(),
            vtkGWL_WNDPROC, (vtkLONG)TkWinChildProc);
        }
#endif
        Tcl_EventuallyFree((ClientData)self, vtkTkImageViewerWidget_Destroy);
        break;
      default:
        // nothing
        ;
    }
  }
}

//------------------------------------------------------------------------------
// vtkTkImageViewerWidget_Init
// Called upon system startup to create vtkTkImageViewerWidget command.
extern "C"
{
  VTK_EXPORT int Vtktkimageviewerwidget_Init(Tcl_Interp* interp);
}

#define VTKTK_TO_STRING(x) VTKTK_TO_STRING0(x)
#define VTKTK_TO_STRING0(x) VTKTK_TO_STRING1(x)
#define VTKTK_TO_STRING1(x) #x
#define VTKTK_VERSION VTKTK_TO_STRING(VTK_MAJOR_VERSION) "." VTKTK_TO_STRING(VTK_MINOR_VERSION)

int Vtktkimageviewerwidget_Init(Tcl_Interp* interp)
{
  // This widget requires Tk to function.
  Tcl_PkgRequire(interp, "Tk", TK_VERSION, 0);
  if (Tcl_PkgPresent(interp, "Tk", TK_VERSION, 0))
  {
    // Register the commands for this package.
    Tcl_CreateCommand(
      interp, "vtkTkImageViewerWidget", vtkTkImageViewerWidget_Cmd, Tk_MainWindow(interp), nullptr);

    // Report that the package is provided.
    return Tcl_PkgProvide(interp, "Vtktkimageviewerwidget", VTKTK_VERSION);
  }
  else
  {
    // Tk is not available.
    return TCL_ERROR;
  }
}

// Here is the windows specific code for creating the window
// The Xwindows version follows after this
#ifdef _WIN32

namespace
{
LRESULT APIENTRY vtkTkImageViewerWidgetProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  LRESULT rval;
  struct vtkTkImageViewerWidget* self =
    (struct vtkTkImageViewerWidget*)vtkGetWindowLong(hWnd, vtkGWL_USERDATA);

  if (!self)
  {
    return 0;
  }

  // forward message to Tk handler
  vtkSetWindowLong(hWnd, vtkGWL_USERDATA, (vtkLONG)((TkWindow*)self->TkWin)->window);
  if (((TkWindow*)self->TkWin)->parentPtr)
  {
    vtkSetWindowLong(hWnd, vtkGWL_WNDPROC, (vtkLONG)TkWinChildProc);
    rval = TkWinChildProc(hWnd, message, wParam, lParam);
  }
  else
  {
//
// TkWinTopLevelProc has been deprecated in Tcl/Tk8.0.  Not sure how
// well this will actually work in 8.0.
//
#if (TK_MAJOR_VERSION < 8)
    vtkSetWindowLong(hWnd, vtkGWL_WNDPROC, (vtkLONG)TkWinTopLevelProc);
    rval = TkWinTopLevelProc(hWnd, message, wParam, lParam);
#else
    if (message == WM_WINDOWPOSCHANGED)
    {
      XEvent event;
      WINDOWPOS* pos = (WINDOWPOS*)lParam;
      TkWindow* winPtr = (TkWindow*)Tk_HWNDToWindow(pos->hwnd);

      if (winPtr == nullptr)
      {
        return 0;
      }

      /*
       * Update the shape of the contained window.
       */
      if (!(pos->flags & SWP_NOSIZE))
      {
        winPtr->changes.width = pos->cx;
        winPtr->changes.height = pos->cy;
      }
      if (!(pos->flags & SWP_NOMOVE))
      {
        winPtr->changes.x = pos->x;
        winPtr->changes.y = pos->y;
      }

      /*
       *  Generate a ConfigureNotify event.
       */
      event.type = ConfigureNotify;
      event.xconfigure.serial = winPtr->display->request;
      event.xconfigure.send_event = False;
      event.xconfigure.display = winPtr->display;
      event.xconfigure.event = winPtr->window;
      event.xconfigure.window = winPtr->window;
      event.xconfigure.border_width = winPtr->changes.border_width;
      event.xconfigure.override_redirect = winPtr->atts.override_redirect;
      event.xconfigure.x = winPtr->changes.x;
      event.xconfigure.y = winPtr->changes.y;
      event.xconfigure.width = winPtr->changes.width;
      event.xconfigure.height = winPtr->changes.height;
      event.xconfigure.above = None;
      Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);

      Tcl_ServiceAll();
      return 0;
    }
    vtkSetWindowLong(hWnd, vtkGWL_WNDPROC, (vtkLONG)TkWinChildProc);
    rval = TkWinChildProc(hWnd, message, wParam, lParam);
#endif
  }

  if (message != WM_PAINT)
  {
    if (self->ImageViewer)
    {
      vtkSetWindowLong(hWnd, vtkGWL_USERDATA, (vtkLONG)self->ImageViewer->GetRenderWindow());
      vtkSetWindowLong(hWnd, vtkGWL_WNDPROC, (vtkLONG)self->OldProc);
      CallWindowProc(self->OldProc, hWnd, message, wParam, lParam);
    }
  }

  // now reset to the original config
  vtkSetWindowLong(hWnd, vtkGWL_USERDATA, (vtkLONG)self);
  vtkSetWindowLong(hWnd, vtkGWL_WNDPROC, (vtkLONG)vtkTkImageViewerWidgetProc);
  return rval;
}
}

//------------------------------------------------------------------------------
// Creates a ImageViewer window and forces Tk to use the window.
static int vtkTkImageViewerWidget_MakeImageViewer(struct vtkTkImageViewerWidget* self)
{
  Display* dpy;
  TkWindow* winPtr = (TkWindow*)self->TkWin;
  Tcl_HashEntry* hPtr;
  int new_flag;
  vtkImageViewer* imgViewer;
  TkWinDrawable* twdPtr;
  HWND parentWin;
  vtkRenderWindow* imgWindow;

  if (self->ImageViewer)
  {
    return TCL_OK;
  }

  dpy = Tk_Display(self->TkWin);

  if (winPtr->window != None)
  {
    // XDestroyWindow(dpy, winPtr->window);
  }

  if (self->IV[0] == '\0')
  {
    // Make the ImageViewer window.
    self->ImageViewer = imgViewer = vtkImageViewer::New();
    ckfree(self->IV);
    self->IV = strdup(Tcl_GetStringResult(self->Interp));
    Tcl_ResetResult(self->Interp);
  }
  else
  {
    // is IV an address ? big ole python hack here
    if (self->IV[0] == 'A' && self->IV[1] == 'd' && self->IV[2] == 'd' && self->IV[3] == 'r')
    {
      void* tmp;
      sscanf(self->IV + 5, "%p", &tmp);
      imgViewer = (vtkImageViewer*)tmp;
    }
    else
    {
      imgViewer = nullptr;
    }
    if (imgViewer != self->ImageViewer)
    {
      if (self->ImageViewer != nullptr)
      {
        self->ImageViewer->UnRegister(nullptr);
      }
      self->ImageViewer = (vtkImageViewer*)(imgViewer);
      if (self->ImageViewer != nullptr)
      {
        self->ImageViewer->Register(nullptr);
      }
    }
  }

  // Set the size
  self->ImageViewer->SetSize(self->Width, self->Height);

  // Set the parent correctly
  // Possibly X dependent
  if ((winPtr->parentPtr != nullptr) && !(winPtr->flags & TK_TOP_LEVEL))
  {
    if (winPtr->parentPtr->window == None)
    {
      Tk_MakeWindowExist((Tk_Window)winPtr->parentPtr);
    }

    parentWin = ((TkWinDrawable*)winPtr->parentPtr->window)->window.handle;
    imgViewer->SetParentId(parentWin);
  }

  // Use the same display
  self->ImageViewer->SetDisplayId(dpy);

  /* Make sure Tk knows to switch to the new colormap when the cursor
   * is over this window when running in color index mode.
   */
  // Tk_SetWindowVisual(self->TkWin, imgViewer->GetDesiredVisual(),
  // ImageViewer->GetDesiredDepth(),
  // ImageViewer->GetDesiredColormap());

  self->ImageViewer->Render();
  imgWindow = self->ImageViewer->GetRenderWindow();

#if (TK_MAJOR_VERSION >= 8)
  twdPtr = (TkWinDrawable*)Tk_AttachHWND(self->TkWin, (HWND)imgWindow->GetGenericWindowId());
#else
  twdPtr = (TkWinDrawable*)ckalloc(sizeof(TkWinDrawable));
  twdPtr->type = TWD_WINDOW;
  twdPtr->window.winPtr = winPtr;
  twdPtr->window.handle = (HWND)imgWindow->GetGenericWindowId();
#endif

  self->OldProc = (WNDPROC)vtkGetWindowLong(twdPtr->window.handle, vtkGWL_WNDPROC);
  vtkSetWindowLong(twdPtr->window.handle, vtkGWL_USERDATA, (vtkLONG)self);
  vtkSetWindowLong(twdPtr->window.handle, vtkGWL_WNDPROC, (vtkLONG)vtkTkImageViewerWidgetProc);

  winPtr->window = (Window)twdPtr;

  hPtr = Tcl_CreateHashEntry(&winPtr->dispPtr->winTable, (char*)winPtr->window, &new_flag);
  Tcl_SetHashValue(hPtr, winPtr);

  winPtr->dirtyAtts = 0;
  winPtr->dirtyChanges = 0;
#ifdef TK_USE_INPUT_METHODS
  winPtr->inputContext = nullptr;
#endif // TK_USE_INPUT_METHODS

  if (!(winPtr->flags & TK_TOP_LEVEL))
  {
    /*
     * If this window has a different colormap than its parent, add
     * the window to the WM_COLORMAP_WINDOWS property for its top-level.
     */
    if ((winPtr->parentPtr != nullptr) &&
      (winPtr->atts.colormap != winPtr->parentPtr->atts.colormap))
    {
      TkWmAddToColormapWindows(winPtr);
    }
  }

  /*
   * Issue a ConfigureNotify event if there were deferred configuration
   * changes (but skip it if the window is being deleted;  the
   * ConfigureNotify event could cause problems if we're being called
   * from Tk_DestroyWindow under some conditions).
   */
  if ((winPtr->flags & TK_NEED_CONFIG_NOTIFY) && !(winPtr->flags & TK_ALREADY_DEAD))
  {
    XEvent event;

    winPtr->flags &= ~TK_NEED_CONFIG_NOTIFY;

    event.type = ConfigureNotify;
    event.xconfigure.serial = LastKnownRequestProcessed(winPtr->display);
    event.xconfigure.send_event = False;
    event.xconfigure.display = winPtr->display;
    event.xconfigure.event = winPtr->window;
    event.xconfigure.window = winPtr->window;
    event.xconfigure.x = winPtr->changes.x;
    event.xconfigure.y = winPtr->changes.y;
    event.xconfigure.width = winPtr->changes.width;
    event.xconfigure.height = winPtr->changes.height;
    event.xconfigure.border_width = winPtr->changes.border_width;
    if (winPtr->changes.stack_mode == Above)
    {
      event.xconfigure.above = winPtr->changes.sibling;
    }
    else
    {
      event.xconfigure.above = None;
    }
    event.xconfigure.override_redirect = winPtr->atts.override_redirect;
    Tk_HandleEvent(&event);
  }

  return TCL_OK;
}

// now the Apple version for Cocoa APIs
#else
#if defined(VTK_USE_COCOA)
//------------------------------------------------------------------------------
// Creates a ImageViewer window and forces Tk to use the window.
static int vtkTkImageViewerWidget_MakeImageViewer(struct vtkTkImageViewerWidget* self)
{
  Display* dpy;
  vtkImageViewer* imgViewer = nullptr;

  if (self->ImageViewer)
  {
    return TCL_OK;
  }

  dpy = Tk_Display(self->TkWin);

  if (self->IV[0] == '\0')
  {
    // Make the ImageViewer window.
    self->ImageViewer = imgViewer = vtkImageViewer::New();
    ckfree(self->IV);
    self->IV = strdup(Tcl_GetStringResult(self->Interp));
    Tcl_ResetResult(self->Interp);
  }
  else
  {
    // is IV an address ? big ole python hack here
    if (self->IV[0] == 'A' && self->IV[1] == 'd' && self->IV[2] == 'd' && self->IV[3] == 'r')
    {
      void* tmp;
      sscanf(self->IV + 5, "%p", &tmp);
      imgViewer = reinterpret_cast<vtkImageViewer*>(tmp);
    }
    if (imgViewer != self->ImageViewer)
    {
      if (self->ImageViewer != nullptr)
      {
        self->ImageViewer->UnRegister(nullptr);
      }
      self->ImageViewer = (vtkImageViewer*)(imgViewer);
      if (self->ImageViewer != nullptr)
      {
        self->ImageViewer->Register(nullptr);
      }
    }
  }

  Tk_MakeWindowExist(self->TkWin);
  // set the ParentId to the NSView
  vtkCocoaRenderWindow* imgWindow =
    static_cast<vtkCocoaRenderWindow*>(imgViewer->GetRenderWindow());
  imgWindow->SetParentId(vtkCocoaTkUtilities::GetDrawableView(self->TkWin));
  imgWindow->SetSize(self->Width, self->Height);

  // Set the size
  self->ImageViewer->SetSize(self->Width, self->Height);

  // Process all outstanding events so that Tk is fully updated
  Tcl_ServiceAll();

  self->ImageViewer->Render();
  return TCL_OK;
}

// now the Xwindows version
#else

//------------------------------------------------------------------------------
// Creates a ImageViewer window and forces Tk to use the window.
static int vtkTkImageViewerWidget_MakeImageViewer(struct vtkTkImageViewerWidget* self)
{
  Display* dpy;
  vtkImageViewer* imgViewer = nullptr;
  vtkXOpenGLRenderWindow* imgWindow;

  if (self->ImageViewer)
  {
    return TCL_OK;
  }

  dpy = Tk_Display(self->TkWin);

  if (Tk_WindowId(self->TkWin) != None)
  {
    XDestroyWindow(dpy, Tk_WindowId(self->TkWin));
  }

  if (self->IV[0] == '\0')
  {
    // Make the ImageViewer window.
    self->ImageViewer = imgViewer = vtkImageViewer::New();
    self->IV = strdup(Tcl_GetStringResult(self->Interp));
    Tcl_ResetResult(self->Interp);
  }
  else
  {
    // is IV an address ? big ole python hack here
    if (self->IV[0] == 'A' && self->IV[1] == 'd' && self->IV[2] == 'd' && self->IV[3] == 'r')
    {
      void* tmp;
      sscanf(self->IV + 5, "%p", &tmp);
      imgViewer = (vtkImageViewer*)tmp;
    }
    if (imgViewer != self->ImageViewer)
    {
      if (self->ImageViewer != nullptr)
      {
        self->ImageViewer->UnRegister(nullptr);
      }
      self->ImageViewer = imgViewer;
      if (self->ImageViewer != nullptr)
      {
        self->ImageViewer->Register(nullptr);
      }
    }
  }

  // get the window
  imgWindow = static_cast<vtkXOpenGLRenderWindow*>(imgViewer->GetRenderWindow());
  // If the imageviewer has already created it's window, throw up our hands and quit...
  if (imgWindow->GetWindowId() != (Window) nullptr)
  {
    return TCL_ERROR;
  }

  // Use the same display
  imgWindow->SetDisplayId(dpy);
  // The visual MUST BE SET BEFORE the window is created.
  Tk_SetWindowVisual(self->TkWin, imgWindow->GetDesiredVisual(), imgWindow->GetDesiredDepth(),
    imgWindow->GetDesiredColormap());

  // Make this window exist, then use that information to make the vtkImageViewer in sync
  Tk_MakeWindowExist(self->TkWin);
  imgViewer->SetWindowId((void*)Tk_WindowId(self->TkWin));

  // Set the size
  self->ImageViewer->SetSize(self->Width, self->Height);

  // Set the parent correctly
  // Possibly X dependent
  if ((Tk_Parent(self->TkWin) == nullptr) || (Tk_IsTopLevel(self->TkWin)))
  {
    imgWindow->SetParentId(XRootWindow(Tk_Display(self->TkWin), Tk_ScreenNumber(self->TkWin)));
  }
  else
  {
    imgWindow->SetParentId(Tk_WindowId(Tk_Parent(self->TkWin)));
  }

  self->ImageViewer->Render();
  return TCL_OK;
}
#endif
#endif
