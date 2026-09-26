#pragma once

#include "abraflexitui/TV.h"
#include "abraflexitui/CliClient.h"
#include "abraflexitui/RecordFieldForm.h"

#include <string>

namespace abraflexitui {

// Schema-driven record-creation UI: a generated RecordFieldForm (one
// input/checkbox per writable field, mandatory ones marked with "*"), with
// a "Raw JSON" toggle for anything the schema doesn't cover well. Missing
// mandatory fields are still ultimately validated server-side too
// (missingFieldsFormatted on a failed create), which "Force" skips.
class RecordCreateForm : public TDialog {
public:
    RecordCreateForm(CliClient &client, std::string evidence, std::string company = {});

    void handleEvent(TEvent &event) override;
    TColorAttr mapColor(uchar index) override;

private:
    void submit(bool dryRun);

    CliClient &client_;
    std::string evidence_;
    std::string company_;
    RecordFieldForm *form_ = nullptr;
    TCheckBoxes *forceBox_;
};

} // namespace abraflexitui
