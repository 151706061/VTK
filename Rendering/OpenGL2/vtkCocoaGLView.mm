// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#import "vtkCocoaMacOSXSDKCompatibility.h" // Needed to support old SDKs
#import <Cocoa/Cocoa.h>

#import "vtkCocoaGLView.h"
#import "vtkCocoaRenderWindow.h"
#import "vtkCocoaRenderWindowInteractor.h"
#import "vtkCommand.h"
#import "vtkNew.h"
#import "vtkOpenGLState.h"
#import "vtkStringArray.h"

//----------------------------------------------------------------------------
// Private
@interface vtkCocoaGLView ()
@property (readwrite, retain, nonatomic) NSTrackingArea* rolloverTrackingArea;
@end

@implementation vtkCocoaGLView

//----------------------------------------------------------------------------
// Private
- (void)emptyMethod:(id)sender
{
  (void)sender;
}

//----------------------------------------------------------------------------
@synthesize rolloverTrackingArea = _rolloverTrackingArea;

//------------------------------------------------------------------------------
- (void)commonInit
{
  // Force Cocoa into "multi threaded mode" because VTK spawns pthreads.
  // Apple's docs say: "If you intend to use Cocoa calls, you must force
  // Cocoa into its multithreaded mode before detaching any POSIX threads.
  // To do this, simply detach an NSThread and have it promptly exit.
  // This is enough to ensure that the locks needed by the Cocoa
  // frameworks are put in place"
  if ([NSThread isMultiThreaded] == NO)
  {
    [NSThread detachNewThreadSelector:@selector(emptyMethod:) toTarget:self withObject:nil];
  }

  // kUTTypeFileURL is deprecated starting with macOS 12.0 but its replacement,
  // UTTypeFileURL, is in UniformTypeIdentfiers.framework which doesn't exist
  // on older versions of macOS.  Conditionally using it would require conditionally
  // linking to that new framework, which just isn't worth the hassle when both
  // are just syntactic sugar for the string "public.file-url".
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  NSString* supportedDragType = (NSString*)kUTTypeFileURL;
#pragma clang diagnostic pop

  // Register the view for file drops.
  [self registerForDraggedTypes:@[ supportedDragType ]];
}

//----------------------------------------------------------------------------
// Overridden (from NSView).
// designated initializer
- (id)initWithFrame:(NSRect)frameRect
{
  self = [super initWithFrame:frameRect];
  if (self)
  {
    [self commonInit];
  }
  return self;
}

//----------------------------------------------------------------------------
// Overridden (from NSView).
// designated initializer
- (/*nullable*/ id)initWithCoder:(NSCoder*)decoder
{
  self = [super initWithCoder:decoder];
  if (self)
  {
    [self commonInit];
  }
  return self;
}

//----------------------------------------------------------------------------
#if !VTK_OBJC_IS_ARC
- (void)dealloc
{
  [_rolloverTrackingArea release];
  [super dealloc];
}
#endif

//----------------------------------------------------------------------------
- (vtkCocoaRenderWindow*)getVTKRenderWindow
{
  return _myVTKRenderWindow;
}

//----------------------------------------------------------------------------
- (void)setVTKRenderWindow:(vtkCocoaRenderWindow*)theVTKRenderWindow
{
  _myVTKRenderWindow = theVTKRenderWindow;
}

//----------------------------------------------------------------------------
- (vtkCocoaRenderWindowInteractor*)getInteractor
{
  if (_myVTKRenderWindow)
  {
    return (vtkCocoaRenderWindowInteractor*)_myVTKRenderWindow->GetInteractor();
  }
  else
  {
    return nullptr;
  }
}

//----------------------------------------------------------------------------
// Overridden (from NSView).
- (void)drawRect:(NSRect)theRect
{
  (void)theRect;

  if (_myVTKRenderWindow && _myVTKRenderWindow->GetMapped())
  {
    vtkOpenGLState* state = _myVTKRenderWindow->GetState();
    state->ResetGLScissorState();
    vtkOpenGLState::ScopedglScissor ss(state);

    _myVTKRenderWindow->Render();
  }
}

