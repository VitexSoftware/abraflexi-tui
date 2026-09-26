#include "abraflexitui/TV.h"
#include "abraflexitui/AppButton.h"
#include "abraflexitui/RelationPickerDialog.h"
#include "abraflexitui/Commands.h"
#include "abraflexitui/EvidenceSchema.h"
#include "abraflexitui/JsonFormat.h"
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

    insert(new TStaticText(TRect(x, y, right, y + 1), "Pick a value, then Select."));
    y += 1;

    bar_ = standardScrollBar(sbVertical | sbHandleKeyboard);
    auto *box = new SimpleStringListBox(TRect(x, y, right, static_cast<short>(y + 13)), bar_);
    insert(bar_);
    insert(box);
    list_ = box;

    y += 14;

    insert(new AppButton(TRect(x, y, static_cast<short>(x + 16), static_cast<short>(y + 2)), "~S~elect",
                          cmRelationPickerSelect, bfDefault));
    insert(new AppButton(TRect(static_cast<short>(x + 18), y, static_cast<short>(x + 30), static_cast<short>(y + 2)),
                          "Cancel", cmCancel, bfNormal));

    loadItems();
    selectNext(False);
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

    records_.clear();
    rowTexts_.clear();

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

            records_.push_back(rec);
            rowTexts_.push_back(text);
        }
    }

    static_cast<SimpleStringListBox *>(list_)->setItems(rowTexts_);
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

    if (event.what == evBroadcast && event.message.command == cmListItemSelected && event.message.infoPtr == list_) {
        endModal(cmOK);
        clearEvent(event);
        return;
    }

    if (event.what != evCommand) {
        return;
    }

    if (event.message.command == cmRelationPickerSelect) {
        endModal(cmOK);
        clearEvent(event);
    }
}

TColorAttr RelationPickerDialog::mapColor(uchar index) {
    TColorAttr color;
    return windowColor(index, color) ? color : TDialog::mapColor(index);
}

} // namespace abraflexitui
