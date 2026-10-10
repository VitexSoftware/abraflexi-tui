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

    void applyQuery(const std::string &query);
    void handleEvent(TEvent &event) override;
    void changeBounds(const TRect &bounds) override;
    TColorAttr mapColor(uchar index) override;

private:
    void chooseFocused();
    void setQueryText(const std::string &query);
    void placeList();

    SimpleListViewer *list_ = nullptr;
    TInputLine *queryInput_ = nullptr;
    std::vector<std::string> allRows_;
    std::vector<std::string> allDbNames_;
    std::vector<std::string> dbNames_;
    std::string query_;
    std::function<void(const std::string &)> onChosen_;
};

} // namespace abraflexitui