//----------------------------------------------------------------------------
// Overridden (from NSView).
- (void)updateTrackingAreas
{
  // clear out the old tracking area
  NSTrackingArea* trackingArea = [self rolloverTrackingArea];
  if (trackingArea)
  {
    [self removeTrackingArea:trackingArea];
  }

  // create a new tracking area
  NSRect rect = [self visibleRect];
  NSTrackingAreaOptions opts =
    (NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved | NSTrackingActiveAlways);
  trackingArea = [[NSTrackingArea alloc] initWithRect:rect options:opts owner:self userInfo:nil];
  [self addTrackingArea:trackingArea];
  [self setRolloverTrackingArea:trackingArea];
#if !VTK_OBJC_IS_ARC
  [trackingArea release];
#endif

  [super updateTrackingAreas];
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (BOOL)acceptsFirstResponder
{
  return YES;
}

//----------------------------------------------------------------------------
// clang-format off
// this unicode code to keysym table is meant to provide keysym similar to the X Window System's XLookupString(),
// for Basic Latin and Latin1 unicode blocks.
// Generated from xlib/X11/keysymdef.h
// Duplicated in Rendering/UI/vtkWin32RenderWindowInteractor.cxx
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

//----------------------------------------------------------------------------
// This table is meant to provide keysym similar to X Window System's XLookupString()
// from macOS virtual keys that are not mapped in the unicode table above.
// See the kVK_* enums in HIToolbox/Events.h. For example, kVK_Return = 0x24, and so
// the string "Return" appears at index 36 in this list.
static const char* MacKeyCodeToKeySymTable[128] = {
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, "Return",nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  "Tab", nullptr, nullptr, "BackSpace", nullptr, "Escape", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, "period", nullptr, "asterisk", nullptr, "plus", nullptr, "Clear", nullptr, nullptr, nullptr, "slash", "KP_Enter", nullptr, "minus", nullptr,
  nullptr, nullptr, "KP_0", "KP_1", "KP_2", "KP_3", "KP_4", "KP_5", "KP_6", "KP_7", nullptr, "KP_8", "KP_9", nullptr, nullptr, nullptr,
  "F5", "F6", "F7", "F3", "F8", "F9", nullptr, "F11", nullptr, "Snapshot", nullptr, nullptr, nullptr, "F10", nullptr, "F12",
  nullptr, nullptr, "Help", "Home", "Prior", "Delete", "F4", "End", "F2", "Next", "F1", "Left", "Right", "Down", "Up", nullptr };
// clang-format on

//----------------------------------------------------------------------------
// Convert a Cocoa key event into a VTK key event
- (void)invokeVTKKeyEvent:(unsigned long)theEventId cocoaEvent:(NSEvent*)theEvent
{
  vtkCocoaRenderWindowInteractor* interactor = [self getInteractor];
  vtkCocoaRenderWindow* renWin = vtkCocoaRenderWindow::SafeDownCast([self getVTKRenderWindow]);

  if (!interactor || !renWin)
  {
    return;
  }

  // Get the location of the mouse event relative to this NSView's bottom
  // left corner.  Since this is NOT a mouse event, we can not use
  // locationInWindow.  Instead we get the mouse location at this instant,
  // which may not be the exact location of the mouse at the time of the
  // keypress, but should be quite close.  There seems to be no better way.
  // And, yes, vtk does sometimes need the mouse location even for key
  // events, example: pressing 'p' to pick the actor under the mouse.
  // Also note that 'mouseLoc' may have nonsense values if a key is pressed
  // while the mouse in not actually in the vtk view but the view is
  // first responder.
  NSPoint windowLoc = [[self window] mouseLocationOutsideOfEventStream];
  NSPoint viewLoc = [self convertPoint:windowLoc fromView:nil];
  NSPoint backingLoc = [self convertPointToBacking:viewLoc];

  NSUInteger flags = [theEvent modifierFlags];
  int shiftDown = ((flags & NSEventModifierFlagShift) != 0);
  int controlDown = ((flags & (NSEventModifierFlagControl | NSEventModifierFlagCommand)) != 0);
  int altDown = ((flags & NSEventModifierFlagOption) != 0);

  unsigned char keyCode = '\0';
  unsigned char charCode = '\0';
  unsigned char charCodeWithoutMod = '\0';
  const char* keySym = nullptr;

  NSEventType type = [theEvent type];
  BOOL isPress = (type == NSEventTypeKeyDown);

  if (type == NSEventTypeKeyUp || type == NSEventTypeKeyDown)
  {
    // Try to get the characters associated with the key event as a BasicLatin or Latin1 string.
    const char* keyedChars = [[theEvent characters] cStringUsingEncoding:NSISOLatin1StringEncoding];
    if (keyedChars)
    {
      charCode = static_cast<unsigned char>(keyedChars[0]);
    }
    keyedChars =
      [[theEvent charactersIgnoringModifiers] cStringUsingEncoding:NSISOLatin1StringEncoding];
    if (keyedChars)
    {
      charCodeWithoutMod = static_cast<unsigned char>(keyedChars[0]);
    }

    // Recover keyCode, fallback on code without mod
    // if keyCode is invalid.
    keyCode = charCode;
    if (charCode == 0)
    {
      keyCode = charCodeWithoutMod;
    }

    // Get the virtual key code and convert it to a keysym as best we can.
    unsigned short macKeyCode = [theEvent keyCode];
    if (macKeyCode < 128)
    {
      keySym = MacKeyCodeToKeySymTable[macKeyCode];
    }
    if (keySym == nullptr)
    {
      keySym = UnicodeToKeySymTable[charCode];
    }
    if (keySym == nullptr)
    {
      keySym = UnicodeToKeySymTable[charCodeWithoutMod];
    }
    if (keySym == nullptr)
    {
      keySym = "None";
    }
  }
  else if (type == NSEventTypeFlagsChanged)
  {
    // Check to see what modifier flag changed.
    if (controlDown != interactor->GetControlKey())
    {
      keySym = "Control_L";
      isPress = (controlDown != 0);
    }
    else if (shiftDown != interactor->GetShiftKey())
    {
      keySym = "Shift_L";
      isPress = (shiftDown != 0);
    }
    else if (altDown != interactor->GetAltKey())
    {
      keySym = "Alt_L";
      isPress = (altDown != 0);
    }
    else
    {
      return;
    }

    theEventId = (isPress ? vtkCommand::KeyPressEvent : vtkCommand::KeyReleaseEvent);
  }
  else // No info from which to generate a VTK key event!
  {
    return;
  }

  if (keySym == nullptr)
  {
    keySym = "None";
  }

  interactor->SetEventInformation(static_cast<int>(backingLoc.x), static_cast<int>(backingLoc.y),
    controlDown, shiftDown, static_cast<char>(keyCode), 1, keySym);
  interactor->SetAltKey(altDown);

  interactor->InvokeEvent(theEventId, nullptr);
  if (isPress && keyCode != '\0')
  {
    interactor->InvokeEvent(vtkCommand::CharEvent, nullptr);
  }
}

//----------------------------------------------------------------------------
// Convert a Cocoa motion event into a VTK button event
- (void)invokeVTKMoveEvent:(unsigned long)theEventId cocoaEvent:(NSEvent*)theEvent
{
  vtkCocoaRenderWindowInteractor* interactor = [self getInteractor];
  vtkCocoaRenderWindow* renWin = vtkCocoaRenderWindow::SafeDownCast([self getVTKRenderWindow]);

  if (!interactor || !renWin)
  {
    return;
  }

  // Get the location of the mouse event relative to this NSView's bottom
  // left corner. Since this is a mouse event, we can use locationInWindow.
  NSPoint windowLoc = [theEvent locationInWindow];
  NSPoint viewLoc = [self convertPoint:windowLoc fromView:nil];
  NSPoint backingLoc = [self convertPointToBacking:viewLoc];

  NSUInteger flags = [theEvent modifierFlags];
  int shiftDown = ((flags & NSEventModifierFlagShift) != 0);
  int controlDown = ((flags & (NSEventModifierFlagControl | NSEventModifierFlagCommand)) != 0);
  int altDown = ((flags & NSEventModifierFlagOption) != 0);

  interactor->SetEventInformation(
    static_cast<int>(backingLoc.x), static_cast<int>(backingLoc.y), controlDown, shiftDown);
  interactor->SetAltKey(altDown);
  interactor->InvokeEvent(theEventId, nullptr);
}

//----------------------------------------------------------------------------
// Convert a Cocoa motion event into a VTK button event
- (void)invokeVTKButtonEvent:(unsigned long)theEventId cocoaEvent:(NSEvent*)theEvent
{
  vtkCocoaRenderWindowInteractor* interactor = [self getInteractor];
  vtkCocoaRenderWindow* renWin = vtkCocoaRenderWindow::SafeDownCast([self getVTKRenderWindow]);

  if (!interactor || !renWin)
  {
    return;
  }

  // Get the location of the mouse event relative to this NSView's bottom
  // left corner. Since this is a mouse event, we can use locationInWindow.
  NSPoint windowLoc = [theEvent locationInWindow];
  NSPoint viewLoc = [self convertPoint:windowLoc fromView:nil];
  NSPoint backingLoc = [self convertPointToBacking:viewLoc];

  int clickCount = static_cast<int>([theEvent clickCount]);
  int repeatCount = ((clickCount > 1) ? clickCount - 1 : 0);

  NSUInteger flags = [theEvent modifierFlags];
  int shiftDown = ((flags & NSEventModifierFlagShift) != 0);
  int controlDown = ((flags & (NSEventModifierFlagControl | NSEventModifierFlagCommand)) != 0);
  int altDown = ((flags & NSEventModifierFlagOption) != 0);

  interactor->SetEventInformation(static_cast<int>(backingLoc.x), static_cast<int>(backingLoc.y),
    controlDown, shiftDown, 0, repeatCount);
  interactor->SetAltKey(altDown);
  interactor->InvokeEvent(theEventId, nullptr);
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)keyDown:(NSEvent*)theEvent
{
  [self invokeVTKKeyEvent:vtkCommand::KeyPressEvent cocoaEvent:theEvent];
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)keyUp:(NSEvent*)theEvent
{
  [self invokeVTKKeyEvent:vtkCommand::KeyReleaseEvent cocoaEvent:theEvent];
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)flagsChanged:(NSEvent*)theEvent
{
  // what kind of event it is will be decided by invokeVTKKeyEvent
  [self invokeVTKKeyEvent:vtkCommand::AnyEvent cocoaEvent:theEvent];
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)mouseMoved:(NSEvent*)theEvent
{
  // Note: this method will only be called if this view's NSWindow
  // is set to receive mouse moved events.  See setAcceptsMouseMovedEvents:
  // An NSWindow created by vtk automatically does accept such events.

  // Ignore motion outside the view in order to mimic other interactors
  NSPoint windowLoc = [theEvent locationInWindow];
  NSPoint viewLoc = [self convertPoint:windowLoc fromView:nil];
  if (NSPointInRect(viewLoc, [self visibleRect]))
  {
    [self invokeVTKMoveEvent:vtkCommand::MouseMoveEvent cocoaEvent:theEvent];
  }
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)mouseDragged:(NSEvent*)theEvent
{
  [self invokeVTKMoveEvent:vtkCommand::MouseMoveEvent cocoaEvent:theEvent];
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)rightMouseDragged:(NSEvent*)theEvent
{
  [self invokeVTKMoveEvent:vtkCommand::MouseMoveEvent cocoaEvent:theEvent];
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)otherMouseDragged:(NSEvent*)theEvent
{
  [self invokeVTKMoveEvent:vtkCommand::MouseMoveEvent cocoaEvent:theEvent];
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)mouseEntered:(NSEvent*)theEvent
{
  // Note: the mouseEntered/mouseExited events depend on the maintenance of
  // the tracking area (updateTrackingAreas).
  [self invokeVTKMoveEvent:vtkCommand::EnterEvent cocoaEvent:theEvent];
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)mouseExited:(NSEvent*)theEvent
{
  [self invokeVTKMoveEvent:vtkCommand::LeaveEvent cocoaEvent:theEvent];
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)scrollWheel:(NSEvent*)theEvent
{
  CGFloat dy = [theEvent deltaY];

  unsigned long eventId = 0;

  if (dy > 0)
  {
    eventId = vtkCommand::MouseWheelForwardEvent;
  }
  else if (dy < 0)
  {
    eventId = vtkCommand::MouseWheelBackwardEvent;
  }

  if (eventId != 0)
  {
    [self invokeVTKMoveEvent:eventId cocoaEvent:theEvent];
  }
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)mouseDown:(NSEvent*)theEvent
{
  [self invokeVTKButtonEvent:vtkCommand::LeftButtonPressEvent cocoaEvent:theEvent];
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)rightMouseDown:(NSEvent*)theEvent
{
  [self invokeVTKButtonEvent:vtkCommand::RightButtonPressEvent cocoaEvent:theEvent];
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)otherMouseDown:(NSEvent*)theEvent
{
  [self invokeVTKButtonEvent:vtkCommand::MiddleButtonPressEvent cocoaEvent:theEvent];
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)mouseUp:(NSEvent*)theEvent
{
  [self invokeVTKButtonEvent:vtkCommand::LeftButtonReleaseEvent cocoaEvent:theEvent];
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)rightMouseUp:(NSEvent*)theEvent
{
  [self invokeVTKButtonEvent:vtkCommand::RightButtonReleaseEvent cocoaEvent:theEvent];
}

//----------------------------------------------------------------------------
// Overridden (from NSResponder).
- (void)otherMouseUp:(NSEvent*)theEvent
{
  [self invokeVTKButtonEvent:vtkCommand::MiddleButtonReleaseEvent cocoaEvent:theEvent];
}

//----------------------------------------------------------------------------
// From NSDraggingDestination protocol.
- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender
{
  NSArray* types = [[sender draggingPasteboard] types];

  // kUTTypeFileURL is deprecated starting with macOS 12.0 but its replacement,
  // UTTypeFileURL, is in UniformTypeIdentfiers.framework which doesn't exist
  // on older versions of macOS.  Conditionally using it would require conditionally
  // linking to that new framework, which just isn't worth the hassle when both
  // are just syntactic sugar for the string "public.file-url".
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  NSString* supportedDragType = (NSString*)kUTTypeFileURL;
#pragma clang diagnostic pop

  if ([types containsObject:supportedDragType])
  {
    return NSDragOperationCopy;
  }
  return NSDragOperationNone;
}

//----------------------------------------------------------------------------
// From NSDraggingDestination protocol.
- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender
{
  vtkCocoaRenderWindowInteractor* interactor = [self getInteractor];

  NSPoint pt = [sender draggingLocation];
  NSPoint viewLoc = [self convertPoint:pt fromView:nil];
  NSPoint backingLoc = [self convertPointToBacking:viewLoc];
  double location[2];
  location[0] = backingLoc.x;
  location[1] = backingLoc.y;
  interactor->InvokeEvent(vtkCommand::UpdateDropLocationEvent, location);

  vtkNew<vtkStringArray> filePaths;
  NSPasteboard* pboard = [sender draggingPasteboard];
  NSArray* fileURLs = [pboard readObjectsForClasses:@[ [NSURL class] ] options:nil];
  for (NSURL* fileURL in fileURLs)
  {
    const char* filePath = [fileURL fileSystemRepresentation];
    filePaths->InsertNextValue(filePath);
  }

  if (filePaths->GetNumberOfTuples() > 0)
  {
    int shiftDown = 0;
    int controlDown = 0;
    int altDown = 0;
    NSWindow* draggingDestinationWindow = [sender draggingDestinationWindow];
    NSEvent* currentEvent = [draggingDestinationWindow currentEvent];
    if (currentEvent)
    {
      NSUInteger flags = [currentEvent modifierFlags];
      shiftDown = ((flags & NSEventModifierFlagShift) != 0);
      controlDown = ((flags & (NSEventModifierFlagControl | NSEventModifierFlagCommand)) != 0);
      altDown = ((flags & NSEventModifierFlagOption) != 0);
    }

    interactor->SetEventInformation(
      static_cast<int>(backingLoc.x), static_cast<int>(backingLoc.y), controlDown, shiftDown);
    interactor->SetAltKey(altDown);

    interactor->InvokeEvent(vtkCommand::DropFilesEvent, filePaths);
    return YES;
  }

  return NO;
}

//----------------------------------------------------------------------------
// Private
- (void)modifyDPIForBackingScaleFactorOfWindow:(/*nullable*/ NSWindow*)window
{
  // Convert from points to pixels.
  NSRect viewRect = [self frame];
  NSRect backingViewRect = [self convertRectToBacking:viewRect];
  CGFloat viewHeight = NSHeight(viewRect);
  CGFloat backingViewHeight = NSHeight(backingViewRect);
  CGFloat backingScaleFactor = 1.0;
  if (viewHeight > 0.0 && backingViewHeight > 0.0)
  {
    // the scale factor based on convertRectToBacking
    backingScaleFactor = backingViewHeight / viewHeight;
  }
  else if (window)
  {
    // fall back to less reliable method
    backingScaleFactor = [window backingScaleFactor];
  }
  assert(backingScaleFactor >= 1.0);

  vtkCocoaRenderWindow* renderWindow = [self getVTKRenderWindow];
  if (renderWindow)
  {
    // Ordinarily, DPI is hardcoded to 72, but in order for vtkTextActors
    // to have the correct apparent size, we adjust it per the NSWindow's
    // scaling factor.
    renderWindow->SetDPI(lround(72.0 * backingScaleFactor));
  }
}

//----------------------------------------------------------------------------
// Overridden (from NSView).
- (void)viewWillMoveToWindow:(/*nullable*/ NSWindow*)inNewWindow
{
  [super viewWillMoveToWindow:inNewWindow];

  [self modifyDPIForBackingScaleFactorOfWindow:inNewWindow];
}

//----------------------------------------------------------------------------
// Overridden (from NSView).
- (void)viewDidChangeBackingProperties
{
  [super viewDidChangeBackingProperties];

  NSWindow* window = [self window];
  [self modifyDPIForBackingScaleFactorOfWindow:window];
}

@end
