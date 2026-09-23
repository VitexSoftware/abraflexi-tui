#include "abraflexitui/TV.h"
#include "abraflexitui/SimpleListViewer.h"
#include "abraflexitui/WindowColors.h"

#include <cstring>

namespace abraflexitui {

SimpleListViewer::SimpleListViewer(const TRect &bounds, TScrollBar *vScrollBar) noexcept
    : TListViewer(bounds, 1, nullptr, vScrollBar) {
}

void SimpleListViewer::setRows(std::vector<std::string> rows) {
    rows_ = std::move(rows);
    topItem = 0;
    setRange(static_cast<short>(rows_.size()));
    // Every list in this app reserves row 0 for a header/status line (see
    // EvidenceListBox, RecordListBox, CompanyListView), so the first
    // selectable row is 1. Landing focus there means Enter/double-click
    // activate immediately on a freshly opened or filtered list, instead of
    // requiring an extra Down keypress past the header first. Going through
    // the virtual focusItem() (rather than assigning `focused` directly)
    // also drives subclass overrides, e.g. RecordListBox updating the
    // embedded detail pane for the newly focused row.
    focusItem(rows_.size() > 1 ? 1 : 0);
    drawView();
}

void SimpleListViewer::draw() {
    TListViewer::draw();

    // These lists commonly sit next to a search/filter TInputLine that keeps
    // real keyboard focus instead (see EvidenceListView, RecordListView), so
    // TListViewer::draw() never paints its normal bright "focused" highlight
    // here - at best it falls back to a dimmer "selected" palette entry that
    // some terminal color themes render almost identically to the plain
    // background (reported: focused row invisible under a custom theme).
    // Repaint just the focused row in reverse video on top, which is
    // guaranteed visible regardless of the resolved palette colors.
    if (range > 0 && focused >= 0 && focused < range) {
        const short line = static_cast<short>(focused - topItem);

        if (line >= 0 && line < size.y) {
            char text[256];
            getText(text, focused, 255);
            text[255] = '\0';

            const TColorAttr color = TColorAttr(getColor(2)).reversed();
            TDrawBuffer b;
            b.moveChar(0, ' ', color, static_cast<ushort>(size.x));
            b.moveStr(1, text, color, static_cast<ushort>(size.x > 1 ? size.x - 1 : 0));
            writeLine(0, line, size.x, 1, b);
        }
    }
}

// Same "don't trust the resolved palette color under an arbitrary terminal
// theme" reasoning as AppButton/StatusView: TListViewer's stock palette (see
// cpListViewer in views.h) resolves through the owning window's and
// application's palette chain, which this app never customizes, so under a
// terminal ANSI theme that recolors most indices red/orange the whole grid
// background turns light red (reported: entire record list unreadable).
// Overriding mapColor() here bypasses that chain for every SimpleListViewer
// (record grids, evidence/company lists, structure lists) with fixed RGB
// colors, so they render the same regardless of the terminal's color scheme.
TColorAttr SimpleListViewer::mapColor(uchar index) {
    static const uint32_t fg = 0xF0F0F0;
    static const uint32_t bg = kFieldBg;         // lighter "this is a data area" tone, matches InputLine.
    static const uint32_t selectedBg = 0x264F78; // accent blue.

    switch (index) {
    case 1: // Active
    case 2: // Inactive
    case 3: // Focused
        return TColorAttr(TColor(TColorRGB(fg)), TColor(TColorRGB(bg)));
    case 4: // Selected
        return TColorAttr(TColor(TColorRGB(fg)), TColor(TColorRGB(selectedBg)));
    case 5: // Divider
        return TColorAttr(TColor(TColorRGB(0x6B7C93)), TColor(TColorRGB(bg)));
    default:
        return TListViewer::mapColor(index);
    }
}

const std::string &SimpleListViewer::rowAt(short item) const {
    static const std::string empty;

    if (item < 0 || static_cast<std::size_t>(item) >= rows_.size()) {
        return empty;
    }

    return rows_[static_cast<std::size_t>(item)];
}

void SimpleListViewer::getText(char *dest, short item, short maxLen) {
    if (item < 0 || static_cast<std::size_t>(item) >= rows_.size()) {
        *dest = '\0';
        return;
    }

    std::strncpy(dest, rows_[static_cast<std::size_t>(item)].c_str(), static_cast<std::size_t>(maxLen));
    dest[maxLen] = '\0';
}

} // namespace abraflexitui
