#pragma once

#include "abraflexitui/TV.h"
#include "abraflexitui/CliClient.h"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace abraflexitui {

// Modal "pick a related record" dialog for a relation-type field
// (FieldSchema::relationEvidence, e.g. "typ-faktury-vydane"). Lists records
// from that evidence and, on selection, builds the same {"ref","showAs"}
// object shape AbraFlexi itself uses for relation values.
class RelationPickerDialog : public TDialog {
public:
    RelationPickerDialog(CliClient &client, std::string relationEvidence, std::string company);

    void applyQuery(const std::string &query);
    void handleEvent(TEvent &event) override;
    TColorAttr mapColor(uchar index) override;

    // Valid only after execView() returns cmOK.
    nlohmann::json selectedValue() const;

private:
    void loadItems();
    void setQueryText(const std::string &query);

    CliClient &client_;
    std::string relationEvidence_;
    std::string company_;

    std::vector<nlohmann::json> allRecords_;
    std::vector<std::string> allTexts_;
    std::vector<nlohmann::json> records_;
    std::vector<std::string> rowTexts_;
    std::string query_;

    TListViewer *list_ = nullptr;
    TInputLine *queryInput_ = nullptr;
    TScrollBar *bar_ = nullptr;
};

} // namespace abraflexitui
