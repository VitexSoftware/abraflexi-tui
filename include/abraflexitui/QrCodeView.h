#pragma once

#include "abraflexitui/TV.h"

#include <string>

namespace abraflexitui {

// Modal QR of an AbraFlexi web address. Esc or the close icon dismisses it.
class QrCodeDialog : public TDialog {
public:
    explicit QrCodeDialog(const std::string &url);

    TColorAttr mapColor(uchar index) override;
};

} // namespace abraflexitui
