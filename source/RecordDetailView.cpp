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

void RecordDetailView::showRecord(const nlohmann::json &record) {
    std::vector<std::string> rows;

    if (!record.is_object()) {
        rows.push_back(record.dump());
        setRows(std::move(rows));
        return;
    }

    for (auto it = record.begin(); it != record.end(); ++it) {
        const std::string &key = it.key();
        const nlohmann::json &value = it.value();

        if (value.is_object() || value.is_array()) {
            std::string pretty = value.dump(2);
            std::istringstream iss(pretty);
            std::string line;
            bool first = true;

            while (std::getline(iss, line)) {
                if (first) {
                    rows.push_back(key + ": " + line);
                    first = false;
                } else {
                    rows.push_back("  " + line);
                }
            }

            if (first) {
                rows.push_back(key + ": " + pretty);
            }
        } else {
            std::string v = value.is_null()
                                 ? std::string()
                                 : (value.is_string() ? value.get<std::string>() : value.dump());
            rows.push_back(key + ": " + v);
        }
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

RecordWindow::RecordWindow(const std::string &evidence, const std::string &id, const nlohmann::json &record)
    : TWindowInit(&TWindow::initFrame),
      TWindow(cascadedRecordRect(), recordTitle(evidence, id, record), wnNoNumber),
      evidence_(evidence), id_(id) {
    options |= ofTileable;
    growMode = gfGrowHiX | gfGrowHiY;

    TScrollBar *bar = standardScrollBar(sbVertical | sbHandleKeyboard);
    RecordDetailView *detail = new RecordDetailView(getExtent().grow(-1, -1), bar);
    growFill(detail);
    detail->showRecord(record);
    insert(detail);
}

} // namespace abraflexitui
