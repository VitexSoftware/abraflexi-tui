#include "abraflexitui/TV.h"
#include "abraflexitui/AppButton.h"
#include "abraflexitui/RecordCreateForm.h"
#include "abraflexitui/Commands.h"
#include "abraflexitui/EvidenceSchema.h"
#include "abraflexitui/JsonFormat.h"
#include "abraflexitui/WindowLayout.h"

#include <vector>

namespace abraflexitui {

RecordCreateForm::RecordCreateForm(CliClient &client, std::string evidence)
    : TWindowInit(&TDialog::initFrame),
      TDialog(TRect(3, 1, 77, 23), ("New record: " + evidence).c_str()),
      client_(client), evidence_(std::move(evidence)) {
    options |= ofCentered;
    makeMaximizable(*this);

    const std::vector<FieldSchema> &schema = EvidenceSchema::fetch(client_, evidence_);

    TRect inner = getExtent();
    inner.grow(-1, -1);

    const short x = inner.a.x;
    short y = inner.a.y;
    const short right = inner.b.x;

    TView *hint = new TStaticText(TRect(x, y, right, y + 1),
                                   "Fill the fields below (mandatory ones are marked with *), then Submit.");
    growWide(hint);
    insert(hint);
    y += 1;

    TView *force = new TCheckBoxes(TRect(x, y, static_cast<short>(x + 40), y + 1),
                                   new TSItem("Force (skip mandatory fields)", nullptr));
    growWide(force);
    forceBox_ = static_cast<TCheckBoxes *>(force);
    insert(forceBox_);
    y += 1;

    const short formBottom = static_cast<short>(inner.b.y - 3);
    form_ = new RecordFieldForm(TRect(x, y, right, formBottom), schema, nlohmann::json::object());
    growFill(form_);
    insert(form_);

    TView *dryRun = new AppButton(TRect(right - 32, formBottom, right - 20, formBottom + 2), "Dry-~R~un",
                                cmRecordCreateDryRun, bfNormal);
    stickCorner(dryRun);
    insert(dryRun);
    TView *submit = new AppButton(TRect(right - 19, formBottom, right - 9, formBottom + 2), "~S~ubmit",
                                cmRecordCreateSubmit, bfDefault);
    stickCorner(submit);
    insert(submit);
    TView *cancel = new AppButton(TRect(right - 8, formBottom, right, formBottom + 2), "Cancel", cmCancel, bfNormal);
    stickCorner(cancel);
    insert(cancel);

    selectNext(False);
}

void RecordCreateForm::submit(bool dryRun) {
    nlohmann::json payload;

    try {
        payload = form_->currentValues();
    } catch (const nlohmann::json::parse_error &e) {
        messageBox(std::string("Invalid JSON: ") + e.what(), mfError | mfOKButton);
        return;
    }

    if (!payload.is_object()) {
        messageBox("JSON data must be an object, e.g. {\"nazev\":\"...\"}", mfError | mfOKButton);
        return;
    }

    if (!forceBox_->mark(0)) {
        std::vector<std::string> missing = form_->missingMandatory();

        if (!missing.empty()) {
            std::string msg = "Mandatory fields are still empty:";

            for (const auto &name : missing) {
                msg += "\n  - " + name;
            }

            msg += "\n\nSubmit anyway?";

            if (messageBox(msg, mfConfirmation | mfYesButton | mfNoButton) != cmYes) {
                return;
            }
        }
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

        default:
            break;
        }
    }
}

} // namespace abraflexitui
