#pragma once

#include "abraflexitui/TV.h"
#include "abraflexitui/CliClient.h"

#include <string>

namespace abraflexitui {

// JSON editor for an existing record. Loads `record show` and saves with
// `record update` (AbraFlexi PUT).
class RecordEditForm : public TDialog {
public:
    RecordEditForm(CliClient &client, std::string evidence, std::string id);

    void handleEvent(TEvent &event) override;

private:
    std::string readEditorText() const;
    void submit(bool dryRun);

    CliClient &client_;
    std::string evidence_;
    std::string id_;
    TMemo *editor_;
};

} // namespace abraflexitui
