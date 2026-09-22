#pragma once

#include "abraflexitui/TV.h"
#include "abraflexitui/CliClient.h"
#include "abraflexitui/SimpleListViewer.h"

#include <nlohmann/json.hpp>

namespace abraflexitui {

// Enter/double-click opens the record list. F2 opens the evidence structure
// (columns, relations, labels), the Flexplorer Info tab.
class EvidenceListBox : public SimpleListViewer {
public:
    EvidenceListBox(const TRect &bounds, TScrollBar *vScrollBar, std::vector<nlohmann::json> evidences) noexcept;

    void handleEvent(TEvent &event) override;

private:
    std::vector<nlohmann::json> evidences_;
    // Row 0 is a header line; evidences_[i] corresponds to row i+1.
};

class EvidenceListView : public TWindow {
public:
    explicit EvidenceListView(CliClient &client);

    void handleEvent(TEvent &event) override;

private:
    void openSelected(unsigned short command, nlohmann::json *evidence);

    CliClient &client_;
    EvidenceListBox *list_;
};

} // namespace abraflexitui
