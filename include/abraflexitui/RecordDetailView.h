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

// One AbraFlexi record in its own desktop window, so several invoices or
// address-book rows can stay open and be tiled next to each other.
class RecordWindow : public TWindow {
public:
    RecordWindow(const std::string &evidence, const std::string &id, const nlohmann::json &record);

    const std::string &evidence() const { return evidence_; }
    const std::string &recordId() const { return id_; }

private:
    std::string evidence_;
    std::string id_;
};

} // namespace abraflexitui
