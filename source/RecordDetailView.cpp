#include "abraflexitui/TV.h"
#include "abraflexitui/RecordDetailView.h"

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

} // namespace abraflexitui
