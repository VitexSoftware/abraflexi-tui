#include "abraflexitui/TV.h"
#include "abraflexitui/EvidenceInfoView.h"
#include "abraflexitui/JsonFormat.h"
#include "abraflexitui/RecordListView.h"
#include "abraflexitui/SimpleListViewer.h"
#include "abraflexitui/WindowColors.h"
#include "abraflexitui/WindowLayout.h"

#include <algorithm>
#include <vector>

namespace abraflexitui {

namespace {

std::string flag(const nlohmann::json &item, const char *key) {
    if (!item.is_object() || !item.contains(key) || !item.at(key).is_boolean()) {
        return " ";
    }

    return item.at(key).get<bool>() ? "Y" : " ";
}

bool isColumnSelected(RecordListView &recordList, const std::string &field) {
    const std::vector<std::string> columns = recordList.currentColumns();
    return std::find(columns.begin(), columns.end(), field) != columns.end();
}

// Structure list box for a RecordListView's "Info" button: lets the user
// add/remove a field to/from the owner's Columns input by pressing Enter or
// clicking its row.
class EvidenceStructureListBox : public SimpleListViewer {
public:
    EvidenceStructureListBox(const TRect &bounds, TScrollBar *vScrollBar, EvidenceInfoView &ownerView) noexcept
        : SimpleListViewer(bounds, vScrollBar), ownerView_(ownerView) {
    }

    void handleEvent(TEvent &event) override {
        const bool isMouseDown = event.what == evMouseDown;
        const bool isEnter = event.what == evKeyDown && event.keyDown.keyCode == kbEnter;

        SimpleListViewer::handleEvent(event);

        if (isMouseDown || isEnter) {
            ownerView_.toggleFocusedField(focused);
            clearEvent(event);
        }
    }

private:
    EvidenceInfoView &ownerView_;
};

} // namespace

EvidenceInfoView::EvidenceInfoView(CliClient &client, std::string evidence, std::string company,
                                    RecordListView *recordList)
    : TWindowInit(&TWindow::initFrame),
      TWindow(TRect(2, 1, 78, 23),
              ("Structure: " + evidence + " [" + (company.empty() ? client.company() : company) + "]").c_str(),
              wnNoNumber),
      company_(company.empty() ? client.company() : std::move(company)), recordList_(recordList) {
    options |= ofCentered | ofTileable;

    CliClient::Result result = client.runJsonForCompany({"record", evidence, "properties"}, company_);
    std::vector<std::string> rows;

    // Column rows are only toggleable when this view was opened from a
    // RecordListView's "Info" button; otherwise the indicator prefix is
    // omitted entirely and rowFields_ stays empty for every row.
    const std::string indicatorPrefix = recordList_ != nullptr ? "    " : "";

    if (!result.ok) {
        rows.push_back("Error: " + result.errorMessage);
        rowFields_.push_back(std::string());
    } else {
        rows.push_back(indicatorPrefix + fitColumn("Column", 22) + " " + fitColumn("Type", 12) + " M W  Title");
        rowFields_.push_back(std::string());

        if (result.data.contains("columns") && result.data.at("columns").is_array()) {
            for (const auto &column : result.data.at("columns")) {
                const std::string name = jsonField(column, "name");
                std::string prefix = indicatorPrefix;

                if (recordList_ != nullptr) {
                    prefix = std::string(isColumnSelected(*recordList_, name) ? "[Y] " : "[ ] ");
                }

                rows.push_back(prefix + fitColumn(name, 22) + " " + fitColumn(jsonField(column, "type"), 12) + " " +
                               flag(column, "mandatory") + " " + flag(column, "writable") + "  " +
                               jsonField(column, "title"));
                rowFields_.push_back(recordList_ != nullptr ? name : std::string());
            }
        }

        rows.push_back(std::string());
        rowFields_.push_back(std::string());
        rows.push_back("Relations");
        rowFields_.push_back(std::string());

        if (result.data.contains("relations") && result.data.at("relations").is_array()) {
            for (const auto &relation : result.data.at("relations")) {
                rows.push_back(fitColumn(jsonField(relation, "name"), 24) + " " +
                               fitColumn(jsonField(relation, "evidenceType"), 16) + " " + jsonField(relation, "url"));
                rowFields_.push_back(std::string());
            }
        }

        rows.push_back(std::string());
        rowFields_.push_back(std::string());
        rows.push_back("Labels");
        rowFields_.push_back(std::string());

        if (result.data.contains("labels") && result.data.at("labels").is_array()) {
            for (const auto &label : result.data.at("labels")) {
                rows.push_back(fitColumn(jsonField(label, "kod"), 16) + " " + jsonField(label, "nazev"));
                rowFields_.push_back(std::string());
            }
        }
    }

    TScrollBar *vBar = standardScrollBar(sbVertical | sbHandleKeyboard);
    EvidenceStructureListBox *list = new EvidenceStructureListBox(getExtent().grow(-1, -1), vBar, *this);
    growFill(list);
    list->setRows(std::move(rows));
    insert(list);
}

void EvidenceInfoView::toggleFocusedField(short item) {
    if (recordList_ == nullptr || item < 0 || static_cast<std::size_t>(item) >= rowFields_.size()) {
        return;
    }

    const std::string &field = rowFields_[static_cast<std::size_t>(item)];

    if (field.empty()) {
        return;
    }

    recordList_->toggleColumn(field);
    close();
}

TColorAttr EvidenceInfoView::mapColor(uchar index) {
    TColorAttr color;
    return windowColor(index, color) ? color : TWindow::mapColor(index);
}

} // namespace abraflexitui
