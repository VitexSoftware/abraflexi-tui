#pragma once

#include "abraflexitui/TV.h"
#include "abraflexitui/CliClient.h"

#include <string>

namespace abraflexitui {

// v1 record-creation UI: a raw JSON payload editor, not a dynamic per-field
// form (that would need a schema-introspection command abraflexi-cli does
// not expose yet). The mandatory-field hint is sourced directly from the
// CLI's own "no data provided" failure payload (PropertiesHelper output),
// so this dialog does not duplicate any AbraFlexi schema logic in C++.
class RecordCreateForm : public TDialog {
public:
    RecordCreateForm(CliClient &client, std::string evidence);

    void handleEvent(TEvent &event) override;

private:
    std::string readEditorText() const;
    void submit(bool dryRun);

    CliClient &client_;
    std::string evidence_;
    TMemo *editor_;
    TCheckBoxes *forceBox_;
};

} // namespace abraflexitui
