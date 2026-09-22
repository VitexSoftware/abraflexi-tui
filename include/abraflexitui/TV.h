#pragma once

// Shared tvision include for the whole app. tvision's headers gate every
// class definition behind a "Uses_ClassName" macro that must be defined
// before <tvision/tv.h> is (re-)included; each translation unit normally has
// to repeat the exact list of Uses_ macros it needs (see tvision's own
// examples/tvdemo/*.cpp). To avoid repeating that list in every .cpp file of
// this small app, we define the superset once here and let every source
// file include this header first.
#define Uses_TKeys
#define Uses_TEvent
#define Uses_TRect
#define Uses_TPoint
#define Uses_TApplication
#define Uses_TDeskTop
#define Uses_TWindow
#define Uses_TDialog
#define Uses_TFrame
#define Uses_TGroup
#define Uses_TView
#define Uses_TScrollBar
#define Uses_TListViewer
#define Uses_TStaticText
#define Uses_TLabel
#define Uses_TButton
#define Uses_TInputLine
#define Uses_TSItem
#define Uses_TCluster
#define Uses_TCheckBoxes
#define Uses_TRadioButtons
#define Uses_TMemo
#define Uses_TEditor
#define Uses_TIndicator
#define Uses_TMenuBar
#define Uses_TSubMenu
#define Uses_TMenuItem
#define Uses_TStatusLine
#define Uses_TStatusItem
#define Uses_TStatusDef
#define Uses_MsgBox
#define Uses_TProgram
#define Uses_TScreen

#include <tvision/tv.h>
