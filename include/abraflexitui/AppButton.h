#pragma once

#include "abraflexitui/TV.h"

namespace abraflexitui {

// A drop-in TButton replacement with fixed truecolor RGB colors instead of
// the classic 16-color BIOS palette entries. TButton::draw() resolves its
// colors through mapColor(1..8) (normal/default/selected/disabled/shortcut
// x3/shadow, see tvision's cpButton), which walks up the owning dialog's
// and application's palette chain - a chain this app never customizes, so
// it renders with tvision's stock colors (green background, by default).
// Terminals with a heavily customized 16-color ANSI theme can remap that
// stock green into something that clashes with the rest of the UI (e.g.
// a theme that recolors most of the app red/orange but leaves green as a
// pale, near-white tone). Overriding mapColor() here bypasses the whole
// palette/ANSI-16 chain for buttons specifically and returns explicit RGB
// colors, so buttons look the same regardless of the terminal's color
// scheme - the same "don't depend on what a specific palette index
// happens to render as" approach used for the list focus highlight.
class AppButton : public TButton {
public:
    using TButton::TButton;

    TColorAttr mapColor(uchar index) override;
};

} // namespace abraflexitui
