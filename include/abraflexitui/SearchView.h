#pragma once

#include "abraflexitui/TV.h"
#include "abraflexitui/CliClient.h"
#include "abraflexitui/SessionStore.h"
#include "abraflexitui/SimpleListViewer.h"

#include <string>
#include <vector>

namespace abraflexitui {

// Search records in one evidence, or evidence names when the evidence field is empty.
class SearchView : public TDialog {
public:
    SearchView(CliClient &client, SessionStore &session);

    void handleEvent(TEvent &event) override;

private:
    void runSearch();
    void openCurrent();

    CliClient &client_;
    SessionStore &session_;
    TInputLine *evidence_;
    TInputLine *query_;
    SimpleListViewer *results_;
    std::vector<std::string> hitEvidence_;
    std::vector<std::string> hitId_;
};

} // namespace abraflexitui
