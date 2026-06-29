// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#if defined(COBALT_RENDERER_XLIB_SUPPORT) || defined(COBALT_RENDERER_XCB_SUPPORT)
#include <X11/Xlib.h>

// Default to stripping out X11 preprocessor macros if no setting has been defined
#ifndef COBALT_RENDERER_STRIP_X11_PREPROCESSOR_MACROS
#define COBALT_RENDERER_STRIP_X11_PREPROCESSOR_MACROS 1
#endif

namespace cobalt { namespace graphics {

struct X11Constants
{
	// Typedefs
#ifdef Bool
	using BoolType = Bool;
#endif
#ifdef Status
	using StatusType = Status;
#endif

	// Constants
	static constexpr auto kAbove = Above;
	static constexpr auto kAllocAll = AllocAll;
	static constexpr auto kAllocNone = AllocNone;
	static constexpr auto kAllowExposures = AllowExposures;
	static constexpr auto kAllTemporary = AllTemporary;
	static constexpr auto kAlreadyGrabbed = AlreadyGrabbed;
	static constexpr auto kAlways = Always;
	static constexpr auto kAnyButton = AnyButton;
	static constexpr auto kAnyKey = AnyKey;
	static constexpr auto kAnyModifier = AnyModifier;
	static constexpr auto kAnyPropertyType = AnyPropertyType;
	static constexpr auto kArcChord = ArcChord;
	static constexpr auto kArcPieSlice = ArcPieSlice;
	static constexpr auto kAsyncBoth = AsyncBoth;
	static constexpr auto kAsyncKeyboard = AsyncKeyboard;
	static constexpr auto kAsyncPointer = AsyncPointer;
	static constexpr auto kAutoRepeatModeDefault = AutoRepeatModeDefault;
	static constexpr auto kAutoRepeatModeOff = AutoRepeatModeOff;
	static constexpr auto kAutoRepeatModeOn = AutoRepeatModeOn;
	static constexpr auto kBadAccess = BadAccess;
	static constexpr auto kBadAlloc = BadAlloc;
	static constexpr auto kBadAtom = BadAtom;
	static constexpr auto kBadColor = BadColor;
	static constexpr auto kBadCursor = BadCursor;
	static constexpr auto kBadDrawable = BadDrawable;
	static constexpr auto kBadFont = BadFont;
	static constexpr auto kBadGC = BadGC;
	static constexpr auto kBadIDChoice = BadIDChoice;
	static constexpr auto kBadImplementation = BadImplementation;
	static constexpr auto kBadLength = BadLength;
	static constexpr auto kBadMatch = BadMatch;
	static constexpr auto kBadName = BadName;
	static constexpr auto kBadPixmap = BadPixmap;
	static constexpr auto kBadRequest = BadRequest;
	static constexpr auto kBadValue = BadValue;
	static constexpr auto kBadWindow = BadWindow;
	static constexpr auto kBelow = Below;
	static constexpr auto kBottomIf = BottomIf;
	static constexpr auto kButton1 = Button1;
	static constexpr auto kButton1Mask = Button1Mask;
	static constexpr auto kButton1MotionMask = Button1MotionMask;
	static constexpr auto kButton2 = Button2;
	static constexpr auto kButton2Mask = Button2Mask;
	static constexpr auto kButton2MotionMask = Button2MotionMask;
	static constexpr auto kButton3 = Button3;
	static constexpr auto kButton3Mask = Button3Mask;
	static constexpr auto kButton3MotionMask = Button3MotionMask;
	static constexpr auto kButton4 = Button4;
	static constexpr auto kButton4Mask = Button4Mask;
	static constexpr auto kButton4MotionMask = Button4MotionMask;
	static constexpr auto kButton5 = Button5;
	static constexpr auto kButton5Mask = Button5Mask;
	static constexpr auto kButton5MotionMask = Button5MotionMask;
	static constexpr auto kButtonMotionMask = ButtonMotionMask;
	static constexpr auto kButtonPress = ButtonPress;
	static constexpr auto kButtonPressMask = ButtonPressMask;
	static constexpr auto kButtonRelease = ButtonRelease;
	static constexpr auto kButtonReleaseMask = ButtonReleaseMask;
	static constexpr auto kCapButt = CapButt;
	static constexpr auto kCapNotLast = CapNotLast;
	static constexpr auto kCapProjecting = CapProjecting;
	static constexpr auto kCapRound = CapRound;
	static constexpr auto kCenterGravity = CenterGravity;
	static constexpr auto kCirculateNotify = CirculateNotify;
	static constexpr auto kCirculateRequest = CirculateRequest;
	static constexpr auto kClientMessage = ClientMessage;
	static constexpr auto kClipByChildren = ClipByChildren;
	static constexpr auto kColormapChangeMask = ColormapChangeMask;
	static constexpr auto kColormapInstalled = ColormapInstalled;
	static constexpr auto kColormapNotify = ColormapNotify;
	static constexpr auto kColormapUninstalled = ColormapUninstalled;
	static constexpr auto kComplex = Complex;
	static constexpr auto kConfigureNotify = ConfigureNotify;
	static constexpr auto kConfigureRequest = ConfigureRequest;
	static constexpr auto kControlMapIndex = ControlMapIndex;
	static constexpr auto kControlMask = ControlMask;
	static constexpr auto kConvex = Convex;
	static constexpr auto kCoordModeOrigin = CoordModeOrigin;
	static constexpr auto kCoordModePrevious = CoordModePrevious;
	static constexpr auto kCopyFromParent = CopyFromParent;
	static constexpr auto kCreateNotify = CreateNotify;
	static constexpr auto kCurrentTime = CurrentTime;
	static constexpr auto kCursorShape = CursorShape;
	static constexpr auto kCWBackingPixel = CWBackingPixel;
	static constexpr auto kCWBackingPlanes = CWBackingPlanes;
	static constexpr auto kCWBackingStore = CWBackingStore;
	static constexpr auto kCWBackPixel = CWBackPixel;
	static constexpr auto kCWBackPixmap = CWBackPixmap;
	static constexpr auto kCWBitGravity = CWBitGravity;
	static constexpr auto kCWBorderPixel = CWBorderPixel;
	static constexpr auto kCWBorderPixmap = CWBorderPixmap;
	static constexpr auto kCWBorderWidth = CWBorderWidth;
	static constexpr auto kCWColormap = CWColormap;
	static constexpr auto kCWCursor = CWCursor;
	static constexpr auto kCWDontPropagate = CWDontPropagate;
	static constexpr auto kCWEventMask = CWEventMask;
	static constexpr auto kCWHeight = CWHeight;
	static constexpr auto kCWOverrideRedirect = CWOverrideRedirect;
	static constexpr auto kCWSaveUnder = CWSaveUnder;
	static constexpr auto kCWSibling = CWSibling;
	static constexpr auto kCWStackMode = CWStackMode;
	static constexpr auto kCWWidth = CWWidth;
	static constexpr auto kCWWinGravity = CWWinGravity;
	static constexpr auto kCWX = CWX;
	static constexpr auto kCWY = CWY;
	static constexpr auto kDefaultBlanking = DefaultBlanking;
	static constexpr auto kDefaultExposures = DefaultExposures;
	static constexpr auto kDestroyAll = DestroyAll;
	static constexpr auto kDestroyNotify = DestroyNotify;
	static constexpr auto kDirectColor = DirectColor;
	static constexpr auto kDisableAccess = DisableAccess;
	static constexpr auto kDisableScreenInterval = DisableScreenInterval;
	static constexpr auto kDisableScreenSaver = DisableScreenSaver;
	static constexpr auto kDoBlue = DoBlue;
	static constexpr auto kDoGreen = DoGreen;
	static constexpr auto kDontAllowExposures = DontAllowExposures;
	static constexpr auto kDontPreferBlanking = DontPreferBlanking;
	static constexpr auto kDoRed = DoRed;
	static constexpr auto kEastGravity = EastGravity;
	static constexpr auto kEnableAccess = EnableAccess;
	static constexpr auto kEnterNotify = EnterNotify;
	static constexpr auto kEnterWindowMask = EnterWindowMask;
	static constexpr auto kEvenOddRule = EvenOddRule;
	static constexpr auto kExpose = Expose;
	static constexpr auto kExposureMask = ExposureMask;
	static constexpr auto kFamilyChaos = FamilyChaos;
	static constexpr auto kFamilyDECnet = FamilyDECnet;
	static constexpr auto kFamilyInternet = FamilyInternet;
	static constexpr auto kFamilyInternet6 = FamilyInternet6;
	static constexpr auto kFamilyServerInterpreted = FamilyServerInterpreted;
	static constexpr auto kFillOpaqueStippled = FillOpaqueStippled;
	static constexpr auto kFillSolid = FillSolid;
	static constexpr auto kFillStippled = FillStippled;
	static constexpr auto kFillTiled = FillTiled;
	static constexpr auto kFirstExtensionError = FirstExtensionError;
	static constexpr auto kFocusChangeMask = FocusChangeMask;
	static constexpr auto kFocusIn = FocusIn;
	static constexpr auto kFocusOut = FocusOut;
	static constexpr auto kFontChange = FontChange;
	static constexpr auto kFontLeftToRight = FontLeftToRight;
	static constexpr auto kFontRightToLeft = FontRightToLeft;
	static constexpr auto kForgetGravity = ForgetGravity;
	static constexpr auto kGCArcMode = GCArcMode;
	static constexpr auto kGCBackground = GCBackground;
	static constexpr auto kGCCapStyle = GCCapStyle;
	static constexpr auto kGCClipMask = GCClipMask;
	static constexpr auto kGCClipXOrigin = GCClipXOrigin;
	static constexpr auto kGCClipYOrigin = GCClipYOrigin;
	static constexpr auto kGCDashList = GCDashList;
	static constexpr auto kGCDashOffset = GCDashOffset;
	static constexpr auto kGCFillRule = GCFillRule;
	static constexpr auto kGCFillStyle = GCFillStyle;
	static constexpr auto kGCFont = GCFont;
	static constexpr auto kGCForeground = GCForeground;
	static constexpr auto kGCFunction = GCFunction;
	static constexpr auto kGCGraphicsExposures = GCGraphicsExposures;
	static constexpr auto kGCJoinStyle = GCJoinStyle;
	static constexpr auto kGCLastBit = GCLastBit;
	static constexpr auto kGCLineStyle = GCLineStyle;
	static constexpr auto kGCLineWidth = GCLineWidth;
	static constexpr auto kGCPlaneMask = GCPlaneMask;
	static constexpr auto kGCStipple = GCStipple;
	static constexpr auto kGCSubwindowMode = GCSubwindowMode;
	static constexpr auto kGCTile = GCTile;
	static constexpr auto kGCTileStipXOrigin = GCTileStipXOrigin;
	static constexpr auto kGCTileStipYOrigin = GCTileStipYOrigin;
	static constexpr auto kGenericEvent = GenericEvent;
	static constexpr auto kGrabFrozen = GrabFrozen;
	static constexpr auto kGrabInvalidTime = GrabInvalidTime;
	static constexpr auto kGrabModeAsync = GrabModeAsync;
	static constexpr auto kGrabModeSync = GrabModeSync;
	static constexpr auto kGrabNotViewable = GrabNotViewable;
	static constexpr auto kGrabSuccess = GrabSuccess;
	static constexpr auto kGraphicsExpose = GraphicsExpose;
	static constexpr auto kGravityNotify = GravityNotify;
	static constexpr auto kGrayScale = GrayScale;
	static constexpr auto kGXand = GXand;
	static constexpr auto kGXandInverted = GXandInverted;
	static constexpr auto kGXandReverse = GXandReverse;
	static constexpr auto kGXclear = GXclear;
	static constexpr auto kGXcopy = GXcopy;
	static constexpr auto kGXcopyInverted = GXcopyInverted;
	static constexpr auto kGXequiv = GXequiv;
	static constexpr auto kGXinvert = GXinvert;
	static constexpr auto kGXnand = GXnand;
	static constexpr auto kGXnoop = GXnoop;
	static constexpr auto kGXnor = GXnor;
	static constexpr auto kGXor = GXor;
	static constexpr auto kGXorInverted = GXorInverted;
	static constexpr auto kGXorReverse = GXorReverse;
	static constexpr auto kGXset = GXset;
	static constexpr auto kGXxor = GXxor;
	static constexpr auto kHostDelete = HostDelete;
	static constexpr auto kHostInsert = HostInsert;
	static constexpr auto kIncludeInferiors = IncludeInferiors;
	static constexpr auto kInputFocus = InputFocus;
	static constexpr auto kInputOnly = InputOnly;
	static constexpr auto kInputOutput = InputOutput;
	static constexpr auto kIsUnmapped = IsUnmapped;
	static constexpr auto kIsUnviewable = IsUnviewable;
	static constexpr auto kIsViewable = IsViewable;
	static constexpr auto kJoinBevel = JoinBevel;
	static constexpr auto kJoinMiter = JoinMiter;
	static constexpr auto kJoinRound = JoinRound;
	static constexpr auto kKBAutoRepeatMode = KBAutoRepeatMode;
	static constexpr auto kKBBellDuration = KBBellDuration;
	static constexpr auto kKBBellPercent = KBBellPercent;
	static constexpr auto kKBBellPitch = KBBellPitch;
	static constexpr auto kKBKey = KBKey;
	static constexpr auto kKBKeyClickPercent = KBKeyClickPercent;
	static constexpr auto kKBLed = KBLed;
	static constexpr auto kKBLedMode = KBLedMode;
	static constexpr auto kKeymapNotify = KeymapNotify;
	static constexpr auto kKeymapStateMask = KeymapStateMask;
	static constexpr auto kKeyPress = KeyPress;
	static constexpr auto kKeyPressMask = KeyPressMask;
	static constexpr auto kKeyRelease = KeyRelease;
	static constexpr auto kKeyReleaseMask = KeyReleaseMask;
	static constexpr auto kLASTEvent = LASTEvent;
	static constexpr auto kLastExtensionError = LastExtensionError;
	static constexpr auto kLeaveNotify = LeaveNotify;
	static constexpr auto kLeaveWindowMask = LeaveWindowMask;
	static constexpr auto kLedModeOff = LedModeOff;
	static constexpr auto kLedModeOn = LedModeOn;
	static constexpr auto kLineDoubleDash = LineDoubleDash;
	static constexpr auto kLineOnOffDash = LineOnOffDash;
	static constexpr auto kLineSolid = LineSolid;
	static constexpr auto kLockMapIndex = LockMapIndex;
	static constexpr auto kLockMask = LockMask;
	static constexpr auto kLowerHighest = LowerHighest;
	static constexpr auto kLSBFirst = LSBFirst;
	static constexpr auto kMapNotify = MapNotify;
	static constexpr auto kMappingBusy = MappingBusy;
	static constexpr auto kMappingFailed = MappingFailed;
	static constexpr auto kMappingKeyboard = MappingKeyboard;
	static constexpr auto kMappingModifier = MappingModifier;
	static constexpr auto kMappingNotify = MappingNotify;
	static constexpr auto kMappingPointer = MappingPointer;
	static constexpr auto kMappingSuccess = MappingSuccess;
	static constexpr auto kMapRequest = MapRequest;
	static constexpr auto kMod1MapIndex = Mod1MapIndex;
	static constexpr auto kMod1Mask = Mod1Mask;
	static constexpr auto kMod2MapIndex = Mod2MapIndex;
	static constexpr auto kMod2Mask = Mod2Mask;
	static constexpr auto kMod3MapIndex = Mod3MapIndex;
	static constexpr auto kMod3Mask = Mod3Mask;
	static constexpr auto kMod4MapIndex = Mod4MapIndex;
	static constexpr auto kMod4Mask = Mod4Mask;
	static constexpr auto kMod5MapIndex = Mod5MapIndex;
	static constexpr auto kMod5Mask = Mod5Mask;
	static constexpr auto kMotionNotify = MotionNotify;
	static constexpr auto kMSBFirst = MSBFirst;
	static constexpr auto kNoEventMask = NoEventMask;
	static constexpr auto kNoExpose = NoExpose;
	static constexpr auto kNonconvex = Nonconvex;
	static constexpr auto kNone = None;
	static constexpr auto kNorthEastGravity = NorthEastGravity;
	static constexpr auto kNorthGravity = NorthGravity;
	static constexpr auto kNorthWestGravity = NorthWestGravity;
	static constexpr auto kNoSymbol = NoSymbol;
	static constexpr auto kNotifyAncestor = NotifyAncestor;
	static constexpr auto kNotifyDetailNone = NotifyDetailNone;
	static constexpr auto kNotifyGrab = NotifyGrab;
	static constexpr auto kNotifyHint = NotifyHint;
	static constexpr auto kNotifyInferior = NotifyInferior;
	static constexpr auto kNotifyNonlinear = NotifyNonlinear;
	static constexpr auto kNotifyNonlinearVirtual = NotifyNonlinearVirtual;
	static constexpr auto kNotifyNormal = NotifyNormal;
	static constexpr auto kNotifyPointer = NotifyPointer;
	static constexpr auto kNotifyPointerRoot = NotifyPointerRoot;
	static constexpr auto kNotifyUngrab = NotifyUngrab;
	static constexpr auto kNotifyVirtual = NotifyVirtual;
	static constexpr auto kNotifyWhileGrabbed = NotifyWhileGrabbed;
	static constexpr auto kNotUseful = NotUseful;
	static constexpr auto kOpposite = Opposite;
	static constexpr auto kOwnerGrabButtonMask = OwnerGrabButtonMask;
	static constexpr auto kParentRelative = ParentRelative;
	static constexpr auto kPlaceOnBottom = PlaceOnBottom;
	static constexpr auto kPlaceOnTop = PlaceOnTop;
	static constexpr auto kPointerMotionHintMask = PointerMotionHintMask;
	static constexpr auto kPointerMotionMask = PointerMotionMask;
	static constexpr auto kPointerRoot = PointerRoot;
	static constexpr auto kPointerWindow = PointerWindow;
	static constexpr auto kPreferBlanking = PreferBlanking;
	static constexpr auto kPropertyChangeMask = PropertyChangeMask;
	static constexpr auto kPropertyDelete = PropertyDelete;
	static constexpr auto kPropertyNewValue = PropertyNewValue;
	static constexpr auto kPropertyNotify = PropertyNotify;
	static constexpr auto kPropModeAppend = PropModeAppend;
	static constexpr auto kPropModePrepend = PropModePrepend;
	static constexpr auto kPropModeReplace = PropModeReplace;
	static constexpr auto kPseudoColor = PseudoColor;
	static constexpr auto kRaiseLowest = RaiseLowest;
	static constexpr auto kReparentNotify = ReparentNotify;
	static constexpr auto kReplayKeyboard = ReplayKeyboard;
	static constexpr auto kReplayPointer = ReplayPointer;
	static constexpr auto kResizeRedirectMask = ResizeRedirectMask;
	static constexpr auto kResizeRequest = ResizeRequest;
	static constexpr auto kRetainPermanent = RetainPermanent;
	static constexpr auto kRetainTemporary = RetainTemporary;
	static constexpr auto kRevertToNone = RevertToNone;
	static constexpr auto kRevertToParent = RevertToParent;
	static constexpr auto kRevertToPointerRoot = RevertToPointerRoot;
	static constexpr auto kScreenSaverActive = ScreenSaverActive;
	static constexpr auto kScreenSaverReset = ScreenSaverReset;
	static constexpr auto kSelectionClear = SelectionClear;
	static constexpr auto kSelectionNotify = SelectionNotify;
	static constexpr auto kSelectionRequest = SelectionRequest;
	static constexpr auto kSetModeDelete = SetModeDelete;
	static constexpr auto kSetModeInsert = SetModeInsert;
	static constexpr auto kShiftMapIndex = ShiftMapIndex;
	static constexpr auto kShiftMask = ShiftMask;
	static constexpr auto kSouthEastGravity = SouthEastGravity;
	static constexpr auto kSouthGravity = SouthGravity;
	static constexpr auto kSouthWestGravity = SouthWestGravity;
	static constexpr auto kStaticColor = StaticColor;
	static constexpr auto kStaticGravity = StaticGravity;
	static constexpr auto kStaticGray = StaticGray;
	static constexpr auto kStippleShape = StippleShape;
	static constexpr auto kStructureNotifyMask = StructureNotifyMask;
	static constexpr auto kSubstructureNotifyMask = SubstructureNotifyMask;
	static constexpr auto kSubstructureRedirectMask = SubstructureRedirectMask;
	static constexpr auto kSuccess = Success;
	static constexpr auto kSyncBoth = SyncBoth;
	static constexpr auto kSyncKeyboard = SyncKeyboard;
	static constexpr auto kSyncPointer = SyncPointer;
	static constexpr auto kTileShape = TileShape;
	static constexpr auto kTopIf = TopIf;
	static constexpr auto kTrueColor = TrueColor;
	static constexpr auto kUnmapGravity = UnmapGravity;
	static constexpr auto kUnmapNotify = UnmapNotify;
	static constexpr auto kUnsorted = Unsorted;
	static constexpr auto kVisibilityChangeMask = VisibilityChangeMask;
	static constexpr auto kVisibilityFullyObscured = VisibilityFullyObscured;
	static constexpr auto kVisibilityNotify = VisibilityNotify;
	static constexpr auto kVisibilityPartiallyObscured = VisibilityPartiallyObscured;
	static constexpr auto kVisibilityUnobscured = VisibilityUnobscured;
	static constexpr auto kWestGravity = WestGravity;
	static constexpr auto kWhenMapped = WhenMapped;
	static constexpr auto kWindingRule = WindingRule;
	static constexpr auto kXYBitmap = XYBitmap;
	static constexpr auto kXYPixmap = XYPixmap;
	static constexpr auto kYSorted = YSorted;
	static constexpr auto kYXBanded = YXBanded;
	static constexpr auto kYXSorted = YXSorted;
	static constexpr auto kZPixmap = ZPixmap;

