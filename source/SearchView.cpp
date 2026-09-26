#include "abraflexitui/TV.h"
#include "abraflexitui/AppButton.h"
#include "abraflexitui/SearchView.h"
#include "abraflexitui/Commands.h"
#include "abraflexitui/JsonFormat.h"
#include "abraflexitui/RecordListView.h"
#include "abraflexitui/TextFold.h"
#include "abraflexitui/WindowColors.h"
#include "abraflexitui/WindowLayout.h"

#include <cstring>
#include <sstream>
#include <vector>

namespace abraflexitui {

namespace {

constexpr unsigned short cmSearchRun = 1031;
constexpr unsigned short cmSearchOpen = 1032;

void setLine(TInputLine *input, const char *text) {
    std::strncpy(input->data, text, static_cast<std::size_t>(input->maxLen));
    input->data[input->maxLen] = '\0';
}

class EvidenceLine : public TInputLine {
public:
    EvidenceLine(const TRect &bounds, SearchView &owner) noexcept : TInputLine(bounds, 64), owner_(owner) {
    }

    void handleEvent(TEvent &event) override {
        TInputLine::handleEvent(event);

        if (data != nullptr && std::string(data) != seen_) {
            seen_ = data;
            owner_.suggestEvidences();
        }
    }

    void remember(const std::string &text) {
        seen_ = text;
    }

private:
    SearchView &owner_;
    std::string seen_;
};

} // namespace

SearchView::SearchView(CliClient &client, SessionStore &session)
    : TWindowInit(&TDialog::initFrame),
      TDialog(TRect(4, 2, 76, 22), "Find"),
      client_(client), session_(session) {
    options |= ofCentered;
    makeMaximizable(*this);

    evidence_ = new EvidenceLine(TRect(14, 2, 40, 3), *this);
    insert(evidence_);
    insert(new TLabel(TRect(2, 2, 14, 3), "~E~vidence:", evidence_));
    query_ = new TInputLine(TRect(50, 2, 72, 3), 80);
    growWide(query_);
    insert(query_);
    insert(new TLabel(TRect(42, 2, 50, 3), "~T~ext:", query_));
    TView *hint = new TStaticText(TRect(2, 3, 72, 4),
                                  "Evidence narrows the catalogue. Tab fills it, Enter opens. Search looks up text.");
    growWide(hint);
    insert(hint);
    TView *search = new AppButton(TRect(2, 4, 14, 6), "~S~earch", cmSearchRun, bfDefault);
    insert(search);
    TView *open = new AppButton(TRect(16, 4, 28, 6), "~O~pen", cmSearchOpen, bfNormal);
    insert(open);
    TView *close = new AppButton(TRect(60, 4, 72, 6), "Close", cmCancel, bfNormal);
    stickRight(close);
    insert(close);

    TScrollBar *bar = standardScrollBar(sbVertical | sbHandleKeyboard);
    results_ = new SimpleListViewer(TRect(2, 6, 72, 20), bar);
    growFill(results_);
    results_->setRows({"(no results)"});
    insert(results_);
    suggestEvidences();
    selectNext(False);
}

void SearchView::loadCatalogue() {
    if (catalogueLoaded_) {
        return;
    }

    catalogueLoaded_ = true;
    CliClient::Result listed = client_.runJson({"list-evidences"});

    if (listed.ok && listed.data.is_array()) {
        for (const auto &item : listed.data) {
            catalogue_.push_back(item);
        }
    }
}

void SearchView::suggestEvidences() {
    if (query_ != nullptr && query_->data[0] != '\0') {
        return;
    }

    loadCatalogue();
    suggesting_ = true;
    hitEvidence_.clear();
    hitId_.clear();
    std::vector<std::string> rows;
    const std::string typed = evidence_ != nullptr ? evidence_->data : "";

    for (const auto &item : catalogue_) {
        const std::string path = jsonField(item, "path");
        const std::string name = jsonField(item, "name");
        const std::string description = jsonField(item, "description");

        if (!foldedContains(path + " " + name + " " + description, typed)) {
            continue;
        }

        rows.push_back(fitColumn(path, 28) + " " + name);
        hitEvidence_.push_back(path);
        hitId_.push_back(std::string());
    }

    if (rows.empty()) {
        rows.push_back(catalogue_.empty() ? "(no evidences)" : "(no match)");
    }

    results_->setRows(std::move(rows));
}

void SearchView::acceptSuggestion() {
    const short index = results_->focused;

    if (index < 0 || static_cast<std::size_t>(index) >= hitEvidence_.size()) {
        return;
    }

    const std::string &path = hitEvidence_[static_cast<std::size_t>(index)];

    if (path.empty()) {
        return;
    }

    setLine(evidence_, path.c_str());
    static_cast<EvidenceLine *>(evidence_)->remember(path);
    evidence_->drawView();
    suggesting_ = false;
}

void SearchView::runSearch() {
    const std::string evidence = evidence_->data;
    const std::string query = query_->data;
    suggesting_ = false;
    hitEvidence_.clear();
    hitId_.clear();
    std::vector<std::string> rows;

    if (query.empty()) {
        messageBox("Enter the text to find.", mfError | mfOKButton);
        return;
    }

    if (evidence.empty()) {
        CliClient::Result listed = client_.runJson({"list-evidences"});

        if (!listed.ok || !listed.data.is_array()) {
            results_->setRows({"Error: " + listed.errorMessage});
            return;
        }

        for (const auto &item : listed.data) {
            const std::string path = jsonField(item, "path");
            const std::string name = jsonField(item, "name");
            const std::string description = jsonField(item, "description");
            const std::string hay = path + " " + name + " " + description;

            if (!foldedContains(hay, query)) {
                continue;
            }

            rows.push_back(fitColumn(path, 28) + " " + name);
            hitEvidence_.push_back(path);
            hitId_.push_back(std::string());
        }
    } else {
        CliClient::Result found = client_.runJson({"record", evidence, "search", "--query=" + query, "--limit=30"});

        if (!found.ok || !found.data.is_array()) {
            results_->setRows({"Error: " + found.errorMessage});
            return;
        }

        for (const auto &item : found.data) {
            rows.push_back(fitColumn(jsonField(item, "id"), 8) + " " + fitColumn(jsonField(item, "kod"), 16) + " " +
                           jsonField(item, "nazev"));
            hitEvidence_.push_back(evidence);
            hitId_.push_back(jsonField(item, "id"));
        }
    }

    if (rows.empty()) {
        rows.push_back("(no matches)");
    }

    results_->setRows(std::move(rows));
}

void SearchView::openCurrent() {
    const short index = results_->focused;

    if (index < 0 || static_cast<std::size_t>(index) >= hitEvidence_.size()) {
        return;
    }

    const std::string &evidence = hitEvidence_[static_cast<std::size_t>(index)];

    if (evidence.empty()) {
        return;
    }

    const std::string &id = hitId_[static_cast<std::size_t>(index)];

    if (id.empty()) {
        TProgram::deskTop->insert(new RecordListView(client_, session_, evidence));
        return;
    }

    CliClient::Result shown = client_.runJson({"record", evidence, "show", id});

    if (!shown.ok) {
        messageBox(shown.errorMessage.empty() ? std::string("Cannot show record") : shown.errorMessage,
                   mfError | mfOKButton);
        return;
    }

    auto *dlg = new TDialog(TRect(6, 2, 74, 22), (evidence + " " + id).c_str());
    dlg->options |= ofCentered;
    makeMaximizable(*dlg);
    std::vector<std::string> lines;
    std::istringstream in(shown.data.dump(2));
    std::string line;

    while (std::getline(in, line)) {
        lines.push_back(line);
    }

    TScrollBar *bar = dlg->standardScrollBar(sbVertical | sbHandleKeyboard);
    auto *list = new SimpleListViewer(TRect(2, 2, 64, 16), bar);
    growFill(list);
    list->setRows(std::move(lines));
    dlg->insert(list);
    TView *ok = new AppButton(TRect(28, 16, 40, 18), "O~K~", cmOK, bfDefault);
    stickBottom(ok);
    dlg->insert(ok);
    TProgram::application->executeDialog(dlg);
}

void SearchView::handleEvent(TEvent &event) {
    if (event.what == evKeyDown && suggesting_ && current == evidence_) {
        if (event.keyDown.keyCode == kbTab) {
            acceptSuggestion();
        } else if (event.keyDown.keyCode == kbEnter) {
            openCurrent();
            clearEvent(event);
            return;
        }
    }

    TDialog::handleEvent(event);

    if (event.what == evCommand && event.message.command == cmSearchRun) {
        runSearch();
        clearEvent(event);
        return;
    }

    if (event.what == evCommand && event.message.command == cmSearchOpen) {
        openCurrent();
        clearEvent(event);
        return;
    }

    if (event.what == evKeyDown && current == evidence_ && results_ != nullptr) {
        short target = results_->focused;
        bool moved = true;

        switch (event.keyDown.keyCode) {
        case kbDown:
            target = static_cast<short>(target + 1);
            break;
        case kbUp:
            target = static_cast<short>(target - 1);
            break;
        case kbPgDn:
            target = static_cast<short>(target + (results_->size.y > 0 ? results_->size.y : 1));
            break;
        case kbPgUp:
            target = static_cast<short>(target - (results_->size.y > 0 ? results_->size.y : 1));
            break;
        default:
            moved = false;
            break;
        }

        if (moved) {
            results_->focusItemNum(target);
            clearEvent(event);
            return;
        }
    }

    if (event.what == evKeyDown && event.keyDown.keyCode == kbEnter && results_ != nullptr &&
        results_->state & sfFocused) {
        openCurrent();
        clearEvent(event);
    }
}

TColorAttr SearchView::mapColor(uchar index) {
    TColorAttr color;
    return windowColor(index, color) ? color : TDialog::mapColor(index);
}

} // namespace abraflexitui
