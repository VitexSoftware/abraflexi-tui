#include "abraflexitui/TV.h"
#include "abraflexitui/AppButton.h"
#include "abraflexitui/RecordListView.h"
#include "abraflexitui/RecordCreateForm.h"
#include "abraflexitui/RecordEditForm.h"
#include "abraflexitui/EvidenceInfoView.h"
#include "abraflexitui/DocumentPreview.h"
#include "abraflexitui/PrintDialog.h"
#include "abraflexitui/DownloadDialog.h"
#include "abraflexitui/Commands.h"
#include "abraflexitui/JsonFormat.h"
#include "abraflexitui/TextFold.h"
#include "abraflexitui/WindowColors.h"
#include "abraflexitui/WindowLayout.h"

#include <algorithm>
#include <cstring>
#include <sstream>

namespace abraflexitui {

namespace {

std::vector<std::string> splitColumns(const std::string &columns) {
    std::vector<std::string> out;
    std::stringstream ss(columns);
    std::string item;

    while (std::getline(ss, item, ',')) {
        // Trim whitespace.
        std::size_t start = item.find_first_not_of(" \t");
        std::size_t end = item.find_last_not_of(" \t");

        if (start != std::string::npos) {
            out.push_back(item.substr(start, end - start + 1));
        }
    }

    if (out.empty()) {
        out.push_back("id");
    }

    return out;
}

void setInputText(TInputLine *input, const std::string &text) {
    std::strncpy(input->data, text.c_str(), static_cast<std::size_t>(input->maxLen));
    input->data[input->maxLen] = '\0';
}

class FindLine : public TInputLine {
public:
    FindLine(const TRect &bounds, RecordListView &owner) noexcept : TInputLine(bounds, 80), owner_(owner) {
    }

    void handleEvent(TEvent &event) override {
        TInputLine::handleEvent(event);

        if (data != nullptr && std::string(data) != seen_) {
            seen_ = data;
            owner_.applyFind(seen_);
        }
    }

    void remember(const std::string &text) {
        seen_ = text;
    }

private:
    RecordListView &owner_;
    std::string seen_;
};

class FieldLine : public TInputLine {
public:
    FieldLine(const TRect &bounds, RecordListView &owner, bool sortableOnly, int maxLen) noexcept
        : TInputLine(bounds, maxLen), owner_(owner), sortableOnly_(sortableOnly) {
    }

    void handleEvent(TEvent &event) override {
        TInputLine::handleEvent(event);

        if (data != nullptr && std::string(data) != seen_) {
            seen_ = data;
            owner_.suggestFields(seen_, sortableOnly_);
        }
    }

