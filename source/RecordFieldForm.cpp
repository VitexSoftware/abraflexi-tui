#include "abraflexitui/TV.h"
#include "abraflexitui/AppButton.h"
#include "abraflexitui/RecordFieldForm.h"
#include "abraflexitui/Commands.h"
#include "abraflexitui/CodeFormat.h"
#include "abraflexitui/JsonFormat.h"
#include "abraflexitui/RelationPickerDialog.h"
#include "abraflexitui/WindowLayout.h"

#include <algorithm>
#include <cstring>

namespace abraflexitui {

namespace {

constexpr short kLabelWidth = 24;
constexpr short kPickButtonWidth = 4;
constexpr int kDefaultMaxLen = 120;
constexpr int kMaxLenCap = 250;
constexpr uint kRawBufSize = 32767;

void setInputText(TInputLine *input, const std::string &text) {
    std::strncpy(input->data, text.c_str(), static_cast<std::size_t>(input->maxLen));
    input->data[input->maxLen] = '\0';
}

std::string inputText(TInputLine *input) {
    return std::string(input->data);
}

std::string fieldLabel(const FieldSchema &field) {
    std::string label = std::string(field.mandatory ? "* " : "  ") + (field.title.empty() ? field.name : field.title);

    if (field.type == "relation" && !field.relationEvidence.empty()) {
        label += " (\xE2\x86\x92 " + field.relationEvidence + ")";
    } else if (field.type == "date") {
        label += " (YYYY-MM-DD)";
    } else if (field.type == "datetime") {
        label += " (YYYY-MM-DD HH:MM:SS)";
    }

    return label;
}

std::vector<FieldSchema> orderedWritableFields(const std::vector<FieldSchema> &schema) {
    std::vector<FieldSchema> out;

    for (const auto &field : schema) {
        if (field.writable) {
            out.push_back(field);
        }
    }

    std::stable_sort(out.begin(), out.end(),
                      [](const FieldSchema &a, const FieldSchema &b) { return a.mandatory && !b.mandatory; });

    return out;
}

bool jsonValueIsTrue(const nlohmann::json &value) {
    return (value.is_boolean() && value.get<bool>()) || value == "true";
}

} // namespace

RecordFieldForm::RecordFieldForm(const TRect &bounds, CliClient &client, std::string company,
                                  std::vector<FieldSchema> schema, nlohmann::json initialValues) noexcept
    : TGroup(bounds), client_(client), company_(std::move(company)), fields_(orderedWritableFields(schema)),
      model_(std::move(initialValues)) {
    if (!model_.is_object()) {
        model_ = nlohmann::json::object();
    }

    growMode = gfGrowHiX | gfGrowHiY;

    TRect inner = getExtent();
    const short x = inner.a.x;
    const short right = inner.b.x;
    const short top = inner.a.y;

    pageLabel_ = new TStaticText(TRect(x, top, static_cast<short>(x + 16), static_cast<short>(top + 1)), "");
    insert(pageLabel_);

    prevBtn_ = new AppButton(TRect(static_cast<short>(x + 17), top, static_cast<short>(x + 27), static_cast<short>(top + 2)),
                              "~<~ Prev", cmFieldFormPrevPage, bfNormal);
    insert(prevBtn_);
    nextBtn_ = new AppButton(TRect(static_cast<short>(x + 28), top, static_cast<short>(x + 38), static_cast<short>(top + 2)),
                              "Next ~>~", cmFieldFormNextPage, bfNormal);
    insert(nextBtn_);

    formatBtn_ = new AppButton(TRect(static_cast<short>(right - 28), top, static_cast<short>(right - 15), static_cast<short>(top + 2)),
                                "~F~ormat", cmFormatCode, bfNormal);
    stickRight(formatBtn_);
    insert(formatBtn_);
    formatBtn_->hide();

    rawToggleBtn_ = new AppButton(TRect(static_cast<short>(right - 14), top, right, static_cast<short>(top + 2)),
                                   "~R~aw/Fields", cmFieldFormToggleRaw, bfNormal);
    stickRight(rawToggleBtn_);
    insert(rawToggleBtn_);

    recomputeGeometry();

    if (fields_.empty()) {
        // No schema available (CLI/lookup failure or an undocumented
        // evidence) - behave exactly like the old plain-JSON dialog: no
        // field page to show or toggle back to.
        pageLabel_->hide();
        prevBtn_->hide();
        nextBtn_->hide();
        rawToggleBtn_->hide();
        formatBtn_->show();
        buildRawEditor(model_.dump(2));
        rawMode_ = true;
    } else {
        showPage(0);
    }
}

void RecordFieldForm::recomputeGeometry() {
    TRect inner = getExtent();
    const short top = static_cast<short>(inner.a.y + 2);
    fieldsArea_ = TRect(inner.a.x, top, inner.b.x, inner.b.y);
    rowsPerPage_ = std::max<int>(1, fieldsArea_.b.y - fieldsArea_.a.y);
    pageCount_ = fields_.empty() ? 1 : static_cast<int>((fields_.size() + rowsPerPage_ - 1) / rowsPerPage_);
    page_ = std::max(0, std::min(page_, pageCount_ - 1));
}

void RecordFieldForm::changeBounds(const TRect &bounds) {
    TGroup::changeBounds(bounds);
    recomputeGeometry();

    if (rawMode_) {
        // The raw editor/scrollbar are plain TViews with no growMode set
        // for this - they were placed once against the old fieldsArea_ in
        // buildRawEditor() and need an explicit reposition here instead.
        if (rawEditor_ != nullptr) {
            rawEditor_->locate(fieldsArea_);
        }

        if (rawScrollBar_ != nullptr) {
            TRect scrollRect(static_cast<short>(fieldsArea_.b.x - 1), fieldsArea_.a.y, fieldsArea_.b.x, fieldsArea_.b.y);
            rawScrollBar_->locate(scrollRect);
        }
    } else {
        // Widen the *currently visible* rows in place instead of tearing
        // them down and rebuilding via showPage(): this changeBounds() call
        // itself runs from inside the owner's own resize cascade (the
        // dialog's TGroup::changeBounds() -> forEach(doCalcChange) loop), so
        // reaching back into our own subview list with remove()/insert()
        // here caused reentrant churn - lost focus, ghosted old field text
        // left on screen, and a dead relation-picker button (all reported
        // after this override was first added). The ask was for fields to
        // get wider on a wider window, not to re-paginate, so this avoids
        // rebuilding anything - Prev/Next still re-paginates properly using
        // the freshly recomputed rowsPerPage_/pageCount_ above.
        reflowCurrentPage();
        updatePageLabel();
    }
}

void RecordFieldForm::reflowCurrentPage() {
    const short inputX = std::min<short>(static_cast<short>(fieldsArea_.a.x + kLabelWidth),
                                         static_cast<short>(fieldsArea_.b.x - 1));

    for (auto &row : pageRows_) {
        TRect labelRect = row.label->getBounds();
        labelRect.b.x = inputX;
        row.label->locate(labelRect);

        if (row.pickButton != nullptr) {
            const short pickX = std::max<short>(inputX, static_cast<short>(fieldsArea_.b.x - kPickButtonWidth));

            TRect textRect = row.widget->getBounds();
            textRect.a.x = inputX;
            textRect.b.x = pickX;
            row.widget->locate(textRect);

            TRect btnRect = row.pickButton->getBounds();
            btnRect.a.x = pickX;
            btnRect.b.x = fieldsArea_.b.x;
            row.pickButton->locate(btnRect);
        } else if (row.field->type != "logic") {
            // Leave the checkbox at its original narrow width - only text
            // inputs and relation displays benefit from the extra width.
            TRect widgetRect = row.widget->getBounds();
            widgetRect.a.x = inputX;
            widgetRect.b.x = fieldsArea_.b.x;
            row.widget->locate(widgetRect);
        }
    }
}

void RecordFieldForm::showPage(int page) {
    flushPageIntoModel();
    clearPageViews();

    page_ = std::max(0, std::min(page, pageCount_ - 1));
    const int start = page_ * rowsPerPage_;
    const int end = std::min<int>(static_cast<int>(fields_.size()), start + rowsPerPage_);
    short y = fieldsArea_.a.y;
    const short inputX = std::min<short>(static_cast<short>(fieldsArea_.a.x + kLabelWidth),
                                         static_cast<short>(fieldsArea_.b.x - 1));

    for (int i = start; i < end; ++i) {
        const FieldSchema &field = fields_[static_cast<std::size_t>(i)];
        auto *label = new TStaticText(TRect(fieldsArea_.a.x, y, inputX, static_cast<short>(y + 1)), fieldLabel(field).c_str());
        insert(label);

        TView *widget = nullptr;
        TButton *pickButton = nullptr;
        std::string originalText;
        bool originalChecked = false;

        if (field.type == "logic") {
            auto *box = new TCheckBoxes(TRect(inputX, y, static_cast<short>(std::min<short>(inputX + 6, fieldsArea_.b.x)),
                                              static_cast<short>(y + 1)),
                                        new TSItem("", nullptr));

            originalChecked = model_.contains(field.name) && jsonValueIsTrue(model_.at(field.name));

            if (originalChecked) {
                box->press(0);
            }

            insert(box);
            widget = box;
        } else if (field.type == "relation" && !field.relationEvidence.empty()) {
            const short pickX = static_cast<short>(std::max<short>(inputX, fieldsArea_.b.x - kPickButtonWidth));

            std::string display;

            if (model_.contains(field.name)) {
                display = jsonDisplay(model_.at(field.name), &field);
            }

            originalText = display;
            auto *text = new TStaticText(TRect(inputX, y, pickX, static_cast<short>(y + 1)), display.c_str());
            insert(text);
            widget = text;

            pickButton = new AppButton(TRect(pickX, y, fieldsArea_.b.x, static_cast<short>(y + 1)), "...",
                                        cmFieldFormPickRelation, bfNormal);
            insert(pickButton);
        } else {
            const int maxLen = field.maxLength > 0 ? std::min(field.maxLength, kMaxLenCap) : kDefaultMaxLen;
            auto *input = new TInputLine(TRect(inputX, y, fieldsArea_.b.x, static_cast<short>(y + 1)), maxLen);

            if (model_.contains(field.name)) {
                originalText = jsonDisplay(model_.at(field.name), &field);
                setInputText(input, originalText);
            }

            insert(input);
            widget = input;
        }

        FieldRow row;
        row.field = &field;
        row.label = label;
        row.widget = widget;
        row.pickButton = pickButton;
        row.originalText = originalText;
        row.originalChecked = originalChecked;
        pageRows_.push_back(std::move(row));
        y = static_cast<short>(y + 1);
    }

    updatePageLabel();

    // Put keyboard focus on the first focusable control of the freshly
    // built page (the pick button for a relation row, since its TStaticText
    // display isn't selectable) so the page is immediately editable -
    // clearPageViews()'s resetCurrent() above only guarantees `current`
    // isn't dangling, not that it lands inside the page itself.
    for (auto &row : pageRows_) {
        TView *focusable = row.pickButton != nullptr ? static_cast<TView *>(row.pickButton) : row.widget;

        if ((focusable->options & ofSelectable) != 0) {
            focusable->select();
            break;
        }
    }
}

void RecordFieldForm::clearPageViews() {
    for (auto &row : pageRows_) {
        remove(row.label);
        delete row.label;
        remove(row.widget);
        delete row.widget;

        if (row.pickButton != nullptr) {
            remove(row.pickButton);
            delete row.pickButton;
        }
    }

    pageRows_.clear();

    // TGroup::remove()/removeView() don't clear `current` - if the deleted
    // row happened to hold keyboard focus, `current` is left dangling
    // (use-after-free) and the whole form silently stops responding to
    // input until something else re-focuses it. Re-anchor it to whatever
    // selectable sibling remains (e.g. the Prev/Next/Raw buttons); showPage()
    // moves it onto the freshly built page below.
    resetCurrent();
}

void RecordFieldForm::flushPageIntoModel() {
    for (auto &row : pageRows_) {
        if (row.field->type == "logic") {
            const bool checked = static_cast<TCheckBoxes *>(row.widget)->mark(0) == True;

            if (checked != row.originalChecked) {
                model_[row.field->name] = checked;
            }
        } else if (row.pickButton != nullptr) {
            // Relation row: the display TStaticText is read-only, so the
            // only way to change the value is through the picker. Leave
            // the model's existing relation object untouched unless the
            // user actually picked a new one this page-visit.
            if (row.hasPickedValue) {
                model_[row.field->name] = row.pickedValue;
            }
        } else {
            std::string text = inputText(static_cast<TInputLine *>(row.widget));

            // Only overwrite the model when the field was actually edited:
            // an untouched relation/nested field's original value may be a
            // whole JSON object, only stringified for display here, and
            // must not be replaced by that flattened string.
            if (text != row.originalText) {
                model_[row.field->name] = text;
            }
        }
    }
}

void RecordFieldForm::updatePageLabel() {
    if (pageLabel_ == nullptr) {
        return;
    }

    remove(pageLabel_);
    delete pageLabel_;
    std::string text = "Page " + std::to_string(page_ + 1) + "/" + std::to_string(pageCount_);
    pageLabel_ = new TStaticText(TRect(fieldsArea_.a.x, static_cast<short>(fieldsArea_.a.y - 2), static_cast<short>(fieldsArea_.a.x + 16),
                                       static_cast<short>(fieldsArea_.a.y - 1)),
                                 text.c_str());
    insert(pageLabel_);
}

void RecordFieldForm::buildRawEditor(const std::string &initialText) {
    TRect scrollRect(static_cast<short>(fieldsArea_.b.x - 1), fieldsArea_.a.y, fieldsArea_.b.x, fieldsArea_.b.y);
    rawScrollBar_ = new TScrollBar(scrollRect);
    rawScrollBar_->options |= ofPostProcess;
    insert(rawScrollBar_);

    rawEditor_ = new TMemo(fieldsArea_, nullptr, rawScrollBar_, nullptr, kRawBufSize);
    insert(rawEditor_);
    rawEditor_->insertText(initialText.c_str(), static_cast<uint>(initialText.size()), False);
    rawEditor_->select();
}

void RecordFieldForm::destroyRawEditor() {
    if (rawEditor_ != nullptr) {
        remove(rawEditor_);
        delete rawEditor_;
        rawEditor_ = nullptr;
    }

    if (rawScrollBar_ != nullptr) {
        remove(rawScrollBar_);
        delete rawScrollBar_;
        rawScrollBar_ = nullptr;
    }
}

std::string RecordFieldForm::readRawText() const {
    if (rawEditor_ == nullptr) {
        return std::string();
    }

    std::vector<char> buf(rawEditor_->bufLen);

    if (buf.empty()) {
        return std::string();
    }

    const uint n = rawEditor_->getText(0, TSpan<char>(buf.data(), buf.size()));
    return std::string(buf.data(), n);
}

void RecordFieldForm::toggleRaw() {
    if (fields_.empty()) {
        // No field page exists to toggle back to.
        return;
    }

    if (!rawMode_) {
        flushPageIntoModel();
        clearPageViews();
        pageLabel_->hide();
        prevBtn_->hide();
        nextBtn_->hide();
        formatBtn_->show();
        buildRawEditor(model_.dump(2));
        rawMode_ = true;
        return;
    }

    nlohmann::json parsed;

    try {
        parsed = nlohmann::json::parse(readRawText());
    } catch (const nlohmann::json::parse_error &e) {
        messageBox(std::string("Invalid JSON: ") + e.what(), mfError | mfOKButton);
        return;
    }

    if (!parsed.is_object()) {
        messageBox("JSON data must be an object.", mfError | mfOKButton);
        return;
    }

    model_ = std::move(parsed);
    destroyRawEditor();
    formatBtn_->hide();
    pageLabel_->show();
    prevBtn_->show();
    nextBtn_->show();
    rawMode_ = false;
    showPage(page_);
}

nlohmann::json RecordFieldForm::currentValues() {
    if (rawMode_) {
        return nlohmann::json::parse(readRawText());
    }

    flushPageIntoModel();
    return model_;
}

std::vector<std::string> RecordFieldForm::missingMandatory() {
    std::vector<std::string> missing;
    nlohmann::json values;

    try {
        values = currentValues();
    } catch (const nlohmann::json::parse_error &) {
        // Invalid raw JSON is reported by the caller's own parse step.
        return missing;
    }

    for (const auto &field : fields_) {
        if (!field.mandatory) {
            continue;
        }

        const bool isEmpty = !values.contains(field.name) || values.at(field.name).is_null() ||
                              (values.at(field.name).is_string() && values.at(field.name).get<std::string>().empty());

        if (isEmpty) {
            missing.push_back(field.title.empty() ? field.name : field.title);
        }
    }

    return missing;
}

void RecordFieldForm::openRelationPicker(FieldRow &row) {
    auto *dlg = new RelationPickerDialog(client_, row.field->relationEvidence, company_);
    const ushort code = TProgram::deskTop->execView(dlg);

    if (code == cmOK) {
        row.pickedValue = dlg->selectedValue();
        row.hasPickedValue = true;

        const std::string display = jsonShowAs(row.pickedValue);
        auto *text = static_cast<TStaticText *>(row.widget);
        TRect bounds = text->getBounds();
        remove(text);
        delete text;
        auto *newText = new TStaticText(bounds, display.c_str());
        insert(newText);
        row.widget = newText;
        row.originalText = display;
    }

    TObject::destroy(dlg);
}

void RecordFieldForm::draw() {
    // Plain TGroup subclasses don't set ofBuffered and so never clear their
    // own area - TGroup::draw() only draws subviews (buttons, labels,
    // inputs), leaving whatever was on screen before showing through the
    // gaps between them. Paint the dialog's own body color across the
    // whole area first (same technique tvision's own TBackground uses),
    // then let the normal subview drawing happen on top of it.
    TDrawBuffer buffer;
    const TColorAttr color = getColor(0x01);

    for (short y = 0; y < size.y; ++y) {
        buffer.moveChar(0, ' ', color, static_cast<ushort>(size.x));
        writeLine(0, y, static_cast<ushort>(size.x), 1, buffer);
    }

    TGroup::draw();
}

void RecordFieldForm::handleEvent(TEvent &event) {
    TGroup::handleEvent(event);

    if (event.what != evCommand) {
        return;
    }

    switch (event.message.command) {
    case cmFieldFormPrevPage:
        showPage(page_ - 1);
        clearEvent(event);
        break;

    case cmFieldFormNextPage:
        showPage(page_ + 1);
        clearEvent(event);
        break;

    case cmFieldFormToggleRaw:
        toggleRaw();
        clearEvent(event);
        break;

    case cmFieldFormPickRelation:
        for (auto &row : pageRows_) {
            if (row.pickButton == static_cast<TButton *>(event.message.infoPtr)) {
                openRelationPicker(row);
                break;
            }
        }

        clearEvent(event);
        break;

    case cmFormatCode:
        if (rawMode_ && rawEditor_ != nullptr) {
            std::string error;

            if (!formatEditorText(*rawEditor_, false, error)) {
                messageBox(error.empty() ? std::string("Could not format JSON") : error, mfError | mfOKButton);
            }

            clearEvent(event);
        }

        break;

    default:
        break;
    }
}

} // namespace abraflexitui
