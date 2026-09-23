#pragma once

#include "abraflexitui/TV.h"
#include "abraflexitui/CliClient.h"
#include "abraflexitui/EvidenceSchema.h"
#include "abraflexitui/SessionStore.h"
#include "abraflexitui/SimpleListViewer.h"
#include "abraflexitui/RecordDetailView.h"

#include <map>
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

    // `titles` maps a column's raw property name to its schema title
    // (EvidenceSchema); a column missing from it prints its raw name, same
    // as when `titles` is left empty (no schema available for this
    // evidence).
    void setRecords(std::vector<nlohmann::json> records, const std::vector<std::string> &columns,
                     const std::map<std::string, std::string> &titles = {});
    const nlohmann::json *selectedRecord() const;
    // Row index (matching focusItemNum()'s convention, i.e. 1-based since
    // row 0 is the header) of the record whose "id" field equals `id`, or
    // -1 if there is no such record in the current result set.
    short rowForId(const std::string &id) const;

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
    // `preferExplicitColumns`: when true, `columns` is kept as given even if
    // the evidence's schema marks its own set of fields "inSummary" - used by
    // menu shortcuts that want a specific, evidence-appropriate column list
    // instead of whatever the schema would otherwise substitute.
    RecordListView(CliClient &client, SessionStore &session, std::string evidence,
                    std::string columns = "id,kod,nazev", int limit = 20, std::string initialFocusId = {},
                    const WindowBounds *initialBounds = nullptr, std::string company = {},
                    bool preferExplicitColumns = false);
    ~RecordListView() override;

    void refresh();
    void onRowFocused(const nlohmann::json *record);
    void onRowActivated(const nlohmann::json *record);
    void changeBounds(const TRect &bounds) override;

    CliClient &client() { return client_; }
    const std::string &evidence() const { return evidence_; }
    const std::string &company() const { return company_; }

    // Used by EvidenceInfoView (opened via the "Info" button) so it can show
    // which of the evidence's fields are currently listed in the Columns
    // input, and add/remove a field there when the user toggles it.
    std::vector<std::string> currentColumns() const;
    void toggleColumn(const std::string &field);

    void handleEvent(TEvent &event) override;
    TColorAttr mapColor(uchar index) override;

private:
    void editSelected();
    void deleteSelected();
    void showFields();
    void printSelected();
    void openSelectedWindow();
    void placePanes();
    void applyPendingFocus();

    CliClient &client_;
    SessionStore &session_;
    std::string evidence_;
    std::string company_;
    std::vector<FieldSchema> schema_;
    std::map<std::string, std::string> fieldTitles_;
    int sessionHandle_ = 0;
    std::string pendingFocusId_;

    TInputLine *filterInput_;
    TInputLine *columnsInput_;
    TInputLine *limitInput_;
    TInputLine *orderInput_;

    RecordListBox *grid_ = nullptr;
    TStaticText *separator_ = nullptr;
    RecordDetailView *detail_ = nullptr;
};

} // namespace abraflexitui
