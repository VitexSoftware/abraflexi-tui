#include "abraflexitui/TV.h"
#include "abraflexitui/AppButton.h"
#include "abraflexitui/RelationPickerDialog.h"
#include "abraflexitui/Commands.h"
#include "abraflexitui/EvidenceSchema.h"
#include "abraflexitui/JsonFormat.h"
#include "abraflexitui/TextFold.h"
#include "abraflexitui/WindowColors.h"

#include <algorithm>
#include <cstring>

namespace abraflexitui {

namespace {

class SimpleStringListBox : public TListViewer {
public:
    SimpleStringListBox(const TRect &bounds, TScrollBar *bar) noexcept : TListViewer(bounds, 1, nullptr, bar) {
    }

    void setItems(const std::vector<std::string> &items) {
        items_ = &items;
        setRange(static_cast<short>(items_->size()));
        drawView();
    }

    void getText(char *dest, short item, short maxLen) override {
        if (items_ != nullptr && item >= 0 && static_cast<std::size_t>(item) < items_->size()) {
            std::strncpy(dest, (*items_)[static_cast<std::size_t>(item)].c_str(), static_cast<std::size_t>(maxLen));
            dest[maxLen] = '\0';
        } else {
            dest[0] = '\0';
        }
    }

private:
    const std::vector<std::string> *items_ = nullptr;
};

class QueryLine : public TInputLine {
public:
    QueryLine(const TRect &bounds, RelationPickerDialog &owner) noexcept
        : TInputLine(bounds, 80), owner_(owner) {
    }

    void handleEvent(TEvent &event) override {
        TInputLine::handleEvent(event);

        if (data != nullptr && std::string(data) != seen_) {
            seen_ = data;
            owner_.applyQuery(seen_);
        }
    }

