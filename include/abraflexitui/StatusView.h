#pragma once

#include "abraflexitui/TV.h"
#include "abraflexitui/CliClient.h"

namespace abraflexitui {

// Modal dialog showing `abraflexi-cli status --format=json`.
class StatusView : public TDialog {
public:
    explicit StatusView(CliClient &client);
};

} // namespace abraflexitui
