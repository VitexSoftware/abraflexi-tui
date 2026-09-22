#include "abraflexitui/TV.h"
#include "abraflexitui/RecordDetailView.h"
#include "abraflexitui/JsonFormat.h"
#include "abraflexitui/WindowLayout.h"

#include <algorithm>
#include <sstream>

namespace abraflexitui {

RecordDetailView::RecordDetailView(const TRect &bounds, TScrollBar *vScrollBar) noexcept
    : SimpleListViewer(bounds, vScrollBar) {
}

void RecordDetailView::showMessage(const std::string &text) {
    setRows({text});
}

namespace {

// True/false spelled out the way the field is actually shown: AbraFlexi's
// "logic" fields round-trip either as JSON booleans or as the "true"/
// "false" strings the offline/online properties schema itself uses.
bool isLogicValue(const nlohmann::json &value) {
    return value.is_boolean() || value == "true" || value == "false";
}

bool logicIsTrue(const nlohmann::json &value) {
    return value.is_boolean() ? value.get<bool>() : value == "true";
}

void appendField(std::vector<std::string> &rows, const std::string &label, const nlohmann::json &value,
                  const FieldSchema *field) {
    if (value.is_object() || value.is_array()) {
        std::string pretty = value.dump(2);
        std::istringstream iss(pretty);
        std::string line;
        bool first = true;

        while (std::getline(iss, line)) {
            if (first) {
                rows.push_back(label + ": " + line);
                first = false;
            } else {
                rows.push_back("  " + line);
            }
        }

        if (first) {
            rows.push_back(label + ": " + pretty);
        }

        return;
    }

    if (field != nullptr && field->type == "logic" && isLogicValue(value)) {
        rows.push_back(label + ": " + (logicIsTrue(value) ? "Ano" : "Ne"));
        return;
    }

    std::string v = value.is_null() ? std::string() : (value.is_string() ? value.get<std::string>() : value.dump());

    if (field != nullptr && field->type == "relation" && !v.empty() && !field->relationEvidence.empty()) {
        v += " (\xE2\x86\x92 " + field->relationEvidence + ")";
    }

    rows.push_back(label + ": " + v);
}

} // namespace

void RecordDetailView::showRecord(const nlohmann::json &record, const std::vector<FieldSchema> *schema) {
    std::vector<std::string> rows;

    if (!record.is_object()) {
        rows.push_back(record.dump());
        setRows(std::move(rows));
        return;
    }

    std::vector<std::string> handled;

    if (schema != nullptr) {
        for (const auto &field : *schema) {
            if (!field.visible || !record.contains(field.name)) {
                continue;
            }

            handled.push_back(field.name);
            appendField(rows, field.title.empty() ? field.name : field.title, record.at(field.name), &field);
        }
    }

    for (auto it = record.begin(); it != record.end(); ++it) {
        if (std::find(handled.begin(), handled.end(), it.key()) != handled.end()) {
            continue;
        }

        appendField(rows, it.key(), it.value(), nullptr);
    }

    if (rows.empty()) {
        rows.push_back("(empty record)");
    }

    setRows(std::move(rows));
}

namespace {

TRect cascadedRecordRect() {
    TRect desk = TProgram::deskTop->getExtent();
    short count = 0;
    TProgram::deskTop->forEach(
        [](TView *view, void *arg) {
            if (dynamic_cast<RecordWindow *>(view) != nullptr) {
                ++*static_cast<short *>(arg);
            }
        },
        &count);

    const short shift = static_cast<short>(count % 8);
    const short maxW = desk.b.x - desk.a.x;
    const short maxH = desk.b.y - desk.a.y;
    short width = maxW > 72 ? static_cast<short>(maxW / 2) : static_cast<short>(maxW - shift);
    short height = static_cast<short>(maxH - shift);

    if (width < 24) {
        width = maxW;
    }

    if (height < 8) {
        height = maxH;
    }

    short x = shift;
    short y = shift;

    if (x + width > desk.b.x) {
        x = std::max<short>(0, static_cast<short>(desk.b.x - width));
    }

    if (y + height > desk.b.y) {
        y = std::max<short>(0, static_cast<short>(desk.b.y - height));
    }

    return TRect(x, y, static_cast<short>(x + width), static_cast<short>(y + height));
}

std::string recordTitle(const std::string &evidence, const std::string &id, const nlohmann::json &record) {
    std::string label = jsonField(record, "kod");

    if (label.empty()) {
        label = jsonField(record, "nazev");
    }

    if (label.empty()) {
        label = id;
    }

    return evidence + " " + label;
}

} // namespace

RecordWindow::RecordWindow(CliClient &client, const std::string &evidence, const std::string &id,
                            const nlohmann::json &record, std::string company)
    : TWindowInit(&TWindow::initFrame),
      TWindow(cascadedRecordRect(),
              (recordTitle(evidence, id, record) + " [" + (company.empty() ? client.company() : company) + "]").c_str(),
              wnNoNumber),
      evidence_(evidence), id_(id), company_(company.empty() ? client.company() : std::move(company)) {
    options |= ofTileable;
    growMode = gfGrowHiX | gfGrowHiY;

    const std::vector<FieldSchema> &schema = EvidenceSchema::fetch(client, evidence_, company_);
    TScrollBar *bar = standardScrollBar(sbVertical | sbHandleKeyboard);
    RecordDetailView *detail = new RecordDetailView(getExtent().grow(-1, -1), bar);
    growFill(detail);
    detail->showRecord(record, &schema);
    insert(detail);
}

} // namespace abraflexitui
