#include "abraflexitui/DownloadDialog.h"
#include "abraflexitui/AppButton.h"
#include "abraflexitui/Commands.h"
#include "abraflexitui/FormatsCatalog.h"
#include "abraflexitui/RawDownload.h"
#include "abraflexitui/WindowColors.h"

#include <cstdlib>
#include <cstring>

namespace abraflexitui {

namespace {

constexpr ushort cmDoDownload = 1091;
constexpr ushort cmBrowsePath = 1092;

class SimpleStringListBox : public TListViewer {
public:
    SimpleStringListBox(const TRect &bounds, TScrollBar *bar) noexcept : TListViewer(bounds, 1, nullptr, bar) {
    }

    void setItems(std::vector<std::string> items) {
        items_ = std::move(items);
        setRange(static_cast<short>(items_.size()));
        drawView();
    }

    void getText(char *dest, short item, short maxLen) override {
        if (item >= 0 && static_cast<std::size_t>(item) < items_.size()) {
            std::strncpy(dest, items_[static_cast<std::size_t>(item)].c_str(), static_cast<std::size_t>(maxLen));
            dest[maxLen] = '\0';
        } else {
            dest[0] = '\0';
        }
    }

private:
    std::vector<std::string> items_;
};

void setInputText(TInputLine *input, const std::string &text) {
    std::strncpy(input->data, text.c_str(), static_cast<std::size_t>(input->maxLen));
    input->data[input->maxLen] = '\0';
}

} // namespace

DownloadDialog::DownloadDialog(CliClient &client, std::string evidence, std::string recordId, std::string company)
    : TWindowInit(&TDialog::initFrame),
      TDialog(TRect(0, 0, 72, 17), ("Download: " + evidence + " " + recordId).c_str()),
      client_(client), evidence_(std::move(evidence)), recordId_(std::move(recordId)),
      company_(company.empty() ? client.company() : std::move(company)) {
    options |= ofCentered;

    short x = 2;
    short y = 2;
    short right = 70;

    insert(new TLabel(TRect(x, y, x + 16, y + 1), "~F~ormat:", nullptr));
    y++;
    formatBar_ = standardScrollBar(sbVertical | sbHandleKeyboard);
    auto *fmtBox = new SimpleStringListBox(TRect(x, y, right, static_cast<short>(y + 6)), formatBar_);
    insert(formatBar_);
    insert(fmtBox);
    formatList_ = fmtBox;
    y += 7;

    insert(new TLabel(TRect(x, y, x + 12, y + 1), "~D~estination:", nullptr));
    pathInput_ = new TInputLine(TRect(x + 13, y, static_cast<short>(right - 12), y + 1), 255);
    insert(pathInput_);
    insert(new AppButton(TRect(static_cast<short>(right - 10), y, right, static_cast<short>(y + 2)), "~B~rowse",
                          cmBrowsePath, bfNormal));
    y += 2;

    insert(new AppButton(TRect(x, y, static_cast<short>(x + 16), static_cast<short>(y + 2)), "~D~ownload",
                          cmDoDownload, bfDefault));
    insert(new AppButton(TRect(static_cast<short>(x + 18), y, static_cast<short>(x + 30), static_cast<short>(y + 2)),
                          "Cancel", cmCancel, bfNormal));

    formats_ = FormatsCatalog::forEvidence(evidence_);
    std::vector<std::string> labels;
    for (const auto &fmt : formats_) {
        labels.push_back(fmt.first);
    }
    fmtBox->setItems(labels);

    updateDefaultPath();
    selectNext(False);
}

void DownloadDialog::updateDefaultPath() {
    const short focused = formatList_->focused;
    if (focused < 0 || static_cast<std::size_t>(focused) >= formats_.size()) {
        return;
    }

    const char *home = std::getenv("HOME");
    std::string dir = (home != nullptr) ? home : "/tmp";
    std::string fileName = evidence_ + "-" + recordId_ + "." + formats_[static_cast<std::size_t>(focused)].second;
    setInputText(pathInput_, dir + "/" + fileName);
    pathInput_->drawView();
}

void DownloadDialog::browseForPath() {
    TFileDialog *dlg = new TFileDialog("*.*", "Save As", "~N~ame", fdOKButton | fdHelpButton, 0);
    if (TProgram::application->executeDialog(dlg) == cmOK) {
        char buf[MAXPATH];
        dlg->getFileName(buf);
        setInputText(pathInput_, buf);
        pathInput_->drawView();
    }
}

void DownloadDialog::doDownload() {
    const short focused = formatList_->focused;
    if (focused < 0 || static_cast<std::size_t>(focused) >= formats_.size()) {
        messageBox("Select a format first.", mfError | mfOKButton);
        return;
    }

    std::string destPath = pathInput_->data;
    if (destPath.empty()) {
        messageBox("Enter a destination path first.", mfError | mfOKButton);
        return;
    }

    const std::string &ext = formats_[static_cast<std::size_t>(focused)].second;
    std::string queryPath = evidence_ + "/" + recordId_ + "." + ext;

    std::string errMsg;
    if (!downloadRawToFile(client_, queryPath, company_, destPath, errMsg)) {
        messageBox("Download failed: " + errMsg, mfError | mfOKButton);
        return;
    }

    messageBox("Saved to " + destPath, mfInformation | mfOKButton);
    endModal(cmOK);
}

void DownloadDialog::handleEvent(TEvent &event) {
    TDialog::handleEvent(event);

    if (event.what == evBroadcast && event.message.command == cmListItemSelected && event.message.infoPtr == formatList_) {
        updateDefaultPath();
        clearEvent(event);
        return;
    }

    if (event.what != evCommand) {
        return;
    }

    if (event.message.command == cmBrowsePath) {
        browseForPath();
        clearEvent(event);
    } else if (event.message.command == cmDoDownload) {
        doDownload();
        clearEvent(event);
    }
}

TColorAttr DownloadDialog::mapColor(uchar index) {
    TColorAttr color;
    return windowColor(index, color) ? color : TDialog::mapColor(index);
}

} // namespace abraflexitui
