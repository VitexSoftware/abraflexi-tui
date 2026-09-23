#pragma once

#include "abraflexitui/TV.h"
#include "abraflexitui/CliClient.h"
#include "abraflexitui/EvidenceSchema.h"
#include "abraflexitui/SimpleListViewer.h"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace abraflexitui {

// Read-only "key: value" field list for a single record. Nested objects/
// arrays are pretty-printed inline (not flattened into dotted-key rows),
// since AbraFlexi relation objects have irregular shapes per evidence.
class RecordDetailView : public SimpleListViewer {
public:
    RecordDetailView(const TRect &bounds, TScrollBar *vScrollBar) noexcept;

    // `schema` fields (when non-null) supply the Czech label, display
    // order and type-aware formatting (logic -> Ano/Ne, relation -> value
    // plus its target evidence); any record key not present in `schema`
    // is still shown, appended at the end under its raw name, so nothing
    // the API returns is ever silently hidden.
    void showRecord(const nlohmann::json &record, const std::vector<FieldSchema> *schema = nullptr);
    void showMessage(const std::string &text);
};

// One AbraFlexi record in its own desktop window, so several invoices or
// address-book rows can stay open and be tiled next to each other.
class RecordWindow : public TWindow {
public:
    RecordWindow(CliClient &client, const std::string &evidence, const std::string &id, const nlohmann::json &record,
                 std::string company = {});

    const std::string &evidence() const { return evidence_; }
    const std::string &recordId() const { return id_; }
    const std::string &company() const { return company_; }

    TColorAttr mapColor(uchar index) override;

private:
    std::string evidence_;
    std::string id_;
    std::string company_;
};

} // namespace abraflexitui
