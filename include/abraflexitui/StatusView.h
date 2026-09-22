#pragma once

#include "abraflexitui/TV.h"
#include "abraflexitui/CliClient.h"

namespace abraflexitui {

// Modal dialog showing `abraflexi-cli status --format=json`.
class StatusView : public TDialog {
public:
    explicit StatusView(CliClient &client);

    TColorAttr mapColor(uchar index) override;

private:
    enum class Signal { Ok, BadCredentials, Unreachable };

    Signal signal_ = Signal::Unreachable;
};

} // namespace abraflexitui