    void remember(const std::string &text) {
        seen_ = text;
    }

private:
    RecordListView &owner_;
    std::string seen_;
    bool sortableOnly_;
};

std::string lastFieldToken(const std::string &text) {
    const std::size_t comma = text.rfind(',');
    std::string token = comma == std::string::npos ? text : text.substr(comma + 1);
    const std::size_t at = token.find('@');

    if (at != std::string::npos) {
        token.resize(at);
    }

    const std::size_t start = token.find_first_not_of(" \t");

    if (start == std::string::npos) {
        return std::string();
    }

    const std::size_t end = token.find_last_not_of(" \t");
    return token.substr(start, end - start + 1);
}

TRect initialWindowRect(const WindowBounds *bounds) {
    if (bounds != nullptr) {
        return TRect(static_cast<short>(bounds->x1), static_cast<short>(bounds->y1),
                     static_cast<short>(bounds->x2), static_cast<short>(bounds->y2));
    }

    return TProgram::deskTop->getExtent();
}

WindowBounds toWindowBounds(const TRect &rect) {
    WindowBounds bounds;
    bounds.x1 = rect.a.x;
    bounds.y1 = rect.a.y;
    bounds.x2 = rect.b.x;
    bounds.y2 = rect.b.y;
    return bounds;
}

} // namespace

RecordListBox::RecordListBox(const TRect &bounds, TScrollBar *vScrollBar, TScrollBar *hScrollBar,
                              RecordListView &ownerView) noexcept
    : SimpleListViewer(bounds, vScrollBar, hScrollBar), ownerView_(ownerView) {
}

void RecordListBox::setRecords(std::vector<nlohmann::json> records, const std::vector<std::string> &columns,
                                const std::map<std::string, std::string> &titles,
                                const std::vector<FieldSchema> *schema) {
    allRecords_ = std::move(records);
    header_.clear();
    allLines_.clear();

    for (std::size_t i = 0; i < columns.size(); ++i) {
        auto title = titles.find(columns[i]);
        header_ += fitColumn(title != titles.end() && !title->second.empty() ? title->second : columns[i], 18);

        if (i + 1 < columns.size()) {
            header_ += " ";
        }
    }

    for (const auto &rec : allRecords_) {
        std::string line;

        for (std::size_t i = 0; i < columns.size(); ++i) {
            const FieldSchema *field = schema != nullptr ? EvidenceSchema::fieldByName(*schema, columns[i]) : nullptr;
            line += fitColumn(jsonField(rec, columns[i].c_str(), field), 18);

            if (i + 1 < columns.size()) {
                line += " ";
            }
        }

        allLines_.push_back(std::move(line));
    }

    applyFilter(filter_);
}

void RecordListBox::applyFilter(const std::string &query) {
    filter_ = query;
    records_.clear();
    std::vector<std::string> rows;
    rows.push_back(header_);

    for (std::size_t i = 0; i < allLines_.size() && i < allRecords_.size(); ++i) {
        if (foldedContains(allLines_[i], filter_)) {
            records_.push_back(allRecords_[i]);
            rows.push_back(allLines_[i]);
        }
    }

    if (records_.empty()) {
        rows.push_back(allRecords_.empty() ? "(no records found)" : "(no match)");
    }

    setRows(std::move(rows));
}

const nlohmann::json *RecordListBox::selectedRecord() const {
    if (focused >= 1 && static_cast<std::size_t>(focused - 1) < records_.size()) {
        return &records_[static_cast<std::size_t>(focused - 1)];
    }

    return nullptr;
}

short RecordListBox::rowForId(const std::string &id) const {
    if (id.empty()) {
        return -1;
    }

    for (std::size_t i = 0; i < records_.size(); ++i) {
        if (recordIdentifier(records_[i]) == id) {
            return static_cast<short>(i + 1);
        }
    }

    return -1;
}

void RecordListBox::focusItem(short item) {
    SimpleListViewer::focusItem(item);
    ownerView_.onRowFocused(selectedRecord());
}

void RecordListBox::handleEvent(TEvent &event) {
    if ((event.what == evMouseDown && (event.mouse.eventFlags & meDoubleClick)) ||
        (event.what == evKeyDown && event.keyDown.keyCode == kbEnter)) {
        ownerView_.onRowActivated(selectedRecord());
        clearEvent(event);
        return;
    }

    SimpleListViewer::handleEvent(event);
}

RecordListView::RecordListView(CliClient &client, SessionStore &session, std::string evidence,
                                std::string columns, int limit, std::string initialFocusId,
                                const WindowBounds *initialBounds, std::string company,
                                bool preferExplicitColumns)
    : TWindowInit(&TWindow::initFrame),
      TWindow(initialWindowRect(initialBounds),
              ("Records: " + evidence + " [" + (company.empty() ? client.company() : company) + "]").c_str(),
              wnNoNumber),
      client_(client), session_(session), evidence_(std::move(evidence)),
      company_(company.empty() ? client.company() : std::move(company)),
      pendingFocusId_(std::move(initialFocusId)) {
    sessionHandle_ = session_.openWindow(evidence_, company_, pendingFocusId_, initialBounds);
    options |= ofTileable;
    growMode = gfGrowHiX | gfGrowHiY;

    schema_ = EvidenceSchema::fetch(client_, evidence_, company_);

    for (const auto &field : schema_) {
        if (!field.title.empty()) {
            fieldTitles_[field.name] = field.title;
        }
    }

    std::vector<std::string> summaryColumns = preferExplicitColumns ? std::vector<std::string>() : EvidenceSchema::summaryNames(schema_);

    if (!summaryColumns.empty()) {
        std::string joined;

        for (std::size_t i = 0; i < summaryColumns.size(); ++i) {
            joined += summaryColumns[i];

            if (i + 1 < summaryColumns.size()) {
                joined += ",";
            }
        }

        columns = joined;
    }

    TRect inner = getExtent();
    inner.grow(-1, -1);

    short x = inner.a.x;
    short y = inner.a.y;
    short right = inner.b.x;

    insert(new TStaticText(
        TRect(x, y, right, y + 1),
        "F5=Refresh  Enter=Show  F4=Preview  F2=Fields  Alt+P=Print  Alt+D=Download  Esc=Close"));
    y += 1;

    insert(new TStaticText(TRect(x, y, x + 6, y + 1), "Find:"));
    findInput_ = new FindLine(TRect(x + 6, y, right, y + 1), *this);
    growWide(findInput_);
    insert(findInput_);
    y += 1;

    insert(new TStaticText(TRect(x, y, x + 7, y + 1), "Filter:"));
    filterInput_ = new TInputLine(TRect(x + 7, y, x + 42, y + 1), 200);
    insert(filterInput_);
    insert(new TStaticText(TRect(x + 44, y, x + 51, y + 1), "Limit:"));
    limitInput_ = new TInputLine(TRect(x + 51, y, x + 58, y + 1), 6);
    setInputText(limitInput_, std::to_string(limit));
    insert(limitInput_);
    y += 1;

    insert(new TStaticText(TRect(x, y, x + 8, y + 1), "Columns:"));
    columnsInput_ = new FieldLine(TRect(x + 8, y, x + 42, y + 1), *this, false, 200);
    setInputText(columnsInput_, columns);
    static_cast<FieldLine *>(columnsInput_)->remember(columns);
    insert(columnsInput_);
    insert(new TStaticText(TRect(x + 44, y, x + 51, y + 1), "Order:"));
    orderInput_ = new FieldLine(TRect(x + 51, y, x + 70, y + 1), *this, true, 40);
    static_cast<FieldLine *>(orderInput_)->remember(std::string());
    insert(orderInput_);
    y += 1;

    // TGroup never fills a group's own background - each child view only
    // paints its own bounds, so the 1-cell gaps between the toolbar buttons
    // below are otherwise never explicitly repainted and can be left
    // showing stale content from whatever was last drawn there (reported:
    // patches of the wrong color appearing in those gaps after an
    // overlapping dialog closed). A blank TStaticText spanning the whole
    // button row, inserted (and so drawn) before the buttons, guarantees
    // every gap cell gets this dialog's own background painted on every
    // redraw; the buttons drawn after it cover their own cells as usual.
    insert(new TStaticText(TRect(x, y, right, static_cast<short>(y + 2)), ""));

    insert(new AppButton(TRect(x, y, x + 10, y + 2), "~R~efresh", cmRecordRefresh, bfNormal));
    insert(new AppButton(TRect(x + 11, y, x + 19, y + 2), "~N~ew", cmRecordCreateNew, bfNormal));
    insert(new AppButton(TRect(x + 20, y, x + 28, y + 2), "Edi~t~", cmRecordEdit, bfNormal));
    insert(new AppButton(TRect(x + 29, y, x + 38, y + 2), "~D~elete", cmRecordDelete, bfNormal));
    insert(new AppButton(TRect(x + 39, y, x + 48, y + 2), "~I~nfo", cmShowEvidenceInfo, bfNormal));
    insert(new AppButton(TRect(x + 49, y, x + 60, y + 2), "~P~review", cmOpenRecordWindow, bfNormal));
    insert(new AppButton(TRect(x + 61, y, x + 71, y + 2), "Prin~t~", cmRecordPrint, bfNormal));
    insert(new AppButton(TRect(x + 72, y, x + 84, y + 2), "D~o~wnload", cmRecordDownload, bfNormal));
    y += 2;

    short bottom = inner.b.y;
    short gridHeight = static_cast<short>((bottom - y) * 3 / 5);
    short gridBottom = y + (gridHeight > 3 ? gridHeight : 3);

    TScrollBar *gridScroll = standardScrollBar(sbVertical | sbHandleKeyboard);
    gridHScroll_ = standardScrollBar(sbHorizontal | sbHandleKeyboard);
    grid_ = new RecordListBox(TRect(x, y, right, gridBottom), gridScroll, gridHScroll_, *this);
    insert(grid_);

    separator_ = new TStaticText(TRect(x, gridBottom, right, gridBottom + 1), std::string(right - x, '\xC4').c_str());
    insert(separator_);

    TScrollBar *detailScroll = standardScrollBar(sbVertical | sbHandleKeyboard);
    detail_ = new RecordDetailView(TRect(x, gridBottom + 1, right, bottom), detailScroll);
    insert(detail_);
    detail_->showMessage("(select a record above)");
    placePanes();
    refresh();
    // Find stays focused so typing narrows the loaded rows. Up/Down still move
    // the grid (see handleEvent).
    findInput_->select();
}

void RecordListView::changeBounds(const TRect &bounds) {
    TWindow::changeBounds(bounds);
    session_.updateBounds(sessionHandle_, toWindowBounds(bounds));

    if (size.y >= 8) {
        placePanes();
    }
}

RecordListView::~RecordListView() {
    session_.closeWindow(sessionHandle_);
}

void RecordListView::placePanes() {
    if (grid_ == nullptr || detail_ == nullptr) {
        return;
    }

    TRect inner = getExtent();
    inner.grow(-1, -1);
    const short y = static_cast<short>(inner.a.y + 6);
    const short bottom = inner.b.y;

    if (bottom - y < 6) {
        return;
    }

    short gridHeight = static_cast<short>((bottom - y) * 3 / 5);

    if (gridHeight < 3) {
        gridHeight = 3;
    }

    short gridBottom = static_cast<short>(y + gridHeight);

    if (gridBottom > bottom - 3) {
        gridBottom = static_cast<short>(bottom - 3);
    }

    // The grid's own last row is reserved for the horizontal scrollbar (per
    // the requested UX: a bar at the bottom of the grid, steered with
    // Left/Right) rather than growing the window - it only becomes visible
    // (via TScrollBar::show(), driven by its range in
    // SimpleListViewer::updateHScrollRange()) once the selected columns
    // don't fit the dialog's width.
    const short listBottom = static_cast<short>(gridBottom - 1);

    TRect gridRect(inner.a.x, y, static_cast<short>(inner.b.x - 1), listBottom);
    grid_->locate(gridRect);

    if (grid_->vScrollBar != nullptr) {
        TRect bar(static_cast<short>(inner.b.x - 1), y, inner.b.x, listBottom);
        grid_->vScrollBar->locate(bar);
    }

    if (gridHScroll_ != nullptr) {
        TRect hbar(inner.a.x, listBottom, static_cast<short>(inner.b.x - 1), gridBottom);
        gridHScroll_->locate(hbar);
    }

    if (separator_ != nullptr) {
        TRect line(inner.a.x, gridBottom, inner.b.x, static_cast<short>(gridBottom + 1));
        separator_->locate(line);
    }

    TRect detailRect(inner.a.x, static_cast<short>(gridBottom + 1), static_cast<short>(inner.b.x - 1), bottom);
    detail_->locate(detailRect);

    if (detail_->vScrollBar != nullptr) {
        TRect bar(static_cast<short>(inner.b.x - 1), static_cast<short>(gridBottom + 1), inner.b.x, bottom);
        detail_->vScrollBar->locate(bar);
    }
}

void RecordListView::applyFind(const std::string &query) {
    if (query == find_ || grid_ == nullptr) {
        return;
    }

    find_ = query;
    grid_->applyFilter(find_);
}

void RecordListView::suggestFields(const std::string &text, bool sortableOnly) {
    if (detail_ == nullptr) {
        return;
    }

    const std::string token = lastFieldToken(text);

    if (token.empty()) {
        onRowFocused(grid_ != nullptr ? grid_->selectedRecord() : nullptr);
        return;
    }

    std::string line = sortableOnly ? "Order: " : "Fields: ";
    int shown = 0;

    for (const auto &field : schema_) {
        if (sortableOnly && !field.sortable) {
            continue;
        }

        if (!foldedContains(field.name + " " + field.title, token)) {
            continue;
        }

        if (shown != 0) {
            line += ", ";
        }

        line += field.name;

        if (!field.title.empty() && field.title != field.name) {
            line += " (" + field.title + ")";
        }

        if (++shown == 8) {
            break;
        }
    }

    if (shown == 0) {
        detail_->showMessage(sortableOnly ? "Order: (no matching field)" : "Fields: (no matching field)");
        return;
    }

    detail_->showMessage(line);
}

namespace {

void setFindLine(TInputLine *input, const std::string &text) {
    std::strncpy(input->data, text.c_str(), static_cast<std::size_t>(input->maxLen));
    input->data[input->maxLen] = '\0';
    const int length = static_cast<int>(std::strlen(input->data));
    input->curPos = length;
    input->selStart = length;
    input->selEnd = length;
    input->firstPos = 0;
    static_cast<FindLine *>(input)->remember(text);
    input->drawView();
}

} // namespace

std::vector<std::string> RecordListView::currentColumns() const {
    return splitColumns(columnsInput_->data);
}

void RecordListView::toggleColumn(const std::string &field) {
    std::vector<std::string> columns = currentColumns();
    auto it = std::find(columns.begin(), columns.end(), field);

    if (it != columns.end()) {
        columns.erase(it);
    } else {
        columns.push_back(field);
    }

    std::string joined;

    for (std::size_t i = 0; i < columns.size(); ++i) {
        joined += columns[i];

        if (i + 1 < columns.size()) {
            joined += ",";
        }
    }

    setInputText(columnsInput_, joined);
    columnsInput_->drawView();
    refresh();
}

void RecordListView::refresh() {
    std::vector<std::string> columns = currentColumns();

    // The record's "id" (and "kod", used as a fallback identifier - see
    // recordIdentifier()) must always be fetched so every row can be
    // identified for show/edit/delete/print, even when the user's chosen
    // display columns don't include them - the displayed columns_ passed to
    // setRecords() below are left untouched.
    std::vector<std::string> fetchColumns = columns;

    for (const char *required : {"id", "kod"}) {
        if (std::find(fetchColumns.begin(), fetchColumns.end(), required) == fetchColumns.end()) {
            fetchColumns.push_back(required);
        }
    }

    std::string fetchColumnsJoined;

    for (std::size_t i = 0; i < fetchColumns.size(); ++i) {
        fetchColumnsJoined += fetchColumns[i];

        if (i + 1 < fetchColumns.size()) {
            fetchColumnsJoined += ",";
        }
    }

    std::vector<std::string> args = {"record", evidence_, "list",
                                      std::string("--columns=") + fetchColumnsJoined,
                                      std::string("--limit=") + limitInput_->data};

    if (filterInput_->data[0] != '\0') {
        args.push_back(std::string("--filter=") + filterInput_->data);
    }

    if (orderInput_->data[0] != '\0') {
        args.push_back(std::string("--order=") + orderInput_->data);
    }

    CliClient::Result result = client_.runJsonForCompany(args, company_);

    if (!result.ok) {
        grid_->setRecords({}, columns);
        detail_->showMessage("Error: " + result.errorMessage);
        return;
    }

    std::vector<nlohmann::json> records;

    if (result.data.is_array()) {
        for (const auto &rec : result.data) {
            records.push_back(rec);
        }
    }

    // setRecords() -> setRows() focuses the first real row and drives
    // RecordListBox::focusItem() -> onRowFocused(), which already shows that
    // record (or the "(select a record above)" placeholder when the result
    // is empty) - no need to reset the detail pane here afterwards.
    grid_->setRecords(std::move(records), columns, fieldTitles_, &schema_);
    applyPendingFocus();
}

void RecordListView::applyPendingFocus() {
    // Only relevant right after construction, when this window is being
    // restored from a previous session with a remembered selected record -
    // re-applying it on every manual Refresh would fight the user's own
    // navigation, so it is consumed (cleared) after the first attempt
    // whether or not a matching row was actually found.
    if (pendingFocusId_.empty()) {
        return;
    }

    const std::string id = std::move(pendingFocusId_);
    pendingFocusId_.clear();
    const short row = grid_->rowForId(id);

    if (row >= 1) {
        grid_->focusItemNum(row);
    }
}

void RecordListView::onRowFocused(const nlohmann::json *record) {
    if (record == nullptr) {
        detail_->showMessage("(select a record above)");
        session_.updateFocused(sessionHandle_, std::string());
    } else {
        detail_->showRecord(*record, &schema_);
        session_.updateFocused(sessionHandle_, recordIdentifier(*record));
    }
}

void RecordListView::onRowActivated(const nlohmann::json *record) {
    if (record == nullptr) {
        return;
    }

    std::string id = recordIdentifier(*record);

    if (id.empty()) {
        detail_->showRecord(*record);
        return;
    }

    CliClient::Result result = client_.runJsonForCompany({"record", evidence_, "show", id}, company_);

    if (!result.ok) {
        detail_->showMessage("Error: " + result.errorMessage);
        return;
    }

    detail_->showRecord(result.data, &schema_);
}

void RecordListView::openSelectedWindow() {
    const nlohmann::json *record = grid_->selectedRecord();

    if (record == nullptr) {
        messageBox("Select a record first.", mfError | mfOKButton);
        return;
    }

    const std::string id = recordIdentifier(*record);

    if (id.empty()) {
        messageBox("The selected row has no id.", mfError | mfOKButton);
        return;
    }

    struct Find {
        std::string evidence;
        std::string id;
        TWindow *found = nullptr;
    } find{evidence_, id, nullptr};

    TProgram::deskTop->forEach(
        [](TView *view, void *arg) {
            auto *seek = static_cast<Find *>(arg);
            auto *preview = dynamic_cast<DocumentPreview *>(view);
            auto *window = dynamic_cast<RecordWindow *>(view);

            if (preview != nullptr && preview->evidence() == seek->evidence && preview->recordId() == seek->id) {
                seek->found = preview;
            } else if (window != nullptr && window->evidence() == seek->evidence && window->recordId() == seek->id) {
                seek->found = window;
            }
        },
        &find);

    if (find.found != nullptr) {
        find.found->select();
        return;
    }

    CliClient::Result result = client_.runJsonForCompany({"record", evidence_, "show", id}, company_);

    if (!result.ok) {
        messageBox(result.errorMessage.empty() ? std::string("Could not open the record") : result.errorMessage,
                   mfError | mfOKButton);
        return;
    }

    if (evidenceHasItems(evidence_)) {
        TProgram::deskTop->insert(new DocumentPreview(client_, evidence_, id, result.data, company_));
    } else {
        TProgram::deskTop->insert(new RecordWindow(client_, evidence_, id, result.data, company_));
    }
}

void RecordListView::editSelected() {
    const nlohmann::json *record = grid_->selectedRecord();

    if (record == nullptr) {
        messageBox("Select a record first.", mfError | mfOKButton);
        return;
    }

    const std::string id = recordIdentifier(*record);

    if (id.empty()) {
        messageBox("The selected row has no id.", mfError | mfOKButton);
        return;
    }

    RecordEditForm *form = new RecordEditForm(client_, evidence_, id, company_);

    if (TProgram::application->executeDialog(form) == cmOK) {
        refresh();
    }
}

void RecordListView::deleteSelected() {
    const nlohmann::json *record = grid_->selectedRecord();

    if (record == nullptr) {
        messageBox("Select a record first.", mfError | mfOKButton);
        return;
    }

    const std::string id = recordIdentifier(*record);

    if (id.empty()) {
        messageBox("The selected row has no id.", mfError | mfOKButton);
        return;
    }

    if (messageBox("Delete " + evidence_ + " " + id + "?", mfConfirmation | mfYesButton | mfNoButton) != cmYes) {
        return;
    }

    CliClient::Result result = client_.runJsonForCompany({"record", evidence_, "delete", id}, company_);

    if (!result.ok) {
        messageBox(result.errorMessage.empty() ? std::string("Delete failed") : result.errorMessage, mfError | mfOKButton);
        return;
    }

    refresh();
}

void RecordListView::showFields() {
    TProgram::deskTop->insert(new EvidenceInfoView(client_, evidence_, company_, this));
}

void RecordListView::printSelected() {
    const nlohmann::json *record = grid_->selectedRecord();

    if (record == nullptr) {
        messageBox("Select a record first.", mfError | mfOKButton);
        return;
    }

    const std::string id = recordIdentifier(*record);

    if (id.empty()) {
        messageBox("The selected row has no id.", mfError | mfOKButton);
        return;
    }

    PrintDialog *dlg = new PrintDialog(client_, evidence_, id, company_);
    TProgram::application->executeDialog(dlg);
}

void RecordListView::downloadSelected() {
    const nlohmann::json *record = grid_->selectedRecord();

    if (record == nullptr) {
        messageBox("Select a record first.", mfError | mfOKButton);
        return;
    }

    const std::string id = recordIdentifier(*record);

    if (id.empty()) {
        messageBox("The selected row has no id.", mfError | mfOKButton);
        return;
    }

    DownloadDialog *dlg = new DownloadDialog(client_, evidence_, id, company_);
    TProgram::application->executeDialog(dlg);
}

void RecordListView::handleEvent(TEvent &event) {
    TWindow::handleEvent(event);

    if (event.what == evCommand) {
        switch (event.message.command) {
        case cmRecordRefresh:
            refresh();
            clearEvent(event);
            break;

        case cmRecordCreateNew: {
            RecordCreateForm *form = new RecordCreateForm(client_, evidence_, company_);
            ushort closedWith = TProgram::application->executeDialog(form);

            if (closedWith == cmOK) {
                refresh();
            }

            clearEvent(event);
            break;
        }

        case cmRecordEdit:
            editSelected();
            clearEvent(event);
            break;

        case cmRecordDelete:
            deleteSelected();
            clearEvent(event);
            break;

        case cmShowEvidenceInfo:
            showFields();
            clearEvent(event);
            break;

        case cmOpenRecordWindow:
            openSelectedWindow();
            clearEvent(event);
            break;

        case cmRecordPrint:
            printSelected();
            clearEvent(event);
            break;

        case cmRecordDownload:
            downloadSelected();
            clearEvent(event);
            break;

        default:
            break;
        }
    } else if (event.what == evKeyDown && current == grid_ && grid_ != nullptr) {
        if (event.keyDown.keyCode == kbBack) {
            std::string next = find_;
            popUtf8(next);
            setFindLine(findInput_, next);
            applyFind(next);
            clearEvent(event);
        } else {
            const TStringView typed = event.keyDown.getText();

            if (typed.size() > 0 && event.keyDown.text[0] >= 32) {
                const std::string next = find_ + std::string(typed.data(), typed.size());
                setFindLine(findInput_, next);
                applyFind(next);
                clearEvent(event);
            }
        }
    } else if (event.what == evKeyDown && current == findInput_ && grid_ != nullptr) {
        if (event.keyDown.keyCode == kbEnter) {
            onRowActivated(grid_->selectedRecord());
            clearEvent(event);
        } else {
            short target = grid_->focused;
            bool moved = true;

            switch (event.keyDown.keyCode) {
            case kbDown:
                target = static_cast<short>(target + 1);
                break;
            case kbUp:
                target = static_cast<short>(target - 1);
                break;
            case kbPgDn:
                target = static_cast<short>(target + (grid_->size.y > 0 ? grid_->size.y : 1));
                break;
            case kbPgUp:
                target = static_cast<short>(target - (grid_->size.y > 0 ? grid_->size.y : 1));
                break;
            default:
                moved = false;
                break;
            }

            if (moved) {
                if (target < 1 && grid_->rowCount() > 1) {
                    target = 1;
                }

                grid_->focusItemNum(target);
                clearEvent(event);
            }
        }
    } else if (event.what == evKeyDown) {
        if (event.keyDown.keyCode == kbAltP) {
            printSelected();
            clearEvent(event);
        } else
        if (event.keyDown.keyCode == kbAltD) {
            downloadSelected();
            clearEvent(event);
        } else
        if (event.keyDown.keyCode == kbF5) {
            refresh();
            clearEvent(event);
        } else if (event.keyDown.keyCode == kbF2) {
            showFields();
            clearEvent(event);
        } else if (event.keyDown.keyCode == kbF4) {
            openSelectedWindow();
            clearEvent(event);
        }
    }
}

TColorAttr RecordListView::mapColor(uchar index) {
    TColorAttr color;
    return windowColor(index, color) ? color : TWindow::mapColor(index);
}

} // namespace abraflexitui
