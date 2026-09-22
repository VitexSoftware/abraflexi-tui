#pragma once

#include "abraflexitui/TV.h"
#include "abraflexitui/CliClient.h"
#include "abraflexitui/SimpleListViewer.h"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace abraflexitui {

// Enter/double-click opens the record list. F2 opens the evidence structure
// (columns, relations, labels), the Flexplorer Info tab.
class EvidenceListBox : public SimpleListViewer {
public:
    EvidenceListBox(const TRect &bounds, TScrollBar *vScrollBar, std::vector<nlohmann::json> evidences) noexcept;

    void applyFilter(const std::string &query);
    void activateFocused();

    void handleEvent(TEvent &event) override;

private:
    void show(const std::vector<nlohmann::json> &rows);

    std::vector<nlohmann::json> all_;
    std::vector<nlohmann::json> evidences_;
    // Row 0 is a header line; evidences_[i] corresponds to row i+1.
};

class EvidenceListView : public TWindow {
public:
    explicit EvidenceListView(CliClient &client);

    void applyQuery(const std::string &query);
    void changeBounds(const TRect &bounds) override;
    void handleEvent(TEvent &event) override;

private:
    void openSelected(unsigned short command, nlohmann::json *evidence);
    void placeList();
    void setQueryText(const std::string &query);

    CliClient &client_;
    TInputLine *queryInput_ = nullptr;
    EvidenceListBox *list_ = nullptr;
    std::string query_;
};

} // namespace abraflexitui
