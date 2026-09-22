#pragma once

#include "abraflexitui/TV.h"
#include "abraflexitui/CliClient.h"
#include "abraflexitui/SimpleListViewer.h"
#include "abraflexitui/RecordDetailView.h"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace abraflexitui {

class RecordListView;

// Row 0 of the underlying SimpleListViewer is a header line; records_[i]
// corresponds to displayed row i+1.
class RecordListBox : public SimpleListViewer {
public:
    RecordListBox(const TRect &bounds, TScrollBar *vScrollBar, RecordListView &ownerView) noexcept;

    void setRecords(std::vector<nlohmann::json> records, const std::vector<std::string> &columns);
    const nlohmann::json *selectedRecord() const;

    void focusItem(short item) override;
    void handleEvent(TEvent &event) override;

private:
    RecordListView &ownerView_;
    std::vector<nlohmann::json> records_;
};

// Generic, evidence-name-parameterized record browser: one window handles
// any AbraFlexi evidence via `abraflexi-cli record <evidence> list|show`,
// there is no per-evidence subclass.
class RecordListView : public TWindow {
public:
    RecordListView(CliClient &client, std::string evidence, std::string columns = "id,kod,nazev",
                    int limit = 20);

    void refresh();
    void onRowFocused(const nlohmann::json *record);
    void onRowActivated(const nlohmann::json *record);
    void changeBounds(const TRect &bounds) override;

    CliClient &client() { return client_; }
    const std::string &evidence() const { return evidence_; }

    void handleEvent(TEvent &event) override;

private:
    std::vector<std::string> currentColumns() const;
    void editSelected();
    void deleteSelected();
    void showFields();
    void openSelectedWindow();
    void placePanes();

    CliClient &client_;
    std::string evidence_;

    TInputLine *filterInput_;
    TInputLine *columnsInput_;
    TInputLine *limitInput_;
    TInputLine *orderInput_;

    RecordListBox *grid_ = nullptr;
    TStaticText *separator_ = nullptr;
    RecordDetailView *detail_ = nullptr;
};

} // namespace abraflexitui
