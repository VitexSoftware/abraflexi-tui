#pragma once

#include "abraflexitui/TV.h"
#include "abraflexitui/CliClient.h"
#include "abraflexitui/SimpleListViewer.h"

#include <string>
#include <vector>

namespace abraflexitui {

class MutableStaticText;

// Flexplorer Changes API screen: enable/disable and webhook register/remove.
class ChangesView : public TDialog {
public:
    explicit ChangesView(CliClient &client);

    void handleEvent(TEvent &event) override;

private:
    void reload();
    void setEnabled(bool enabled);
    void registerHook();
    void unregisterHook();

    CliClient &client_;
    TInputLine *url_;
    SimpleListViewer *hooks_;
    MutableStaticText *status_;
    std::vector<std::string> hookIds_;
};

} // namespace abraflexitui
