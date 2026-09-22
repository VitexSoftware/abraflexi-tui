#pragma once

#include "abraflexitui/TV.h"
#include "abraflexitui/SimpleListViewer.h"

#include <nlohmann/json.hpp>
#include <string>

namespace abraflexitui {

// Read-only "key: value" field list for a single record. Nested objects/
// arrays are pretty-printed inline (not flattened into dotted-key rows),
// since AbraFlexi relation objects have irregular shapes per evidence.
class RecordDetailView : public SimpleListViewer {
public:
    RecordDetailView(const TRect &bounds, TScrollBar *vScrollBar) noexcept;

    void showRecord(const nlohmann::json &record);
    void showMessage(const std::string &text);
};

} // namespace abraflexitui
