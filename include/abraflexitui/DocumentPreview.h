#pragma once

#include "abraflexitui/TV.h"
#include "abraflexitui/CliClient.h"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace abraflexitui {

// Doklady expose polozkyDokladu. An address-book row exposes kontakty.
bool evidenceHasItems(const std::string &evidence);

// Read-only document window: a short header and the line-item listing.
// Filter and sort are dialogs, so two of these windows can sit side by side
// without edit controls.
class DocumentPreview : public TWindow {
public:
    DocumentPreview(CliClient &client, std::string evidence, std::string id, const nlohmann::json &record,
                    std::string company = {});

    const std::string &evidence() const { return evidence_; }
    const std::string &recordId() const { return id_; }
    const std::string &company() const { return company_; }

    void changeBounds(const TRect &bounds) override;
    void handleEvent(TEvent &event) override;
    TColorAttr mapColor(uchar index) override;

private:
    void placeItems();
    void reloadItems();
    void askFilter();
    void askSort();
    void showStatus();

    CliClient &client_;
    std::string evidence_;
    std::string id_;
    std::string company_;
    std::string filter_;
    std::string order_;
    std::string relation_;
    std::string itemsLabel_;
    std::vector<std::string> columns_;

    class ItemList;
    class StatusLine;
    ItemList *items_ = nullptr;
    StatusLine *status_ = nullptr;
};

} // namespace abraflexitui
