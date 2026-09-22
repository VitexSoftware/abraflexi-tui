#pragma once

#include "abraflexitui/TV.h"
#include "abraflexitui/EvidenceSchema.h"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace abraflexitui {

// Generated, per-type field form for a record's *writable* schema fields
// (logic -> checkbox, everything else -> a labelled/length-limited text
// input), shared by RecordCreateForm and RecordEditForm. Evidences can
// have far more writable fields than fit on screen, and classic Turbo
// Vision has no primitive for scrolling a group of arbitrary child views
// (TScroller only redraws a single view's own content, e.g. TMemo/TEditor
// text) - so fields are paged instead, with Prev/Next controls, the same
// approach classic multi-field TUI forms use in place of scrolling.
//
// A "Raw JSON" toggle swaps the whole paged area for a plain TMemo (the
// same JSON textarea this form replaces), which doubles as the automatic
// fallback when `schema` is empty (CLI/schema lookup failed): the form
// then behaves exactly like the old plain-JSON dialog.
class RecordFieldForm : public TGroup {
public:
    RecordFieldForm(const TRect &bounds, std::vector<FieldSchema> schema, nlohmann::json initialValues) noexcept;

    // Flushes whichever mode (fields or raw JSON) is currently active into
    // the accumulated value model and returns it - what submit() sends.
    nlohmann::json currentValues();

    // Schema titles (or raw names) of mandatory+writable fields that are
    // still empty in currentValues(); empty when nothing is missing. Only
    // meaningful when a schema was supplied (always empty otherwise).
    std::vector<std::string> missingMandatory();

    void handleEvent(TEvent &event) override;
    void draw() override;

private:
    void showPage(int page);
    void clearPageViews();
    void flushPageIntoModel();
    void toggleRaw();
    void updatePageLabel();
    void buildRawEditor(const std::string &initialText);
    void destroyRawEditor();
    std::string readRawText() const;

    std::vector<FieldSchema> fields_;
    nlohmann::json model_;
    TRect fieldsArea_;
    int page_ = 0;
    int rowsPerPage_ = 1;
    int pageCount_ = 1;
    bool rawMode_ = false;

    struct FieldRow {
        const FieldSchema *field;
        TStaticText *label;
        TView *widget;
        // What the widget was seeded with, to detect an actual edit. A
        // relation/nested field's original value can be a whole JSON
        // object; it is stringified into the text input for display, but
        // must not overwrite the model with that flattened string unless
        // the user actually changed it (otherwise every page visit would
        // silently replace the real relation object with its dumped-text
        // stand-in).
        std::string originalText;
        bool originalChecked = false;
    };
    std::vector<FieldRow> pageRows_;

    TStaticText *pageLabel_ = nullptr;
    TButton *prevBtn_ = nullptr;
    TButton *nextBtn_ = nullptr;
    TButton *rawToggleBtn_ = nullptr;
    TButton *formatBtn_ = nullptr;
    TMemo *rawEditor_ = nullptr;
    TScrollBar *rawScrollBar_ = nullptr;
};

} // namespace abraflexitui
