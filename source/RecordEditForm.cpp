#include "abraflexitui/TV.h"
#include "abraflexitui/AppButton.h"
#include "abraflexitui/RecordEditForm.h"
#include "abraflexitui/Commands.h"
#include "abraflexitui/EvidenceSchema.h"
#include "abraflexitui/WindowColors.h"
#include "abraflexitui/WindowLayout.h"

#include <vector>

namespace abraflexitui {

RecordEditForm::RecordEditForm(CliClient &client, std::string evidence, std::string id, std::string company)
    : TWindowInit(&TDialog::initFrame),
      TDialog(TRect(3, 1, 77, 23),
              ("Edit " + evidence + " " + id + " [" + (company.empty() ? client.company() : company) + "]").c_str()),
      client_(client), evidence_(std::move(evidence)), id_(std::move(id)),
      company_(company.empty() ? client.company() : std::move(company)) {
    options |= ofCentered;
    makeMaximizable(*this);

    TRect inner = getExtent();
    inner.grow(-1, -1);
    const short x = inner.a.x;
    const short right = inner.b.x;
    short y = inner.a.y;

    CliClient::Result current = client_.runJsonForCompany({"record", evidence_, "show", id_}, company_);
    nlohmann::json initial = current.ok ? current.data : nlohmann::json{{"id", id_}};
    const std::vector<FieldSchema> &schema = EvidenceSchema::fetch(client_, evidence_, company_);

    std::string hintText = current.ok ? "Edit the fields, then Save. Dry-Run does not write."
                                       : ("Could not load record: " + current.errorMessage);
    TView *hint = new TStaticText(TRect(x, y, right, y + 1), hintText.c_str());
    growWide(hint);
    insert(hint);
    y += 1;

    const short formBottom = static_cast<short>(inner.b.y - 3);
    form_ = new RecordFieldForm(TRect(x, y, right, formBottom), client_, company_, schema, initial);
    growFill(form_);
    insert(form_);

    TView *dryRun = new AppButton(TRect(right - 35, formBottom + 1, right - 23, formBottom + 3), "Dry-~R~un",
                                cmRecordCreateDryRun, bfNormal);
    stickCorner(dryRun);
    insert(dryRun);
    TView *save = new AppButton(TRect(right - 22, formBottom + 1, right - 13, formBottom + 3), "~S~ave",
                              cmRecordCreateSubmit, bfDefault);
    stickCorner(save);
    insert(save);
    TView *cancel = new AppButton(TRect(right - 12, formBottom + 1, right - 5, formBottom + 3), "Cancel", cmCancel, bfNormal);
    stickCorner(cancel);
    insert(cancel);

    selectNext(False);
}

void RecordEditForm::submit(bool dryRun) {
    nlohmann::json payload;

    try {
        payload = form_->currentValues();
    } catch (const nlohmann::json::parse_error &e) {
        messageBox(std::string("Invalid JSON: ") + e.what(), mfError | mfOKButton);
        return;
    }

    if (!payload.is_object()) {
        messageBox("JSON data must be an object.", mfError | mfOKButton);
        return;
    }

    std::vector<std::string> missing = form_->missingMandatory();

    if (!missing.empty()) {
        std::string msg = "Mandatory fields are still empty:";

        for (const auto &name : missing) {
            msg += "\n  - " + name;
        }

        msg += "\n\nSave anyway?";

        if (messageBox(msg, mfConfirmation | mfYesButton | mfNoButton) != cmYes) {
            return;
        }
    }

    std::vector<std::string> args = {"record", evidence_, "update", id_, "--data=" + payload.dump()};

    if (dryRun) {
        args.push_back("--dry-run");
    }

    CliClient::Result result = client_.runJsonForCompany(args, company_);

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
    }
}

TColorAttr RecordEditForm::mapColor(uchar index) {
    TColorAttr color;
    return windowColor(index, color) ? color : TDialog::mapColor(index);
}

} // namespace abraflexitui
