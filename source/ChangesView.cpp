#include "abraflexitui/TV.h"
#include "abraflexitui/AppButton.h"
#include "abraflexitui/ChangesView.h"
#include "abraflexitui/Commands.h"
#include "abraflexitui/JsonFormat.h"
#include "abraflexitui/WindowColors.h"
#include "abraflexitui/WindowLayout.h"

#include <cstring>
#include <vector>

namespace abraflexitui {

class MutableStaticText : public TStaticText {
public:
    MutableStaticText(const TRect &bounds, const char *aText) noexcept : TStaticText(bounds, aText) {
    }

    void setMessage(const std::string &value) {
        delete[] const_cast<char *>(text);
        text = newStr(value);
        drawView();
    }
};

namespace {

constexpr unsigned short cmChangesEnable = 1033;
constexpr unsigned short cmChangesDisable = 1034;
constexpr unsigned short cmChangesRegister = 1035;
constexpr unsigned short cmChangesUnregister = 1036;
constexpr unsigned short cmChangesReload = 1037;

void setLine(TInputLine *input, const std::string &text) {
    std::strncpy(input->data, text.c_str(), static_cast<std::size_t>(input->maxLen));
    input->data[input->maxLen] = '\0';
}

} // namespace

ChangesView::ChangesView(CliClient &client)
    : TWindowInit(&TDialog::initFrame),
      TDialog(TRect(4, 2, 76, 22), "Changes API"),
      client_(client) {
    options |= ofCentered;
    makeMaximizable(*this);

    status_ = new MutableStaticText(TRect(2, 2, 70, 3), "Loading...");
    growWide(status_);
    insert(status_);
    TView *heading = new TStaticText(TRect(2, 3, 70, 4), "Webhooks");
    growWide(heading);
    insert(heading);

    TScrollBar *bar = standardScrollBar(sbVertical | sbHandleKeyboard);
    hooks_ = new SimpleListViewer(TRect(2, 4, 70, 12), bar);
    growFill(hooks_);
    insert(hooks_);

    url_ = new TInputLine(TRect(10, 12, 70, 13), 240);
    stickBottomWide(url_);
    insert(url_);
    TView *urlLabel = new TLabel(TRect(2, 12, 10, 13), "~U~RL:", url_);
    stickBottom(urlLabel);
    insert(urlLabel);

    TView *enable = new AppButton(TRect(2, 14, 16, 16), "~E~nable", cmChangesEnable, bfNormal);
    stickBottom(enable);
    insert(enable);
    TView *disable = new AppButton(TRect(17, 14, 31, 16), "~D~isable", cmChangesDisable, bfNormal);
    stickBottom(disable);
    insert(disable);
    TView *reg = new AppButton(TRect(32, 14, 48, 16), "~R~egister", cmChangesRegister, bfNormal);
    stickBottom(reg);
    insert(reg);
    TView *unreg = new AppButton(TRect(49, 14, 66, 16), "Unre~g~ister", cmChangesUnregister, bfNormal);
    stickBottom(unreg);
    insert(unreg);
    TView *refresh = new AppButton(TRect(2, 17, 16, 19), "Re~f~resh", cmChangesReload, bfNormal);
    stickBottom(refresh);
    insert(refresh);
    TView *close = new AppButton(TRect(54, 17, 66, 19), "Close", cmCancel, bfNormal);
    stickCorner(close);
    insert(close);

    reload();
    selectNext(False);
}

void ChangesView::reload() {
    hookIds_.clear();
    CliClient::Result result = client_.runJson({"changes", "status"});

    if (!result.ok) {
        status_->setMessage("Error: " + result.errorMessage);
        hooks_->setRows({result.errorMessage});
        return;
    }

    const std::string enabled = result.data.value("enabled", false) ? "enabled" : "disabled";
    const std::string version = jsonField(result.data, "globalVersion");
    status_->setMessage("Changes API is " + enabled + ". Global version: " + version);

    std::vector<std::string> rows;

    if (result.data.contains("hooks") && result.data.at("hooks").is_array()) {
        for (const auto &hook : result.data.at("hooks")) {
            const std::string id = jsonField(hook, "id");
            rows.push_back(fitColumn(id, 8) + " " + jsonField(hook, "url"));
            hookIds_.push_back(id);
        }
    }

    if (rows.empty()) {
        rows.push_back("(no webhooks)");
    }

    hooks_->setRows(std::move(rows));
}

void ChangesView::setEnabled(bool enabled) {
    CliClient::Result result = client_.runJson({"changes", enabled ? "enable" : "disable"});

    if (!result.ok) {
        messageBox(result.errorMessage.empty() ? std::string("Changes API request failed") : result.errorMessage,
                   mfError | mfOKButton);
    }

    reload();
}

void ChangesView::registerHook() {
    const std::string url = url_->data;

    if (url.empty()) {
        messageBox("Enter the webhook URL first.", mfError | mfOKButton);
        return;
    }

    CliClient::Result result = client_.runJson({"changes", "register", "--url=" + url});

    if (!result.ok) {
        messageBox(result.errorMessage.empty() ? std::string("Webhook was not registered") : result.errorMessage,
                   mfError | mfOKButton);
        return;
    }

    setLine(url_, "");
    url_->drawView();
    reload();
}

void ChangesView::unregisterHook() {
    const short index = hooks_->focused;

    if (index < 0 || static_cast<std::size_t>(index) >= hookIds_.size() || hookIds_[static_cast<std::size_t>(index)].empty()) {
        messageBox("Select a webhook first.", mfError | mfOKButton);
        return;
    }

    const std::string id = hookIds_[static_cast<std::size_t>(index)];

    if (messageBox("Remove webhook " + id + "?", mfConfirmation | mfYesButton | mfNoButton) != cmYes) {
        return;
    }

    CliClient::Result result = client_.runJson({"changes", "unregister", id});

    if (!result.ok) {
        messageBox(result.errorMessage.empty() ? std::string("Webhook was not removed") : result.errorMessage,
                   mfError | mfOKButton);
        return;
    }

    reload();
}

void ChangesView::handleEvent(TEvent &event) {
    TDialog::handleEvent(event);

    if (event.what != evCommand) {
        return;
    }

    switch (event.message.command) {
    case cmChangesEnable:
        setEnabled(true);
        clearEvent(event);
        break;
    case cmChangesDisable:
        setEnabled(false);
        clearEvent(event);
        break;
    case cmChangesRegister:
        registerHook();
        clearEvent(event);
        break;
    case cmChangesUnregister:
        unregisterHook();
        clearEvent(event);
        break;
    case cmChangesReload:
        reload();
        clearEvent(event);
        break;
    default:
        break;
    }
}

TColorAttr ChangesView::mapColor(uchar index) {
    TColorAttr color;
    return windowColor(index, color) ? color : TDialog::mapColor(index);
}

} // namespace abraflexitui
