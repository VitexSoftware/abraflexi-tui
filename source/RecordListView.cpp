#include "abraflexitui/TV.h"
#include "abraflexitui/RecordListView.h"
#include "abraflexitui/RecordCreateForm.h"
#include "abraflexitui/RecordEditForm.h"
#include "abraflexitui/EvidenceInfoView.h"
#include "abraflexitui/Commands.h"
#include "abraflexitui/JsonFormat.h"

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

} // namespace

RecordListBox::RecordListBox(const TRect &bounds, TScrollBar *vScrollBar, RecordListView &ownerView) noexcept
    : SimpleListViewer(bounds, vScrollBar), ownerView_(ownerView) {
}

void RecordListBox::setRecords(std::vector<nlohmann::json> records, const std::vector<std::string> &columns) {
    records_ = std::move(records);

    std::vector<std::string> rows;
    std::string header;

    for (std::size_t i = 0; i < columns.size(); ++i) {
        header += fitColumn(columns[i], 18);

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

RecordListView::RecordListView(CliClient &client, std::string evidence, std::string columns, int limit)
    : TWindowInit(&TWindow::initFrame),
      TWindow(TProgram::deskTop->getExtent(), ("Records: " + evidence).c_str(), wnNoNumber),
      client_(client), evidence_(std::move(evidence)) {
    options |= ofCentered;
    growMode = gfGrowHiX | gfGrowHiY;

    TRect inner = getExtent();
    inner.grow(-1, -1);

    short x = inner.a.x;
    short y = inner.a.y;
    short right = inner.b.x;

    insert(new TStaticText(
        TRect(x, y, right, y + 1),
        "F5=Refresh  Enter=Show  F2=Fields  Esc=Close"));
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

    insert(new TButton(TRect(x, y, x + 12, y + 2), "~R~efresh", cmRecordRefresh, bfNormal));
    insert(new TButton(TRect(x + 13, y, x + 22, y + 2), "~N~ew", cmRecordCreateNew, bfNormal));
    insert(new TButton(TRect(x + 23, y, x + 32, y + 2), "Ed~i~t", cmRecordEdit, bfNormal));
    insert(new TButton(TRect(x + 33, y, x + 44, y + 2), "~D~elete", cmRecordDelete, bfNormal));
    insert(new TButton(TRect(x + 45, y, x + 56, y + 2), "~I~nfo", cmShowEvidenceInfo, bfNormal));
    y += 2;

    short bottom = inner.b.y;
    short gridHeight = static_cast<short>((bottom - y) * 3 / 5);
    short gridBottom = y + (gridHeight > 3 ? gridHeight : 3);

    TScrollBar *gridScroll = standardScrollBar(sbVertical | sbHandleKeyboard);
    grid_ = new RecordListBox(TRect(x, y, right, gridBottom), gridScroll, *this);
    insert(grid_);

    insert(new TStaticText(TRect(x, gridBottom, right, gridBottom + 1), std::string(right - x, '\xC4').c_str()));

    TScrollBar *detailScroll = standardScrollBar(sbVertical | sbHandleKeyboard);
    detail_ = new RecordDetailView(TRect(x, gridBottom + 1, right, bottom), detailScroll);
    insert(detail_);
    detail_->showMessage("(select a record above)");

    refresh();
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

    CliClient::Result result = client_.runJson(args);

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

    grid_->setRecords(std::move(records), columns);
    detail_->showMessage("(select a record above)");
}

void RecordListView::onRowFocused(const nlohmann::json *record) {
    if (record == nullptr) {
        detail_->showMessage("(select a record above)");
    } else {
        detail_->showRecord(*record);
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

    CliClient::Result result = client_.runJson({"record", evidence_, "show", id});

    if (!result.ok) {
        detail_->showMessage("Error: " + result.errorMessage);
        return;
    }

    detail_->showRecord(result.data);
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

    RecordEditForm *form = new RecordEditForm(client_, evidence_, id);

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

    CliClient::Result result = client_.runJson({"record", evidence_, "delete", id});

    if (!result.ok) {
        messageBox(result.errorMessage.empty() ? std::string("Delete failed") : result.errorMessage, mfError | mfOKButton);
        return;
    }

    refresh();
}

void RecordListView::showFields() {
    TProgram::deskTop->insert(new EvidenceInfoView(client_, evidence_));
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
            RecordCreateForm *form = new RecordCreateForm(client_, evidence_);
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
        }
    }
}

} // namespace abraflexitui
