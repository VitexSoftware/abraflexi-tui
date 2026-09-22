#include "abraflexitui/TV.h"
#include "abraflexitui/AppButton.h"
#include "abraflexitui/RecordListView.h"
#include "abraflexitui/RecordCreateForm.h"
#include "abraflexitui/RecordEditForm.h"
#include "abraflexitui/EvidenceInfoView.h"
#include "abraflexitui/DocumentPreview.h"
#include "abraflexitui/Commands.h"
#include "abraflexitui/JsonFormat.h"
#include "abraflexitui/WindowLayout.h"

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

RecordListBox::RecordListBox(const TRect &bounds, TScrollBar *vScrollBar, RecordListView &ownerView) noexcept
    : SimpleListViewer(bounds, vScrollBar), ownerView_(ownerView) {
}

void RecordListBox::setRecords(std::vector<nlohmann::json> records, const std::vector<std::string> &columns,
                                const std::map<std::string, std::string> &titles) {
    records_ = std::move(records);

    std::vector<std::string> rows;
    std::string header;

    for (std::size_t i = 0; i < columns.size(); ++i) {
        auto title = titles.find(columns[i]);
        header += fitColumn(title != titles.end() && !title->second.empty() ? title->second : columns[i], 18);

        if (i + 1 < columns.size()) {
            header += " ";
        }
    }

    rows.push_back(header);

    for (const auto &rec : records_) {
        std::string line;

        for (std::size_t i = 0; i < columns.size(); ++i) {
            line += fitColumn(jsonField(rec, columns[i].c_str()), 18);

            if (i + 1 < columns.size()) {
                line += " ";
            }
        }

        rows.push_back(line);
    }

    if (records_.empty()) {
        rows.push_back("(no records found)");
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
        if (jsonField(records_[i], "id") == id) {
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
                                const WindowBounds *initialBounds, std::string company)
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

    std::vector<std::string> summaryColumns = EvidenceSchema::summaryNames(schema_);

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
        "F5=Refresh  Enter=Show  F4=Preview  F2=Fields  Esc=Close"));
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
    columnsInput_ = new TInputLine(TRect(x + 8, y, x + 42, y + 1), 200);
    setInputText(columnsInput_, columns);
    insert(columnsInput_);
    insert(new TStaticText(TRect(x + 44, y, x + 51, y + 1), "Order:"));
    orderInput_ = new TInputLine(TRect(x + 51, y, x + 70, y + 1), 40);
    insert(orderInput_);
    y += 1;

    insert(new AppButton(TRect(x, y, x + 12, y + 2), "~R~efresh", cmRecordRefresh, bfNormal));
    insert(new AppButton(TRect(x + 13, y, x + 22, y + 2), "~N~ew", cmRecordCreateNew, bfNormal));
    insert(new AppButton(TRect(x + 23, y, x + 32, y + 2), "Edi~t~", cmRecordEdit, bfNormal));
    insert(new AppButton(TRect(x + 33, y, x + 44, y + 2), "~D~elete", cmRecordDelete, bfNormal));
    insert(new AppButton(TRect(x + 45, y, x + 56, y + 2), "~I~nfo", cmShowEvidenceInfo, bfNormal));
    insert(new AppButton(TRect(x + 57, y, x + 70, y + 2), "~P~review", cmOpenRecordWindow, bfNormal));
    y += 2;

    short bottom = inner.b.y;
    short gridHeight = static_cast<short>((bottom - y) * 3 / 5);
    short gridBottom = y + (gridHeight > 3 ? gridHeight : 3);

    TScrollBar *gridScroll = standardScrollBar(sbVertical | sbHandleKeyboard);
    grid_ = new RecordListBox(TRect(x, y, right, gridBottom), gridScroll, *this);
    insert(grid_);

    separator_ = new TStaticText(TRect(x, gridBottom, right, gridBottom + 1), std::string(right - x, '\xC4').c_str());
    insert(separator_);

    TScrollBar *detailScroll = standardScrollBar(sbVertical | sbHandleKeyboard);
    detail_ = new RecordDetailView(TRect(x, gridBottom + 1, right, bottom), detailScroll);
    insert(detail_);
    detail_->showMessage("(select a record above)");
    placePanes();
    refresh();
    // Keyboard focus starts directly on the record grid (not the Filter/
    // Columns/Limit/Order fields above it) so Up/Down/PgUp/PgDn and Enter
    // work immediately via TListViewer's own built-in key handling.
    grid_->select();
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
    const short y = static_cast<short>(inner.a.y + 5);
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

    TRect gridRect(inner.a.x, y, static_cast<short>(inner.b.x - 1), gridBottom);
    grid_->locate(gridRect);

    if (grid_->vScrollBar != nullptr) {
        TRect bar(static_cast<short>(inner.b.x - 1), y, inner.b.x, gridBottom);
        grid_->vScrollBar->locate(bar);
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

std::vector<std::string> RecordListView::currentColumns() const {
    return splitColumns(columnsInput_->data);
}

void RecordListView::refresh() {
    std::vector<std::string> columns = currentColumns();

    std::vector<std::string> args = {"record", evidence_, "list",
                                      std::string("--columns=") + columnsInput_->data,
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
    grid_->setRecords(std::move(records), columns, fieldTitles_);
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
        session_.updateFocused(sessionHandle_, jsonField(*record, "id"));
    }
}

void RecordListView::onRowActivated(const nlohmann::json *record) {
    if (record == nullptr) {
        return;
    }

    std::string id = jsonField(*record, "id");

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

    const std::string id = jsonField(*record, "id");

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

    const std::string id = jsonField(*record, "id");

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

    const std::string id = jsonField(*record, "id");

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
    TProgram::deskTop->insert(new EvidenceInfoView(client_, evidence_, company_));
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

        default:
            break;
        }
    } else if (event.what == evKeyDown) {
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

} // namespace abraflexitui
