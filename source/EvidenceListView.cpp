#include "abraflexitui/TV.h"
#include "abraflexitui/EvidenceListView.h"
#include "abraflexitui/Commands.h"
#include "abraflexitui/JsonFormat.h"
#include "abraflexitui/RecordListView.h"
#include "abraflexitui/EvidenceInfoView.h"
#include "abraflexitui/WindowLayout.h"

#include <cctype>
#include <cstring>
#include <string>
#include <vector>

namespace abraflexitui {

namespace {

std::string foldChar(unsigned code) {
    switch (code) {
    case 0x00E1:
    case 0x00C1:
        return "a";
    case 0x010D:
    case 0x010C:
        return "c";
    case 0x010F:
    case 0x010E:
        return "d";
    case 0x00E9:
    case 0x00C9:
    case 0x011B:
    case 0x011A:
        return "e";
    case 0x00ED:
    case 0x00CD:
        return "i";
    case 0x0148:
    case 0x0147:
        return "n";
    case 0x00F3:
    case 0x00D3:
        return "o";
    case 0x0159:
    case 0x0158:
        return "r";
    case 0x0161:
    case 0x0160:
        return "s";
    case 0x0165:
    case 0x0164:
        return "t";
    case 0x00FA:
    case 0x00DA:
    case 0x016F:
    case 0x016E:
        return "u";
    case 0x00FD:
    case 0x00DD:
        return "y";
    case 0x017E:
    case 0x017D:
        return "z";
    default:
        return std::string();
    }
}

std::string fold(const std::string &text) {
    std::string out;

    for (std::size_t i = 0; i < text.size();) {
        const auto c = static_cast<unsigned char>(text[i]);

        if (c < 0x80) {
            out.push_back(static_cast<char>(std::tolower(c)));
            ++i;
            continue;
        }

        if ((c & 0xE0) == 0xC0 && i + 1 < text.size()) {
            const unsigned code = (static_cast<unsigned>(c & 0x1F) << 6) |
                                  (static_cast<unsigned char>(text[i + 1]) & 0x3F);
            const std::string folded = foldChar(code);

            if (!folded.empty()) {
                out += folded;
            }

            i += 2;
            continue;
        }

        out.push_back(static_cast<char>(c));
        ++i;
    }

    return out;
}

std::string evidenceLine(const nlohmann::json &evidence) {
    return fitColumn(jsonField(evidence, "path"), 24) + " " + fitColumn(jsonField(evidence, "name"), 32) + " " +
           jsonField(evidence, "description");
}

class QueryLine : public TInputLine {
public:
    QueryLine(const TRect &bounds, EvidenceListView &owner) noexcept
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
    EvidenceListView &owner_;
    std::string seen_;
};

} // namespace

EvidenceListBox::EvidenceListBox(const TRect &bounds, TScrollBar *vScrollBar,
                                  std::vector<nlohmann::json> evidences) noexcept
    : SimpleListViewer(bounds, vScrollBar), all_(std::move(evidences)) {
    applyFilter(std::string());
}

void EvidenceListBox::show(const std::vector<nlohmann::json> &rows) {
    evidences_ = rows;
    std::vector<std::string> lines;
    lines.push_back(fitColumn("Path", 24) + " " + fitColumn("Name", 32) + " " + "Description");

    for (const auto &evidence : evidences_) {
        lines.push_back(evidenceLine(evidence));
    }

    if (evidences_.empty()) {
        lines.push_back("(no match)");
    }

    setRows(std::move(lines));
}

void EvidenceListBox::applyFilter(const std::string &query) {
    const std::string needle = fold(query);
    std::vector<nlohmann::json> matched;

    for (const auto &evidence : all_) {
        const std::string haystack = fold(jsonField(evidence, "path") + " " + jsonField(evidence, "name") + " " +
                                           jsonField(evidence, "description"));

        if (needle.empty() || haystack.find(needle) != std::string::npos) {
            matched.push_back(evidence);
        }
    }

    show(std::move(matched));
}

void EvidenceListBox::activateFocused() {
    if (focused >= 1 && static_cast<std::size_t>(focused - 1) < evidences_.size()) {
        nlohmann::json selected = evidences_[static_cast<std::size_t>(focused - 1)];
        message(owner, evCommand, cmOpenRecordList, &selected);
    }
}

void EvidenceListBox::handleEvent(TEvent &event) {
    if ((event.what == evMouseDown && (event.mouse.eventFlags & meDoubleClick)) ||
        (event.what == evKeyDown && event.keyDown.keyCode == kbEnter)) {
        activateFocused();
        clearEvent(event);
        return;
    }

    if (event.what == evKeyDown && event.keyDown.keyCode == kbF2) {
        if (focused >= 1 && static_cast<std::size_t>(focused - 1) < evidences_.size()) {
            nlohmann::json selected = evidences_[static_cast<std::size_t>(focused - 1)];
            message(owner, evCommand, cmShowEvidenceInfo, &selected);
        }

        clearEvent(event);
        return;
    }

    SimpleListViewer::handleEvent(event);
}

EvidenceListView::EvidenceListView(CliClient &client)
    : TWindowInit(&TWindow::initFrame),
      TWindow(TRect(1, 1, 79, 22), "Evidences", wnNoNumber),
      client_(client) {
    options |= ofCentered | ofTileable;

    CliClient::Result result = client.runJson({"list-evidences"});
    std::vector<nlohmann::json> evidences;

    if (result.ok && result.data.is_array()) {
        for (const auto &ev : result.data) {
            evidences.push_back(ev);
        }
    }

    TRect inner = getExtent().grow(-1, -1);
    insert(new TStaticText(TRect(inner.a.x, inner.a.y, inner.a.x + 6, inner.a.y + 1), "Find:"));
    queryInput_ = new QueryLine(TRect(inner.a.x + 6, inner.a.y, inner.b.x, inner.a.y + 1), *this);
    growWide(queryInput_);
    insert(queryInput_);

    TScrollBar *vBar = standardScrollBar(sbVertical | sbHandleKeyboard);
    list_ = new EvidenceListBox(TRect(inner.a.x, inner.a.y + 1, inner.b.x, inner.b.y), vBar, std::move(evidences));
    growFill(list_);
    insert(list_);
    placeList();

    if (!result.ok) {
        list_->setRows({std::string("Error: ") + result.errorMessage});
    }

    queryInput_->select();
}

void EvidenceListView::applyQuery(const std::string &query) {
    if (query == query_ || list_ == nullptr) {
        return;
    }

    query_ = query;
    list_->applyFilter(query_);
}

void EvidenceListView::setQueryText(const std::string &query) {
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

void EvidenceListView::placeList() {
    if (list_ == nullptr || queryInput_ == nullptr) {
        return;
    }

    TRect inner = getExtent().grow(-1, -1);
    TRect field(inner.a.x + 6, inner.a.y, inner.b.x, inner.a.y + 1);
    queryInput_->locate(field);

    if (inner.b.y - inner.a.y < 3) {
        return;
    }

    TRect grid(inner.a.x, inner.a.y + 1, inner.b.x - 1, inner.b.y);
    list_->locate(grid);

    if (list_->vScrollBar != nullptr) {
        TRect bar(inner.b.x - 1, inner.a.y + 1, inner.b.x, inner.b.y);
        list_->vScrollBar->locate(bar);
    }
}

void EvidenceListView::changeBounds(const TRect &bounds) {
    TWindow::changeBounds(bounds);
    placeList();
}

void EvidenceListView::openSelected(unsigned short command, nlohmann::json *evidence) {
    if (evidence == nullptr) {
        return;
    }

    const std::string path = jsonField(*evidence, "path");

    if (path.empty()) {
        return;
    }

    if (command == cmShowEvidenceInfo) {
        TProgram::deskTop->insert(new EvidenceInfoView(client_, path));
    } else {
        TProgram::deskTop->insert(new RecordListView(client_, path));
    }
}

void EvidenceListView::handleEvent(TEvent &event) {
    TWindow::handleEvent(event);

    if (event.what == evKeyDown && current == list_ && list_ != nullptr) {
        if (event.keyDown.keyCode == kbBack) {
            if (!query_.empty()) {
                std::string next = query_;
                while (!next.empty() && (static_cast<unsigned char>(next.back()) & 0xC0) == 0x80) {
                    next.pop_back();
                }

                if (!next.empty()) {
                    next.pop_back();
                }

                setQueryText(next);
            }

            clearEvent(event);
            return;
        }

        const TStringView typed = event.keyDown.getText();

        if (typed.size() > 0 && event.keyDown.text[0] >= 32) {
            setQueryText(query_ + std::string(typed.data(), typed.size()));
            clearEvent(event);
            return;
        }

        if (event.keyDown.keyCode == kbEnter) {
            list_->activateFocused();
            clearEvent(event);
            return;
        }
    }

    if (event.what == evKeyDown && current == queryInput_ && event.keyDown.keyCode == kbEnter) {
        list_->activateFocused();
        clearEvent(event);
        return;
    }

    if (event.what != evCommand) {
        return;
    }

    if (event.message.command == cmOpenRecordList || event.message.command == cmShowEvidenceInfo) {
        openSelected(event.message.command, static_cast<nlohmann::json *>(event.message.infoPtr));
        clearEvent(event);
    }
}

} // namespace abraflexitui
