#include "abraflexitui/TV.h"
#include "abraflexitui/AppButton.h"
#include "abraflexitui/QueryView.h"
#include "abraflexitui/Commands.h"
#include "abraflexitui/CodeFormat.h"
#include "abraflexitui/WindowLayout.h"

#include <cstring>
#include <sstream>
#include <vector>

namespace abraflexitui {

namespace {

constexpr unsigned short cmQuerySend = 1030;
constexpr ushort kBodySize = 8192;

class FormatRadios : public TRadioButtons {
public:
    FormatRadios(const TRect &bounds, TSItem *items, std::function<void(int)> onChange) noexcept
        : TRadioButtons(bounds, items), onChange_(std::move(onChange)) {
    }

    void press(int item) override {
        TRadioButtons::press(item);
        onChange_(item);
    }

    void movedTo(int item) override {
        TRadioButtons::movedTo(item);
        onChange_(item);
    }

private:
    std::function<void(int)> onChange_;
};

void setLine(TInputLine *input, const std::string &text) {
    std::strncpy(input->data, text.c_str(), static_cast<std::size_t>(input->maxLen));
    input->data[input->maxLen] = '\0';
}

std::vector<std::string> splitLines(const std::string &text) {
    std::vector<std::string> rows;
    std::istringstream in(text);
    std::string line;

    while (std::getline(in, line)) {
        rows.push_back(line);
    }

    if (rows.empty()) {
        rows.push_back("(empty response)");
    }

    return rows;
}

} // namespace

QueryView::QueryView(CliClient &client, ProfileStore &store, std::function<void()> onFormatChanged)
    : TWindowInit(&TDialog::initFrame),
      TDialog(TRect(2, 1, 78, 23), "Query"),
      client_(client),
      store_(store),
      onFormatChanged_(std::move(onFormatChanged)) {
    options |= ofCentered;
    makeMaximizable(*this);

    method_ = new TInputLine(TRect(12, 2, 22, 3), 8);
    setLine(method_, "GET");
    insert(method_);
    insert(new TLabel(TRect(2, 2, 12, 3), "~M~ethod:", method_));
    path_ = new TInputLine(TRect(32, 2, 74, 3), 240);
    growWide(path_);
    insert(path_);
    insert(new TLabel(TRect(24, 2, 32, 3), "~P~ath:", path_));

    format_ = new FormatRadios(TRect(12, 3, 32, 5), new TSItem("JSON", new TSItem("XML", nullptr)),
                               [this](int item) { setFormat(item == 1 ? ProfileStore::QueryFormat::Xml
                                                                      : ProfileStore::QueryFormat::Json); });
    insert(format_);
    ushort selected = store_.queryFormat() == ProfileStore::QueryFormat::Xml ? 1 : 0;
    format_->setData(&selected);

    TScrollBar *bodyBar = standardScrollBar(sbVertical | sbHandleKeyboard);
    body_ = new SyntaxMemo(TRect(2, 5, 74, 9), nullptr, bodyBar, nullptr, kBodySize);
    body_->setMode(store_.queryFormat() == ProfileStore::QueryFormat::Xml ? EditorMode::Xml : EditorMode::Json);
    growFill(body_);
    insert(body_);
    insert(new TLabel(TRect(2, 3, 12, 4), "~B~ody:", body_));

    TView *send = new AppButton(TRect(2, 9, 14, 11), "~S~end", cmQuerySend, bfDefault);
    stickBottom(send);
    insert(send);
    TView *format = new AppButton(TRect(16, 9, 30, 11), "~F~ormat", cmFormatCode, bfNormal);
    stickBottom(format);
    insert(format);
    TView *close = new AppButton(TRect(62, 9, 74, 11), "Close", cmCancel, bfNormal);
    stickCorner(close);
    insert(close);

    TScrollBar *resultBar = standardScrollBar(sbVertical | sbHandleKeyboard);
    result_ = new SimpleListViewer(TRect(2, 11, 74, 21), resultBar);
    stickBottomWide(result_);
    result_->setRows({"(response)"});
    insert(result_);
    selectNext(False);
}

void QueryView::setFormat(ProfileStore::QueryFormat format) {
    if (format == store_.queryFormat()) {
        return;
    }

    store_.setQueryFormat(format);
    std::string error;
    store_.save(error);
    body_->setMode(format == ProfileStore::QueryFormat::Xml ? EditorMode::Xml : EditorMode::Json);

    if (onFormatChanged_) {
        onFormatChanged_();
    }
}

void QueryView::formatBody() {
    std::string error;
    const bool xml = store_.queryFormat() == ProfileStore::QueryFormat::Xml;

    if (!formatEditorText(*body_, xml, error)) {
        messageBox(error.empty() ? std::string("Could not format the body") : error, mfError | mfOKButton);
    }
}

void QueryView::send() {
    const std::string method = method_->data;
    const std::string path = path_->data;

    if (path.empty()) {
        const char *sample = store_.queryFormat() == ProfileStore::QueryFormat::Xml ? "adresar.xml?limit=5"
                                                                                    : "adresar.json?limit=5";
        messageBox(std::string("Path is required, for example ") + sample, mfError | mfOKButton);
        return;
    }

    std::vector<std::string> args = {"query", path, "--method=" + method};
    std::vector<char> buf(body_->bufLen);
    std::string body;

    if (!buf.empty()) {
        const uint n = body_->getText(0, TSpan<char>(buf.data(), buf.size()));
        body.assign(buf.data(), n);
    }

    if (!body.empty() && body.find_first_not_of(" \t\r\n") != std::string::npos) {
        args.push_back("--body=" + body);
    }

    CliClient::Result result = client_.runJson(args);
    const std::string text = result.data.is_null() ? result.errorMessage : result.data.dump(2);
    result_->setRows(splitLines(result.ok ? text : ("Error: " + result.errorMessage + "\n" + text)));
}

void QueryView::handleEvent(TEvent &event) {
    TDialog::handleEvent(event);

    if (event.what == evCommand && event.message.command == cmQuerySend) {
        send();
        clearEvent(event);
    } else if (event.what == evCommand && event.message.command == cmFormatCode) {
        formatBody();
        clearEvent(event);
    }
}

} // namespace abraflexitui
