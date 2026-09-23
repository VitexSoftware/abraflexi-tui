#pragma once

#include "abraflexitui/TV.h"
#include "abraflexitui/CliClient.h"
#include "abraflexitui/SimpleListViewer.h"

#include <functional>
#include <string>
#include <vector>

namespace abraflexitui {

// Lists companies. Enter or a double-click chooses one for later requests.
class CompanyListView : public TWindow {
public:
    CompanyListView(CliClient &client, std::function<void(const std::string &)> onChosen);

    TColorAttr mapColor(uchar index) override;

private:
    void chooseFocused();

    SimpleListViewer *list_;
    std::vector<std::string> dbNames_;
    std::function<void(const std::string &)> onChosen_;
};

} // namespace abraflexitui
