#include "abraflexitui/TV.h"
#include "abraflexitui/EvidenceInfoView.h"
#include "abraflexitui/JsonFormat.h"
#include "abraflexitui/RecordListView.h"
#include "abraflexitui/SimpleListViewer.h"
#include "abraflexitui/TextFold.h"
#include "abraflexitui/WindowColors.h"
#include "abraflexitui/WindowLayout.h"

#include <cstring>

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

class QueryLine : public TInputLine {
public:
    QueryLine(const TRect &bounds, EvidenceInfoView &owner) noexcept : TInputLine(bounds, 80), owner_(owner) {
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
    EvidenceInfoView &owner_;
    std::string seen_;
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

    // Column rows are only toggleable when this view was opened from a
    // RecordListView's "Info" button; otherwise the indicator prefix is
    // omitted entirely and rowFields_ stays empty for every row.
    const std::string indicatorPrefix = recordList_ != nullptr ? "    " : "";

    if (!result.ok) {
        allRows_.push_back("Error: " + result.errorMessage);
        allFields_.push_back(std::string());
    } else {
        allRows_.push_back(indicatorPrefix + fitColumn("Column", 22) + " " + fitColumn("Type", 12) + " M W  Title");
        allFields_.push_back(std::string());

        if (result.data.contains("columns") && result.data.at("columns").is_array()) {
            for (const auto &column : result.data.at("columns")) {
                const std::string name = jsonField(column, "name");
                std::string prefix = indicatorPrefix;

                if (recordList_ != nullptr) {
                    prefix = std::string(isColumnSelected(*recordList_, name) ? "[Y] " : "[ ] ");
                }

                allRows_.push_back(prefix + fitColumn(name, 22) + " " + fitColumn(jsonField(column, "type"), 12) + " " +
                                   flag(column, "mandatory") + " " + flag(column, "writable") + "  " +
                                   jsonField(column, "title"));
                allFields_.push_back(recordList_ != nullptr ? name : std::string());
            }
        }

        allRows_.push_back(std::string());
        allFields_.push_back(std::string());
        allRows_.push_back("Relations");
        allFields_.push_back(std::string());

        if (result.data.contains("relations") && result.data.at("relations").is_array()) {
            for (const auto &relation : result.data.at("relations")) {
                allRows_.push_back(fitColumn(jsonField(relation, "name"), 24) + " " +
                                   fitColumn(jsonField(relation, "evidenceType"), 16) + " " + jsonField(relation, "url"));
                allFields_.push_back(std::string());
            }
        }

        allRows_.push_back(std::string());
        allFields_.push_back(std::string());
        allRows_.push_back("Labels");
        allFields_.push_back(std::string());

        if (result.data.contains("labels") && result.data.at("labels").is_array()) {
            for (const auto &label : result.data.at("labels")) {
                allRows_.push_back(fitColumn(jsonField(label, "kod"), 16) + " " + jsonField(label, "nazev"));
                allFields_.push_back(std::string());
            }
        }
    }

    TRect inner = getExtent().grow(-1, -1);
    insert(new TStaticText(TRect(inner.a.x, inner.a.y, inner.a.x + 6, inner.a.y + 1), "Find:"));
    queryInput_ = new QueryLine(TRect(inner.a.x + 6, inner.a.y, inner.b.x, inner.a.y + 1), *this);
    growWide(queryInput_);
    insert(queryInput_);

    TScrollBar *vBar = standardScrollBar(sbVertical | sbHandleKeyboard);
    list_ = new EvidenceStructureListBox(TRect(inner.a.x, inner.a.y + 1, inner.b.x, inner.b.y), vBar, *this);
    insert(list_);
    placeList();
    applyQuery(std::string());
    queryInput_->select();
}

void EvidenceInfoView::applyQuery(const std::string &query) {
    query_ = query;
    std::vector<std::string> rows;
    rowFields_.clear();

    for (std::size_t i = 0; i < allRows_.size() && i < allFields_.size(); ++i) {
        if (query_.empty() || foldedContains(allRows_[i], query_)) {
            rows.push_back(allRows_[i]);
            rowFields_.push_back(allFields_[i]);
        }
    }

    if (rows.empty()) {
        rows.push_back("(no match)");
        rowFields_.push_back(std::string());
    }

    list_->setRows(std::move(rows));
}

void EvidenceInfoView::setQueryText(const std::string &query) {
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

void EvidenceInfoView::placeList() {
    if (list_ == nullptr) {
        return;
    }

    TRect inner = getExtent().grow(-1, -1);

    if (queryInput_ != nullptr) {
        TRect field(inner.a.x + 6, inner.a.y, inner.b.x, inner.a.y + 1);
        queryInput_->locate(field);
    }

    if (inner.b.y - inner.a.y < 3) {
        return;
    }

    TRect grid(inner.a.x, inner.a.y + 1, static_cast<short>(inner.b.x - 1), inner.b.y);
    list_->locate(grid);

    if (list_->vScrollBar != nullptr) {
        TRect bar(static_cast<short>(inner.b.x - 1), inner.a.y + 1, inner.b.x, inner.b.y);
        list_->vScrollBar->locate(bar);
    }
}

void EvidenceInfoView::changeBounds(const TRect &bounds) {
    TWindow::changeBounds(bounds);
    placeList();
}

void EvidenceInfoView::handleEvent(TEvent &event) {
    TWindow::handleEvent(event);

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
        }
    }

    if (event.what == evKeyDown && current == queryInput_ && list_ != nullptr) {
        if (event.keyDown.keyCode == kbEnter) {
            toggleFocusedField(list_->focused);
            clearEvent(event);
            return;
        }

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
        }
    }
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
