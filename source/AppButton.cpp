#include "abraflexitui/AppButton.h"

namespace abraflexitui {

namespace {

// Matches cpButton's 8 local indices: 1=normal, 2=default, 3=selected
// (pressed/focused), 4=disabled, 5-7=shortcut letter in each of those three
// states, 8=shadow. Index 8 is deliberately NOT handled here: TButton::draw()
// paints it as a flat fill (see tbutton.cpp, cShadow = getColor(8)), which is
// meant to read as "a darkened patch of this dialog's own background", not
// an unrelated color - a fixed shadow color looks fine against a dark dialog
// but shows up as a nonsensical black block against a light one. Falling
// through to TButton::mapColor(8) keeps the shadow adaptive, exactly like
// every other dialog element that isn't a button.
TColorAttr colorFor(uchar index) {
    switch (index) {
    case 1: // normal
        return TColorAttr(TColor(TColorRGB(0xFFFFFF)), TColor(TColorRGB(0x000000)));
    case 2: // default (primary action)
        return TColorAttr(TColor(TColorRGB(0xFFFFFF)), TColor(TColorRGB(0x000000)));
    case 3: // selected/pressed
        return TColorAttr(TColor(TColorRGB(0x000000)), TColor(TColorRGB(0xFFFFFF)));
    case 4: // disabled
        return TColorAttr(TColor(TColorRGB(0x808080)), TColor(TColorRGB(0x000000)));
    case 5: // shortcut letter, normal
        return TColorAttr(TColor(TColorRGB(0xF2C23E)), TColor(TColorRGB(0x000000)));
    case 6: // shortcut letter, default
        return TColorAttr(TColor(TColorRGB(0xF2C23E)), TColor(TColorRGB(0x000000)));
    case 7: // shortcut letter, selected
        return TColorAttr(TColor(TColorRGB(0xB34700)), TColor(TColorRGB(0xFFFFFF)));
    default:
        return TColorAttr(TColor(TColorRGB(0xFFFFFF)), TColor(TColorRGB(0x000000)));
    }
}

} // namespace

TColorAttr AppButton::mapColor(uchar index) {
    if (index >= 1 && index <= 7) {
        return colorFor(index);
    }

    return TButton::mapColor(index);
}

} // namespace abraflexitui