    void remember(const std::string &text) {
        seen_ = text;
    }

private:
    RelationPickerDialog &owner_;
    std::string seen_;
};

} // namespace

RelationPickerDialog::RelationPickerDialog(CliClient &client, std::string relationEvidence, std::string company)
    : TWindowInit(&TDialog::initFrame),
      TDialog(TRect(0, 0, 72, 20), ("Select: " + relationEvidence).c_str()),
      client_(client), relationEvidence_(std::move(relationEvidence)),
      company_(company.empty() ? client.company() : std::move(company)) {
    options |= ofCentered;

    short x = 2;
    short y = 2;
    short right = 70;

    insert(new TStaticText(TRect(x, y, right, y + 1), "Type to narrow, then Select."));
    y += 1;

    insert(new TStaticText(TRect(x, y, x + 6, y + 1), "Find:"));
    queryInput_ = new QueryLine(TRect(x + 6, y, right, y + 1), *this);
    insert(queryInput_);
    y += 1;

    bar_ = standardScrollBar(sbVertical | sbHandleKeyboard);
    auto *box = new SimpleStringListBox(TRect(x, y, right, static_cast<short>(y + 12)), bar_);
    insert(bar_);
    insert(box);
    list_ = box;

    y += 13;

    insert(new AppButton(TRect(x, y, static_cast<short>(x + 16), static_cast<short>(y + 2)), "~S~elect",
                          cmRelationPickerSelect, bfDefault));
    insert(new AppButton(TRect(static_cast<short>(x + 18), y, static_cast<short>(x + 30), static_cast<short>(y + 2)),
                          "Cancel", cmCancel, bfNormal));

    loadItems();
    queryInput_->select();
}

void RelationPickerDialog::loadItems() {
    const std::vector<FieldSchema> &schema = EvidenceSchema::fetch(client_, relationEvidence_, company_);
    std::vector<std::string> columns = EvidenceSchema::summaryNames(schema);

    if (columns.empty()) {
        columns = {"kod", "nazev"};
    }

    for (const char *required : {"id", "kod"}) {
        if (std::find(columns.begin(), columns.end(), required) == columns.end()) {
            columns.push_back(required);
        }
    }

    std::string columnsJoined;

    for (std::size_t i = 0; i < columns.size(); ++i) {
        columnsJoined += columns[i];

        if (i + 1 < columns.size()) {
            columnsJoined += ",";
        }
    }

    CliClient::Result result = client_.runJsonForCompany(
        {"record", relationEvidence_, "list", "--columns=" + columnsJoined, "--limit=200"}, company_);

    allRecords_.clear();
    allTexts_.clear();

    if (result.ok && result.data.is_array()) {
        for (const auto &rec : result.data) {
            std::string text;

            for (const auto &col : columns) {
                if (col == "id" || col == "kod") {
                    continue;
                }

                std::string value = jsonField(rec, col.c_str(), EvidenceSchema::fieldByName(schema, col));

                if (!value.empty()) {
                    text += (text.empty() ? "" : "  ") + value;
                }
            }

            if (text.empty()) {
                text = recordIdentifier(rec);
            }

            allRecords_.push_back(rec);
            allTexts_.push_back(text);
        }
    }

    query_.clear();
    applyQuery(std::string());
}

void RelationPickerDialog::applyQuery(const std::string &query) {
    query_ = query;
    records_.clear();
    rowTexts_.clear();

    for (std::size_t i = 0; i < allTexts_.size() && i < allRecords_.size(); ++i) {
        if (foldedContains(allTexts_[i], query_)) {
            records_.push_back(allRecords_[i]);
            rowTexts_.push_back(allTexts_[i]);
        }
    }

    if (rowTexts_.empty()) {
        rowTexts_.push_back(allTexts_.empty() ? "(no records)" : "(no match)");
    }

    static_cast<SimpleStringListBox *>(list_)->setItems(rowTexts_);
}

void RelationPickerDialog::setQueryText(const std::string &query) {
    if (queryInput_ == nullptr) {
        return;
    }

    std::strncpy(queryInput_->data, query.c_str(), static_cast<std::size_t>(queryInput_->maxLen));
    queryInput_->data[queryInput_->maxLen] = '\0';
    const int length = static_cast<int>(std::strlen(queryInput_->data));
    queryInput_->curPos = length;
    queryInput_->selStart = length;
    queryInput_->selEnd = length;
    queryInput_->firstPos = 0;
    static_cast<QueryLine *>(queryInput_)->remember(query);
    queryInput_->drawView();
    applyQuery(query);
}

nlohmann::json RelationPickerDialog::selectedValue() const {
    nlohmann::json value;

    if (list_ == nullptr) {
        return value;
    }

    const short focused = list_->focused;

    if (focused < 0 || static_cast<std::size_t>(focused) >= records_.size()) {
        return value;
    }

    const std::string ident = recordIdentifier(records_[static_cast<std::size_t>(focused)]);
    value["ref"] = "/c/" + company_ + "/" + relationEvidence_ + "/" + ident;
    value["showAs"] = rowTexts_[static_cast<std::size_t>(focused)];
    return value;
}

void RelationPickerDialog::handleEvent(TEvent &event) {
    TDialog::handleEvent(event);

    if (event.what == evKeyDown && current == list_ && list_ != nullptr) {
        if (event.keyDown.keyCode == kbBack) {
            std::string next = query_;
            popUtf8(next);
            setQueryText(next);
            clearEvent(event);
            return;
        }

        const TStringView typed = event.keyDown.getText();

        if (typed.size() > 0 && event.keyDown.text[0] >= 32) {
            setQueryText(query_ + std::string(typed.data(), typed.size()));
            clearEvent(event);
            return;
        }
    }

    if (event.what == evKeyDown && current == queryInput_ && list_ != nullptr) {
        short target = list_->focused;
        bool moved = true;

        switch (event.keyDown.keyCode) {
        case kbDown:
            target = static_cast<short>(target + 1);
            break;
        case kbUp:
            target = static_cast<short>(target - 1);
            break;
        case kbPgDn:
            target = static_cast<short>(target + (list_->size.y > 0 ? list_->size.y : 1));
            break;
        case kbPgUp:
            target = static_cast<short>(target - (list_->size.y > 0 ? list_->size.y : 1));
            break;
        default:
            moved = false;
            break;
        }

        if (moved) {
            list_->focusItemNum(target);
            clearEvent(event);
            return;
        }
    }

    if (event.what == evBroadcast && event.message.command == cmListItemSelected && event.message.infoPtr == list_) {
        if (selectedValue().is_object()) {
            endModal(cmOK);
        }

        clearEvent(event);
        return;
    }

    if (event.what != evCommand) {
        return;
    }

    if (event.message.command == cmRelationPickerSelect) {
        if (selectedValue().is_object()) {
            endModal(cmOK);
        }

        clearEvent(event);
    }
}

TColorAttr RelationPickerDialog::mapColor(uchar index) {
    TColorAttr color;
    return windowColor(index, color) ? color : TDialog::mapColor(index);
}

} // namespace abraflexitui