	// Conditional constants (not always defined on all platforms)
#ifdef AllPlanes
	static constexpr auto kAllPlanes = AllPlanes;
#endif
#ifdef False
	static constexpr auto kFalse = False;
#endif
#ifdef QueuedAfterFlush
	static constexpr auto kQueuedAfterFlush = QueuedAfterFlush;
#endif
#ifdef QueuedAfterReading
	static constexpr auto kQueuedAfterReading = QueuedAfterReading;
#endif
#ifdef QueuedAlready
	static constexpr auto kQueuedAlready = QueuedAlready;
#endif
#ifdef True
	static constexpr auto kTrue = True;
#endif
#ifdef XBufferOverflow
	static constexpr auto kXBufferOverflow = XBufferOverflow;
#endif
#ifdef XLookupBoth
	static constexpr auto kXLookupBoth = XLookupBoth;
#endif
#ifdef XLookupChars
	static constexpr auto kXLookupChars = XLookupChars;
#endif
#ifdef XLookupKeySym
	static constexpr auto kXLookupKeySym = XLookupKeySym;
#endif
#ifdef XLookupNone
	static constexpr auto kXLookupNone = XLookupNone;
#endif
#ifdef XIMHighlight
	static constexpr auto kXIMHighlight = XIMHighlight;
#endif
#ifdef XIMHotKeyStateOFF
	static constexpr auto kXIMHotKeyStateOFF = XIMHotKeyStateOFF;
#endif
#ifdef XIMHotKeyStateON
	static constexpr auto kXIMHotKeyStateON = XIMHotKeyStateON;
#endif
#ifdef XIMInitialState
	static constexpr auto kXIMInitialState = XIMInitialState;
#endif
#ifdef XIMPreeditArea
	static constexpr auto kXIMPreeditArea = XIMPreeditArea;
#endif
#ifdef XIMPreeditCallbacks
	static constexpr auto kXIMPreeditCallbacks = XIMPreeditCallbacks;
#endif
#ifdef XIMPreeditDisable
	static constexpr auto kXIMPreeditDisable = XIMPreeditDisable;
#endif
#ifdef XIMPreeditEnable
	static constexpr auto kXIMPreeditEnable = XIMPreeditEnable;
#endif
#ifdef XIMPreeditNone
	static constexpr auto kXIMPreeditNone = XIMPreeditNone;
#endif
#ifdef XIMPreeditNothing
	static constexpr auto kXIMPreeditNothing = XIMPreeditNothing;
#endif
#ifdef XIMPreeditPosition
	static constexpr auto kXIMPreeditPosition = XIMPreeditPosition;
#endif
#ifdef XIMPreeditUnKnown
	static constexpr auto kXIMPreeditUnKnown = XIMPreeditUnKnown;
#endif
#ifdef XIMPreserveState
	static constexpr auto kXIMPreserveState = XIMPreserveState;
#endif
#ifdef XIMPrimary
	static constexpr auto kXIMPrimary = XIMPrimary;
#endif
#ifdef XIMReverse
	static constexpr auto kXIMReverse = XIMReverse;
#endif
#ifdef XIMSecondary
	static constexpr auto kXIMSecondary = XIMSecondary;
#endif
#ifdef XIMStatusArea
	static constexpr auto kXIMStatusArea = XIMStatusArea;
#endif
#ifdef XIMStatusCallbacks
	static constexpr auto kXIMStatusCallbacks = XIMStatusCallbacks;
#endif
#ifdef XIMStatusNone
	static constexpr auto kXIMStatusNone = XIMStatusNone;
#endif
#ifdef XIMStatusNothing
	static constexpr auto kXIMStatusNothing = XIMStatusNothing;
#endif
#ifdef XIMStringConversionBottomEdge
	static constexpr auto kXIMStringConversionBottomEdge = XIMStringConversionBottomEdge;
#endif
#ifdef XIMStringConversionBuffer
	static constexpr auto kXIMStringConversionBuffer = XIMStringConversionBuffer;
#endif
#ifdef XIMStringConversionChar
	static constexpr auto kXIMStringConversionChar = XIMStringConversionChar;
#endif
#ifdef XIMStringConversionConcealed
	static constexpr auto kXIMStringConversionConcealed = XIMStringConversionConcealed;
#endif
#ifdef XIMStringConversionLeftEdge
	static constexpr auto kXIMStringConversionLeftEdge = XIMStringConversionLeftEdge;
#endif
#ifdef XIMStringConversionLine
	static constexpr auto kXIMStringConversionLine = XIMStringConversionLine;
#endif
#ifdef XIMStringConversionRetrieval
	static constexpr auto kXIMStringConversionRetrieval = XIMStringConversionRetrieval;
#endif
#ifdef XIMStringConversionRightEdge
	static constexpr auto kXIMStringConversionRightEdge = XIMStringConversionRightEdge;
#endif
#ifdef XIMStringConversionSubstitution
	static constexpr auto kXIMStringConversionSubstitution = XIMStringConversionSubstitution;
#endif
#ifdef XIMStringConversionTopEdge
	static constexpr auto kXIMStringConversionTopEdge = XIMStringConversionTopEdge;
#endif
#ifdef XIMStringConversionWord
	static constexpr auto kXIMStringConversionWord = XIMStringConversionWord;
#endif
#ifdef XIMStringConversionWrapped
	static constexpr auto kXIMStringConversionWrapped = XIMStringConversionWrapped;
#endif
#ifdef XIMTertiary
	static constexpr auto kXIMTertiary = XIMTertiary;
#endif
#ifdef XIMUnderline
	static constexpr auto kXIMUnderline = XIMUnderline;
#endif
#ifdef XIMVisibleToBackword
	static constexpr auto kXIMVisibleToBackword = XIMVisibleToBackword;
#endif
#ifdef XIMVisibleToCenter
	static constexpr auto kXIMVisibleToCenter = XIMVisibleToCenter;
#endif
#ifdef XIMVisibleToForward
	static constexpr auto kXIMVisibleToForward = XIMVisibleToForward;
#endif
#ifdef XNArea
	static constexpr auto kXNArea = XNArea;
#endif
#ifdef XNAreaNeeded
	static constexpr auto kXNAreaNeeded = XNAreaNeeded;
#endif
#ifdef XNBackground
	static constexpr auto kXNBackground = XNBackground;
#endif
#ifdef XNBackgroundPixmap
	static constexpr auto kXNBackgroundPixmap = XNBackgroundPixmap;
#endif
#ifdef XNBaseFontName
	static constexpr auto kXNBaseFontName = XNBaseFontName;
#endif
#ifdef XNClientWindow
	static constexpr auto kXNClientWindow = XNClientWindow;
#endif
#ifdef XNColormap
	static constexpr auto kXNColormap = XNColormap;
#endif
#ifdef XNContextualDrawing
	static constexpr auto kXNContextualDrawing = XNContextualDrawing;
#endif
#ifdef XNCursor
	static constexpr auto kXNCursor = XNCursor;
#endif
#ifdef XNDefaultString
	static constexpr auto kXNDefaultString = XNDefaultString;
#endif
#ifdef XNDestroyCallback
	static constexpr auto kXNDestroyCallback = XNDestroyCallback;
#endif
#ifdef XNDirectionalDependentDrawing
	static constexpr auto kXNDirectionalDependentDrawing = XNDirectionalDependentDrawing;
#endif
#ifdef XNFilterEvents
	static constexpr auto kXNFilterEvents = XNFilterEvents;
#endif
#ifdef XNFocusWindow
	static constexpr auto kXNFocusWindow = XNFocusWindow;
#endif
#ifdef XNFontInfo
	static constexpr auto kXNFontInfo = XNFontInfo;
#endif
#ifdef XNFontSet
	static constexpr auto kXNFontSet = XNFontSet;
#endif
#ifdef XNForeground
	static constexpr auto kXNForeground = XNForeground;
#endif
#ifdef XNGeometryCallback
	static constexpr auto kXNGeometryCallback = XNGeometryCallback;
#endif
#ifdef XNHotKey
	static constexpr auto kXNHotKey = XNHotKey;
#endif
#ifdef XNHotKeyState
	static constexpr auto kXNHotKeyState = XNHotKeyState;
#endif
#ifdef XNInputStyle
	static constexpr auto kXNInputStyle = XNInputStyle;
#endif
#ifdef XNLineSpace
	static constexpr auto kXNLineSpace = XNLineSpace;
#endif
#ifdef XNMissingCharSet
	static constexpr auto kXNMissingCharSet = XNMissingCharSet;
#endif
#ifdef XNOMAutomatic
	static constexpr auto kXNOMAutomatic = XNOMAutomatic;
#endif
#ifdef XNOrientation
	static constexpr auto kXNOrientation = XNOrientation;
#endif
#ifdef XNPreeditAttributes
	static constexpr auto kXNPreeditAttributes = XNPreeditAttributes;
#endif
#ifdef XNPreeditCaretCallback
	static constexpr auto kXNPreeditCaretCallback = XNPreeditCaretCallback;
#endif
#ifdef XNPreeditDoneCallback
	static constexpr auto kXNPreeditDoneCallback = XNPreeditDoneCallback;
#endif
#ifdef XNPreeditDrawCallback
	static constexpr auto kXNPreeditDrawCallback = XNPreeditDrawCallback;
#endif
#ifdef XNPreeditStartCallback
	static constexpr auto kXNPreeditStartCallback = XNPreeditStartCallback;
#endif
#ifdef XNPreeditState
	static constexpr auto kXNPreeditState = XNPreeditState;
#endif
#ifdef XNPreeditStateNotifyCallback
	static constexpr auto kXNPreeditStateNotifyCallback = XNPreeditStateNotifyCallback;
#endif
#ifdef XNQueryICValuesList
	static constexpr auto kXNQueryICValuesList = XNQueryICValuesList;
#endif
#ifdef XNQueryIMValuesList
	static constexpr auto kXNQueryIMValuesList = XNQueryIMValuesList;
#endif
#ifdef XNQueryInputStyle
	static constexpr auto kXNQueryInputStyle = XNQueryInputStyle;
#endif
#ifdef XNQueryOrientation
	static constexpr auto kXNQueryOrientation = XNQueryOrientation;
#endif
#ifdef XNR6PreeditCallback
	static constexpr auto kXNR6PreeditCallback = XNR6PreeditCallback;
#endif
#ifdef XNRequiredCharSet
	static constexpr auto kXNRequiredCharSet = XNRequiredCharSet;
#endif
#ifdef XNResetState
	static constexpr auto kXNResetState = XNResetState;
#endif
#ifdef XNResourceClass
	static constexpr auto kXNResourceClass = XNResourceClass;
#endif
#ifdef XNResourceName
	static constexpr auto kXNResourceName = XNResourceName;
#endif
#ifdef XNSeparatorofNestedList
	static constexpr auto kXNSeparatorofNestedList = XNSeparatorofNestedList;
#endif
#ifdef XNSpotLocation
	static constexpr auto kXNSpotLocation = XNSpotLocation;
#endif
#ifdef XNStatusAttributes
	static constexpr auto kXNStatusAttributes = XNStatusAttributes;
#endif
#ifdef XNStatusDoneCallback
	static constexpr auto kXNStatusDoneCallback = XNStatusDoneCallback;
#endif
#ifdef XNStatusDrawCallback
	static constexpr auto kXNStatusDrawCallback = XNStatusDrawCallback;
#endif
#ifdef XNStatusStartCallback
	static constexpr auto kXNStatusStartCallback = XNStatusStartCallback;
#endif
#ifdef XNStdColormap
	static constexpr auto kXNStdColormap = XNStdColormap;
#endif
#ifdef XNStringConversion
	static constexpr auto kXNStringConversion = XNStringConversion;
#endif
#ifdef XNStringConversionCallback
	static constexpr auto kXNStringConversionCallback = XNStringConversionCallback;
#endif
#ifdef XNVaNestedList
	static constexpr auto kXNVaNestedList = XNVaNestedList;
#endif
#ifdef XNVisiblePosition
	static constexpr auto kXNVisiblePosition = XNVisiblePosition;
#endif
};

}} // namespace cobalt::graphics

