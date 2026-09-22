#include "abraflexitui/TV.h"
#include "abraflexitui/RecordEditForm.h"
#include "abraflexitui/Commands.h"
#include "abraflexitui/CodeFormat.h"
#include "abraflexitui/JsonFormat.h"
#include "abraflexitui/WindowLayout.h"

#include <vector>

namespace abraflexitui {

namespace {

constexpr ushort kBufSize = 32767;

} // namespace

RecordEditForm::RecordEditForm(CliClient &client, std::string evidence, std::string id)
    : TWindowInit(&TDialog::initFrame),
      TDialog(TRect(3, 1, 77, 23), ("Edit " + evidence + " " + id).c_str()),
      client_(client), evidence_(std::move(evidence)), id_(std::move(id)) {
    options |= ofCentered;
    makeMaximizable(*this);

    TRect inner = getExtent();
    inner.grow(-1, -1);
    const short x = inner.a.x;
    const short right = inner.b.x;
    short y = inner.a.y;

    TView *hint = new TStaticText(TRect(x, y, right, y + 1), "Edit the JSON, then Save. Dry-Run does not write.");
    growWide(hint);
    insert(hint);
    y += 1;

    const short editorBottom = static_cast<short>(inner.b.y - 3);
    TScrollBar *vBar = standardScrollBar(sbVertical | sbHandleKeyboard);
    editor_ = new TMemo(TRect(x, y, right, editorBottom), nullptr, vBar, nullptr, kBufSize);
    growFill(editor_);
    insert(editor_);

    CliClient::Result current = client_.runJson({"record", evidence_, "show", id_});
    const std::string text = current.ok ? current.data.dump(2) : std::string("{\n  \"id\": \"") + id_ + "\"\n}";
    editor_->insertText(text.c_str(), static_cast<uint>(text.size()), False);

    if (!current.ok) {
        TView *warning = new TStaticText(TRect(x, editorBottom, right, editorBottom + 1),
                                         ("Could not load record: " + current.errorMessage).c_str());
        stickBottomWide(warning);
        insert(warning);
    }

    TView *format = new TButton(TRect(x, editorBottom + 1, x + 14, editorBottom + 3), "~F~ormat", cmFormatCode, bfNormal);
    stickBottom(format);
    insert(format);
    TView *dryRun = new TButton(TRect(right - 30, editorBottom + 1, right - 18, editorBottom + 3), "Dry-~R~un",
                                cmRecordCreateDryRun, bfNormal);
    stickCorner(dryRun);
    insert(dryRun);
    TView *save = new TButton(TRect(right - 17, editorBottom + 1, right - 8, editorBottom + 3), "~S~ave",
                              cmRecordCreateSubmit, bfDefault);
    stickCorner(save);
    insert(save);
    TView *cancel = new TButton(TRect(right - 7, editorBottom + 1, right, editorBottom + 3), "Cancel", cmCancel, bfNormal);
    stickCorner(cancel);
    insert(cancel);

    selectNext(False);
}

std::string RecordEditForm::readEditorText() const {
    std::vector<char> buf(editor_->bufLen);

    if (buf.empty()) {
        return std::string();
    }

    const uint n = editor_->getText(0, TSpan<char>(buf.data(), buf.size()));
    return std::string(buf.data(), n);
}

void RecordEditForm::submit(bool dryRun) {
    nlohmann::json payload;

    try {
        payload = nlohmann::json::parse(readEditorText());
    } catch (const nlohmann::json::parse_error &e) {
        messageBox(std::string("Invalid JSON: ") + e.what(), mfError | mfOKButton);
        return;
    }

    if (!payload.is_object()) {
        messageBox("JSON data must be an object.", mfError | mfOKButton);
        return;
    }

    std::vector<std::string> args = {"record", evidence_, "update", id_, "--data=" + payload.dump()};

    if (dryRun) {
        args.push_back("--dry-run");
    }

    CliClient::Result result = client_.runJson(args);

    if (!result.ok) {
        messageBox(result.errorMessage.empty() ? std::string("Failed to update record") : result.errorMessage,
                   mfError | mfOKButton);
        return;
    }

    messageBox(dryRun ? "Dry-run successful." : "Record updated.", mfInformation | mfOKButton);

    if (!dryRun) {
        endModal(cmOK);
    }
}

void RecordEditForm::handleEvent(TEvent &event) {
    TDialog::handleEvent(event);

    if (event.what != evCommand) {
        return;
    }

    if (event.message.command == cmRecordCreateDryRun) {
        submit(true);
        clearEvent(event);
    } else if (event.message.command == cmRecordCreateSubmit) {
        submit(false);
        clearEvent(event);
    } else if (event.message.command == cmFormatCode) {
        std::string error;

        if (!formatEditorText(*editor_, false, error)) {
            messageBox(error.empty() ? std::string("Could not format JSON") : error, mfError | mfOKButton);
        }

        clearEvent(event);
    }
}

} // namespace abraflexitui
