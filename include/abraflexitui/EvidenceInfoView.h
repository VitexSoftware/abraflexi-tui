#pragma once

#include "abraflexitui/TV.h"
#include "abraflexitui/CliClient.h"

#include <string>

namespace abraflexitui {

// Flexplorer evidence Info tab: columns, relations and labels.
class EvidenceInfoView : public TWindow {
public:
    EvidenceInfoView(CliClient &client, std::string evidence, std::string company = {});

private:
    std::string company_;
};

} // namespace abraflexitui
