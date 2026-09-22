#include "abraflexitui/TV.h"
#include "abraflexitui/AppButton.h"
#include "abraflexitui/SearchView.h"
#include "abraflexitui/Commands.h"
#include "abraflexitui/JsonFormat.h"
#include "abraflexitui/RecordListView.h"
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

} // namespace

SearchView::SearchView(CliClient &client, SessionStore &session)
    : TWindowInit(&TDialog::initFrame),
      TDialog(TRect(4, 2, 76, 22), "Find"),
      client_(client), session_(session) {
    options |= ofCentered;
    makeMaximizable(*this);

    evidence_ = new TInputLine(TRect(14, 2, 40, 3), 64);
    insert(evidence_);
    insert(new TLabel(TRect(2, 2, 14, 3), "~E~vidence:", evidence_));
    query_ = new TInputLine(TRect(50, 2, 72, 3), 80);
    growWide(query_);
    insert(query_);
    insert(new TLabel(TRect(42, 2, 50, 3), "~T~ext:", query_));
    TView *hint = new TStaticText(TRect(2, 3, 72, 4), "Empty evidence searches catalogue names. Enter opens the hit.");
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
    selectNext(False);
}

void SearchView::runSearch() {
    const std::string evidence = evidence_->data;
    const std::string query = query_->data;
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

            if (hay.find(query) == std::string::npos) {
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

    if (event.what == evKeyDown && event.keyDown.keyCode == kbEnter && results_ != nullptr &&
        results_->state & sfFocused) {
        openCurrent();
        clearEvent(event);
    }
}

} // namespace abraflexitui