// Bulk undefine the X11 macros if requested. The original values can now be accessed using the constants defined above.
#if COBALT_RENDERER_STRIP_X11_PREPROCESSOR_MACROS
#undef Above
#undef AllocAll
#undef AllocNone
#undef AllowExposures
#undef AllTemporary
#undef AlreadyGrabbed
#undef Always
#undef AnyButton
#undef AnyKey
#undef AnyModifier
#undef AnyPropertyType
#undef ArcChord
#undef ArcPieSlice
#undef AsyncBoth
#undef AsyncKeyboard
#undef AsyncPointer
#undef AutoRepeatModeDefault
#undef AutoRepeatModeOff
#undef AutoRepeatModeOn
#undef BadAccess
#undef BadAlloc
#undef BadAtom
#undef BadColor
#undef BadCursor
#undef BadDrawable
#undef BadFont
#undef BadGC
#undef BadIDChoice
#undef BadImplementation
#undef BadLength
#undef BadMatch
#undef BadName
#undef BadPixmap
#undef BadRequest
#undef BadValue
#undef BadWindow
#undef Below
#undef BottomIf
#undef Button1
#undef Button1Mask
#undef Button1MotionMask
#undef Button2
#undef Button2Mask
#undef Button2MotionMask
#undef Button3
#undef Button3Mask
#undef Button3MotionMask
#undef Button4
#undef Button4Mask
#undef Button4MotionMask
#undef Button5
#undef Button5Mask
#undef Button5MotionMask
#undef ButtonMotionMask
#undef ButtonPress
#undef ButtonPressMask
#undef ButtonRelease
#undef ButtonReleaseMask
#undef CapButt
#undef CapNotLast
#undef CapProjecting
#undef CapRound
#undef CenterGravity
#undef CirculateNotify
#undef CirculateRequest
#undef ClientMessage
#undef ClipByChildren
#undef ColormapChangeMask
#undef ColormapInstalled
#undef ColormapNotify
#undef ColormapUninstalled
#undef Complex
#undef ConfigureNotify
#undef ConfigureRequest
#undef ControlMapIndex
#undef ControlMask
#undef Convex
#undef CoordModeOrigin
#undef CoordModePrevious
#undef CopyFromParent
#undef CreateNotify
#undef CurrentTime
#undef CursorShape
#undef CWBackingPixel
#undef CWBackingPlanes
#undef CWBackingStore
#undef CWBackPixel
#undef CWBackPixmap
#undef CWBitGravity
#undef CWBorderPixel
#undef CWBorderPixmap
#undef CWBorderWidth
#undef CWColormap
#undef CWCursor
#undef CWDontPropagate
#undef CWEventMask
#undef CWHeight
#undef CWOverrideRedirect
#undef CWSaveUnder
#undef CWSibling
#undef CWStackMode
#undef CWWidth
#undef CWWinGravity
#undef CWX
#undef CWY
#undef DefaultBlanking
#undef DefaultExposures
#undef DestroyAll
#undef DestroyNotify
#undef DirectColor
#undef DisableAccess
#undef DisableScreenInterval
#undef DisableScreenSaver
#undef DoBlue
#undef DoGreen
#undef DontAllowExposures
#undef DontPreferBlanking
#undef DoRed
#undef EastGravity
#undef EnableAccess
#undef EnterNotify
#undef EnterWindowMask
#undef EvenOddRule
#undef Expose
#undef ExposureMask
#undef FamilyChaos
#undef FamilyDECnet
#undef FamilyInternet
#undef FamilyInternet6
#undef FamilyServerInterpreted
#undef FillOpaqueStippled
#undef FillSolid
#undef FillStippled
#undef FillTiled
#undef FirstExtensionError
#undef FocusChangeMask
#undef FocusIn
#undef FocusOut
#undef FontChange
#undef FontLeftToRight
#undef FontRightToLeft
#undef ForgetGravity
#undef GCArcMode
#undef GCBackground
#undef GCCapStyle
#undef GCClipMask
#undef GCClipXOrigin
#undef GCClipYOrigin
#undef GCDashList
#undef GCDashOffset
#undef GCFillRule
#undef GCFillStyle
#undef GCFont
#undef GCForeground
#undef GCFunction
#undef GCGraphicsExposures
#undef GCJoinStyle
#undef GCLastBit
#undef GCLineStyle
#undef GCLineWidth
#undef GCPlaneMask
#undef GCStipple
#undef GCSubwindowMode
#undef GCTile
#undef GCTileStipXOrigin
#undef GCTileStipYOrigin
#undef GenericEvent
#undef GrabFrozen
#undef GrabInvalidTime
#undef GrabModeAsync
#undef GrabModeSync
#undef GrabNotViewable
#undef GrabSuccess
#undef GraphicsExpose
#undef GravityNotify
#undef GrayScale
#undef GXand
#undef GXandInverted
#undef GXandReverse
#undef GXclear
#undef GXcopy
#undef GXcopyInverted
#undef GXequiv
#undef GXinvert
#undef GXnand
#undef GXnoop
#undef GXnor
#undef GXor
#undef GXorInverted
#undef GXorReverse
#undef GXset
#undef GXxor
#undef HostDelete
#undef HostInsert
#undef IncludeInferiors
#undef InputFocus
#undef InputOnly
#undef InputOutput
#undef IsUnmapped
#undef IsUnviewable
#undef IsViewable
#undef JoinBevel
#undef JoinMiter
#undef JoinRound
#undef KBAutoRepeatMode
#undef KBBellDuration
#undef KBBellPercent
#undef KBBellPitch
#undef KBKey
#undef KBKeyClickPercent
#undef KBLed
#undef KBLedMode
#undef KeymapNotify
#undef KeymapStateMask
#undef KeyPress
#undef KeyPressMask
#undef KeyRelease
#undef KeyReleaseMask
#undef LASTEvent
#undef LastExtensionError
#undef LeaveNotify
#undef LeaveWindowMask
#undef LedModeOff
#undef LedModeOn
#undef LineDoubleDash
#undef LineOnOffDash
#undef LineSolid
#undef LockMapIndex
#undef LockMask
#undef LowerHighest
#undef LSBFirst
#undef MapNotify
#undef MappingBusy
#undef MappingFailed
#undef MappingKeyboard
#undef MappingModifier
#undef MappingNotify
#undef MappingPointer
#undef MappingSuccess
#undef MapRequest
#undef Mod1MapIndex
#undef Mod1Mask
#undef Mod2MapIndex
#undef Mod2Mask
#undef Mod3MapIndex
#undef Mod3Mask
#undef Mod4MapIndex
#undef Mod4Mask
#undef Mod5MapIndex
#undef Mod5Mask
#undef MotionNotify
#undef MSBFirst
#undef NoEventMask
#undef NoExpose
#undef Nonconvex
#undef None
#undef NorthEastGravity
#undef NorthGravity
#undef NorthWestGravity
#undef NoSymbol
#undef NotifyAncestor
#undef NotifyDetailNone
#undef NotifyGrab
#undef NotifyHint
#undef NotifyInferior
#undef NotifyNonlinear
#undef NotifyNonlinearVirtual
#undef NotifyNormal
#undef NotifyPointer
#undef NotifyPointerRoot
#undef NotifyUngrab
#undef NotifyVirtual
#undef NotifyWhileGrabbed
#undef NotUseful
#undef Opposite
#undef OwnerGrabButtonMask
#undef ParentRelative
#undef PlaceOnBottom
#undef PlaceOnTop
#undef PointerMotionHintMask
#undef PointerMotionMask
#undef PointerRoot
#undef PointerWindow
#undef PreferBlanking
#undef PropertyChangeMask
#undef PropertyDelete
#undef PropertyNewValue
#undef PropertyNotify
#undef PropModeAppend
#undef PropModePrepend
#undef PropModeReplace
#undef PseudoColor
#undef RaiseLowest
#undef ReparentNotify
#undef ReplayKeyboard
#undef ReplayPointer
#undef ResizeRedirectMask
#undef ResizeRequest
#undef RetainPermanent
#undef RetainTemporary
#undef RevertToNone
#undef RevertToParent
#undef RevertToPointerRoot
#undef ScreenSaverActive
#undef ScreenSaverReset
#undef SelectionClear
#undef SelectionNotify
#undef SelectionRequest
#undef SetModeDelete
#undef SetModeInsert
#undef ShiftMapIndex
#undef ShiftMask
#undef SouthEastGravity
#undef SouthGravity
#undef SouthWestGravity
#undef StaticColor
#undef StaticGravity
#undef StaticGray
#undef StippleShape
#undef StructureNotifyMask
#undef SubstructureNotifyMask
#undef SubstructureRedirectMask
#undef Success
#undef SyncBoth
#undef SyncKeyboard
#undef SyncPointer
#undef TileShape
#undef TopIf
#undef TrueColor
#undef UnmapGravity
#undef UnmapNotify
#undef Unsorted
#undef VisibilityChangeMask
#undef VisibilityFullyObscured
#undef VisibilityNotify
#undef VisibilityPartiallyObscured
#undef VisibilityUnobscured
#undef WestGravity
#undef WhenMapped
#undef WindingRule
#undef XYBitmap
#undef XYPixmap
#undef YSorted
#undef YXBanded
#undef YXSorted
#undef ZPixmap
#undef AllPlanes
#undef Bool
#undef False
#undef QueuedAfterFlush
#undef QueuedAfterReading
#undef QueuedAlready
#undef Status
#undef True
#undef XBufferOverflow
#undef XLookupBoth
#undef XLookupChars
#undef XLookupKeySym
#undef XLookupNone
#undef XIMHighlight
#undef XIMHotKeyStateOFF
#undef XIMHotKeyStateON
#undef XIMInitialState
#undef XIMPreeditArea
#undef XIMPreeditCallbacks
#undef XIMPreeditDisable
#undef XIMPreeditEnable
#undef XIMPreeditNone
#undef XIMPreeditNothing
#undef XIMPreeditPosition
#undef XIMPreeditUnKnown
#undef XIMPreserveState
#undef XIMPrimary
#undef XIMReverse
#undef XIMSecondary
#undef XIMStatusArea
#undef XIMStatusCallbacks
#undef XIMStatusNone
#undef XIMStatusNothing
#undef XIMStringConversionBottomEdge
#undef XIMStringConversionBuffer
#undef XIMStringConversionChar
#undef XIMStringConversionConcealed
#undef XIMStringConversionLeftEdge
#undef XIMStringConversionLine
#undef XIMStringConversionRetrieval
#undef XIMStringConversionRightEdge
#undef XIMStringConversionSubstitution
#undef XIMStringConversionTopEdge
#undef XIMStringConversionWord
#undef XIMStringConversionWrapped
#undef XIMTertiary
#undef XIMUnderline
#undef XIMVisibleToBackword
#undef XIMVisibleToCenter
#undef XIMVisibleToForward
#undef XNArea
#undef XNAreaNeeded
#undef XNBackground
#undef XNBackgroundPixmap
#undef XNBaseFontName
#undef XNClientWindow
#undef XNColormap
#undef XNContextualDrawing
#undef XNCursor
#undef XNDefaultString
#undef XNDestroyCallback
#undef XNDirectionalDependentDrawing
#undef XNFilterEvents
#undef XNFocusWindow
#undef XNFontInfo
#undef XNFontSet
#undef XNForeground
#undef XNGeometryCallback
#undef XNHotKey
#undef XNHotKeyState
#undef XNInputStyle
#undef XNLineSpace
#undef XNMissingCharSet
#undef XNOMAutomatic
#undef XNOrientation
#undef XNPreeditAttributes
#undef XNPreeditCaretCallback
#undef XNPreeditDoneCallback
#undef XNPreeditDrawCallback
#undef XNPreeditStartCallback
#undef XNPreeditState
#undef XNPreeditStateNotifyCallback
#undef XNQueryICValuesList
#undef XNQueryIMValuesList
#undef XNQueryInputStyle
#undef XNQueryOrientation
#undef XNR6PreeditCallback
#undef XNRequiredCharSet
#undef XNResetState
#undef XNResourceClass
#undef XNResourceName
#undef XNSeparatorofNestedList
#undef XNSpotLocation
#undef XNStatusAttributes
#undef XNStatusDoneCallback
#undef XNStatusDrawCallback
#undef XNStatusStartCallback
#undef XNStdColormap
#undef XNStringConversion
#undef XNStringConversionCallback
#undef XNVaNestedList
#undef XNVisiblePosition
#undef mblen
#undef mbtowc
#undef wctomb
#endif

#endif
