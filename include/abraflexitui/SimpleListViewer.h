#pragma once

#include "abraflexitui/TV.h"

#include <string>
#include <vector>

namespace abraflexitui {

// A read-only TListViewer backed by a plain vector of already-formatted
// strings, one per row. Used for every tabular/list display in this app
// (companies, evidences, record grids, record detail field lines) instead of
// a separate TListBox/TDataCollection per screen.
class SimpleListViewer : public TListViewer {
public:
    SimpleListViewer(const TRect &bounds, TScrollBar *vScrollBar) noexcept;

    void setRows(std::vector<std::string> rows);
    const std::string &rowAt(short item) const;
    std::size_t rowCount() const { return rows_.size(); }

    void getText(char *dest, short item, short maxLen) override;

protected:
    std::vector<std::string> rows_;
};

} // namespace abraflexitui
