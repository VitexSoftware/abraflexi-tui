#pragma once

#include "abraflexitui/TV.h"
#include "abraflexitui/CliClient.h"
#include "abraflexitui/RecordFieldForm.h"

#include <string>

namespace abraflexitui {

// Schema-driven editor for an existing record: a generated RecordFieldForm
// (one input/checkbox per writable field, "Raw JSON" toggle for the rest).
// Loads `record show` and saves with `record update` (AbraFlexi PUT).
class RecordEditForm : public TDialog {
public:
    RecordEditForm(CliClient &client, std::string evidence, std::string id, std::string company = {});

    void handleEvent(TEvent &event) override;
    TColorAttr mapColor(uchar index) override;

private:
    void submit(bool dryRun);

    CliClient &client_;
    std::string evidence_;
    std::string id_;
    std::string company_;
    RecordFieldForm *form_ = nullptr;
};

} // namespace abraflexitui
