#include "abraflexitui/TV.h"
#include "abraflexitui/RecordCreateForm.h"
#include "abraflexitui/Commands.h"
#include "abraflexitui/CodeFormat.h"
#include "abraflexitui/JsonFormat.h"
#include "abraflexitui/WindowLayout.h"

#include <sstream>
#include <vector>

namespace abraflexitui {

namespace {

constexpr uint kBufSize = 16384;

std::string buildMandatoryHint(const nlohmann::json &preflightData) {
    if (!preflightData.is_object() || !preflightData.contains("mandatoryFieldsFormatted")) {
        return "(could not determine mandatory fields; check JSON manually)";
    }

    std::ostringstream oss;
    oss << "Mandatory fields for this evidence:";

    int shown = 0;

    for (const auto &item : preflightData.at("mandatoryFieldsFormatted")) {
        oss << "\n  - " << jsonDisplay(item);
        ++shown;

        if (shown >= 6) {
            oss << "\n  ...";
            break;
        }
    }

    if (shown == 0) {
        oss << "\n  (none reported)";
    }

    return oss.str();
}

} // namespace

RecordCreateForm::RecordCreateForm(CliClient &client, std::string evidence)
    : TWindowInit(&TDialog::initFrame),
      TDialog(TRect(3, 1, 77, 23), ("New record: " + evidence).c_str()),
      client_(client), evidence_(std::move(evidence)) {
    options |= ofCentered;
    makeMaximizable(*this);

    // Preflight: deliberately call create with no --data so abraflexi-cli's
    // own "No data provided" failure payload hands back the mandatory-field
    // hint (PropertiesHelper::getMandatoryFields), without re-implementing
    // any AbraFlexi schema logic here.
    CliClient::Result preflight = client_.runJson({"record", evidence_, "create"});

    TRect inner = getExtent();
    inner.grow(-1, -1);

    short x = inner.a.x;
    short y = inner.a.y;
    short right = inner.b.x;

    TView *hint = new TStaticText(TRect(x, y, right, y + 6), buildMandatoryHint(preflight.data).c_str());
    growWide(hint);
    insert(hint);
    y += 6;

    insert(new TStaticText(TRect(x, y, x + 36, y + 1), "JSON data:"));
    TView *force = new TCheckBoxes(TRect(x + 36, y, right, y + 1), new TSItem("Force (skip mandatory fields)", nullptr));
    growWide(force);
    forceBox_ = static_cast<TCheckBoxes *>(force);
    insert(forceBox_);
    y += 1;

    short editorBottom = static_cast<short>(inner.b.y - 3);
    TScrollBar *vBar = standardScrollBar(sbVertical | sbHandleKeyboard);
    editor_ = new TMemo(TRect(x, y, right, editorBottom), nullptr, vBar, nullptr, kBufSize);
    static const char kTemplate[] = "{\n  \n}";
    editor_->insertText(kTemplate, sizeof(kTemplate) - 1, False);
    growFill(editor_);
    insert(editor_);
    y = editorBottom;

    TView *format = new TButton(TRect(x, y, x + 14, y + 2), "~F~ormat", cmFormatCode, bfNormal);
    stickBottom(format);
    insert(format);
    TView *dryRun = new TButton(TRect(right - 32, y, right - 20, y + 2), "Dry-~R~un", cmRecordCreateDryRun, bfNormal);
    stickCorner(dryRun);
    insert(dryRun);
    TView *submit = new TButton(TRect(right - 19, y, right - 9, y + 2), "~S~ubmit", cmRecordCreateSubmit, bfDefault);
    stickCorner(submit);
    insert(submit);
    TView *cancel = new TButton(TRect(right - 8, y, right, y + 2), "Cancel", cmCancel, bfNormal);
    stickCorner(cancel);
    insert(cancel);

    selectNext(False);
}

std::string RecordCreateForm::readEditorText() const {
    std::vector<char> buf(editor_->bufLen);

    if (buf.empty()) {
        return std::string();
    }

    uint n = editor_->getText(0, TSpan<char>(buf.data(), buf.size()));
    return std::string(buf.data(), n);
}

void RecordCreateForm::submit(bool dryRun) {
    std::string text = readEditorText();

    nlohmann::json payload;

    try {
        payload = nlohmann::json::parse(text);
    } catch (const nlohmann::json::parse_error &e) {
        messageBox(std::string("Invalid JSON: ") + e.what(), mfError | mfOKButton);
        return;
    }

    if (!payload.is_object()) {
        messageBox("JSON data must be an object, e.g. {\"nazev\":\"...\"}", mfError | mfOKButton);
        return;
    }

    std::vector<std::string> args = {"record", evidence_, "create", "--data=" + payload.dump()};

    if (dryRun) {
        args.push_back("--dry-run");
    }

    if (forceBox_->mark(0)) {
        args.push_back("--force");
    }

    CliClient::Result result = client_.runJson(args);

    if (!result.ok) {
        std::string msg = result.errorMessage.empty() ? "Failed to create record" : result.errorMessage;

        if (result.data.is_object() && result.data.contains("missingFieldsFormatted")) {
            for (const auto &item : result.data.at("missingFieldsFormatted")) {
                msg += "\n  - " + jsonDisplay(item);
            }
        }

        messageBox(msg, mfError | mfOKButton);
        return;
    }

    std::string idText = jsonField(result.data, "id");
    std::string identText = jsonField(result.data, "ident");
    std::string msg = dryRun ? "Dry-run successful." : "Record created successfully!";

    if (!idText.empty()) {
        msg += "\nID: " + idText;
    }

    if (!identText.empty()) {
        msg += "\nIdent: " + identText;
    }

    messageBox(msg, mfInformation | mfOKButton);

    if (!dryRun) {
        endModal(cmOK);
    }
}

void RecordCreateForm::handleEvent(TEvent &event) {
    TDialog::handleEvent(event);

    if (event.what == evCommand) {
        switch (event.message.command) {
        case cmRecordCreateDryRun:
            submit(true);
            clearEvent(event);
            break;

        case cmRecordCreateSubmit:
            submit(false);
            clearEvent(event);
            break;

        case cmFormatCode: {
            std::string error;

            if (!formatEditorText(*editor_, false, error)) {
                messageBox(error.empty() ? std::string("Could not format JSON") : error, mfError | mfOKButton);
            }

            clearEvent(event);
            break;
        }

        default:
            break;
        }
    }
}

} // namespace abraflexitui
