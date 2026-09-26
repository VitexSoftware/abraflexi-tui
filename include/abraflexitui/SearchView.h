#pragma once

#include "abraflexitui/TV.h"
#include "abraflexitui/CliClient.h"
#include "abraflexitui/SessionStore.h"
#include "abraflexitui/SimpleListViewer.h"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace abraflexitui {

// Search records in one evidence, or evidence names when the evidence field is empty.
class SearchView : public TDialog {
public:
    SearchView(CliClient &client, SessionStore &session);

    void suggestEvidences();
    void handleEvent(TEvent &event) override;
    TColorAttr mapColor(uchar index) override;

private:
    void runSearch();
    void openCurrent();
    void loadCatalogue();
    void acceptSuggestion();

    CliClient &client_;
    SessionStore &session_;
    TInputLine *evidence_;
    TInputLine *query_;
    SimpleListViewer *results_;
    std::vector<std::string> hitEvidence_;
    std::vector<std::string> hitId_;
    std::vector<nlohmann::json> catalogue_;
    bool catalogueLoaded_ = false;
    bool suggesting_ = false;
};

} // namespace abraflexitui
