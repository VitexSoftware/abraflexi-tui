#include "abraflexitui/TV.h"
#include "abraflexitui/SimpleListViewer.h"

#include <cstring>

namespace abraflexitui {

SimpleListViewer::SimpleListViewer(const TRect &bounds, TScrollBar *vScrollBar) noexcept
    : TListViewer(bounds, 1, nullptr, vScrollBar) {
}

void SimpleListViewer::setRows(std::vector<std::string> rows) {
    rows_ = std::move(rows);
    focused = 0;
    topItem = 0;
    setRange(static_cast<short>(rows_.size()));
    drawView();
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
