#include "abraflexitui/TV.h"
#include "abraflexitui/AppButton.h"
#include "abraflexitui/RecordFieldForm.h"
#include "abraflexitui/Commands.h"
#include "abraflexitui/CodeFormat.h"
#include "abraflexitui/WindowLayout.h"

#include <algorithm>
#include <cstring>

namespace abraflexitui {

namespace {

constexpr short kLabelWidth = 24;
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
    } else if (field.type == "date" || field.type == "datetime") {
        label += " (YYYY-MM-DD)";
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

std::string jsonValueAsText(const nlohmann::json &value) {
    if (value.is_null()) {
        return std::string();
    }

    return value.is_string() ? value.get<std::string>() : value.dump();
}

} // namespace

RecordFieldForm::RecordFieldForm(const TRect &bounds, std::vector<FieldSchema> schema,
                                  nlohmann::json initialValues) noexcept
    : TGroup(bounds), fields_(orderedWritableFields(schema)), model_(std::move(initialValues)) {
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

    fieldsArea_ = TRect(x, static_cast<short>(top + 2), right, inner.b.y);
    rowsPerPage_ = std::max<int>(1, fieldsArea_.b.y - fieldsArea_.a.y);
    pageCount_ = fields_.empty() ? 1 : static_cast<int>((fields_.size() + rowsPerPage_ - 1) / rowsPerPage_);

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

            widget = box;
        } else {
            const int maxLen = field.maxLength > 0 ? std::min(field.maxLength, kMaxLenCap) : kDefaultMaxLen;
            auto *input = new TInputLine(TRect(inputX, y, fieldsArea_.b.x, static_cast<short>(y + 1)), maxLen);

            if (model_.contains(field.name)) {
                originalText = jsonValueAsText(model_.at(field.name));
                setInputText(input, originalText);
            }

            widget = input;
        }

        insert(widget);
        pageRows_.push_back({&field, label, widget, originalText, originalChecked});
        y = static_cast<short>(y + 1);
    }

    updatePageLabel();
}

void RecordFieldForm::clearPageViews() {
    for (auto &row : pageRows_) {
        remove(row.label);
        delete row.label;
        remove(row.widget);
        delete row.widget;
    }

    pageRows_.clear();
}

void RecordFieldForm::flushPageIntoModel() {
    for (auto &row : pageRows_) {
        if (row.field->type == "logic") {
            const bool checked = static_cast<TCheckBoxes *>(row.widget)->mark(0) == True;

            if (checked != row.originalChecked) {
                model_[row.field->name] = checked;
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
