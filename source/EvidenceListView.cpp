#include "abraflexitui/TV.h"
#include "abraflexitui/EvidenceListView.h"
#include "abraflexitui/Commands.h"
#include "abraflexitui/JsonFormat.h"
#include "abraflexitui/RecordListView.h"
#include "abraflexitui/EvidenceInfoView.h"
#include "abraflexitui/WindowLayout.h"

#include <vector>

namespace abraflexitui {

EvidenceListBox::EvidenceListBox(const TRect &bounds, TScrollBar *vScrollBar,
                                  std::vector<nlohmann::json> evidences) noexcept
    : SimpleListViewer(bounds, vScrollBar), evidences_(std::move(evidences)) {
    std::vector<std::string> rows;
    rows.push_back(fitColumn("Path", 24) + " " + fitColumn("Name", 32) + " " + "Description");

    for (const auto &ev : evidences_) {
        rows.push_back(fitColumn(jsonField(ev, "path"), 24) + " " + fitColumn(jsonField(ev, "name"), 32) + " " +
                        jsonField(ev, "description"));
    }

    setRows(std::move(rows));
}

void EvidenceListBox::handleEvent(TEvent &event) {
    if ((event.what == evMouseDown && (event.mouse.eventFlags & meDoubleClick)) ||
        (event.what == evKeyDown && event.keyDown.keyCode == kbEnter)) {
        if (focused >= 1 && static_cast<std::size_t>(focused - 1) < evidences_.size()) {
            nlohmann::json selected = evidences_[static_cast<std::size_t>(focused - 1)];
            message(owner, evCommand, cmOpenRecordList, &selected);
        }

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
    options |= ofCentered;

    CliClient::Result result = client.runJson({"list-evidences"});
    std::vector<nlohmann::json> evidences;

    if (result.ok && result.data.is_array()) {
        for (const auto &ev : result.data) {
            evidences.push_back(ev);
        }
    }

    TScrollBar *vBar = standardScrollBar(sbVertical | sbHandleKeyboard);
    list_ = new EvidenceListBox(getExtent().grow(-1, -1), vBar, std::move(evidences));
    growFill(list_);
    insert(list_);

    if (!result.ok) {
        list_->setRows({std::string("Error: ") + result.errorMessage});
    }
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

    if (event.what != evCommand) {
        return;
    }

    if (event.message.command == cmOpenRecordList || event.message.command == cmShowEvidenceInfo) {
        openSelected(event.message.command, static_cast<nlohmann::json *>(event.message.infoPtr));
        clearEvent(event);
    }
}

} // namespace abraflexitui
