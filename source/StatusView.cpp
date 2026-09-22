#include "abraflexitui/TV.h"
#include "abraflexitui/StatusView.h"
#include "abraflexitui/WindowLayout.h"

#include <string>
#include <vector>

namespace abraflexitui {

namespace {

std::string fieldStr(const nlohmann::json &j, const char *key) {
    if (!j.contains(key) || j.at(key).is_null()) {
        return std::string();
    }

    const auto &v = j.at(key);
    return v.is_string() ? v.get<std::string>() : v.dump();
}

std::string pad(const std::string &label) {
    std::string out = label;
    while (out.size() < 16) {
        out += ' ';
    }
    return out;
}

} // namespace

StatusView::StatusView(CliClient &client)
    : TWindowInit(&TDialog::initFrame),
      TDialog(TRect(20, 4, 62, 18), "AbraFlexi Status") {
    options |= ofCentered;
    makeMaximizable(*this);

    CliClient::Result result = client.runJson({"status"});

    std::vector<std::string> lines;

    if (!result.ok) {
        lines.push_back("Error:");
        lines.push_back(result.errorMessage.empty() ? "unknown error" : result.errorMessage);
    } else {
        lines.push_back(pad("URL:") + fieldStr(result.data, "url"));
        lines.push_back(pad("User:") + fieldStr(result.data, "user"));
        lines.push_back(pad("Company:") + fieldStr(result.data, "company"));
        lines.push_back(pad("Company Name:") + fieldStr(result.data, "companyName"));
        lines.push_back(pad("Company DB:") + fieldStr(result.data, "companyDb"));
        lines.push_back(pad("Company State:") + fieldStr(result.data, "companyState"));
        lines.push_back(std::string());
        lines.push_back("Server and company are reachable.");
    }

    short y = 2;

    for (const auto &line : lines) {
        TView *row = new TStaticText(TRect(2, y, size.x - 2, y + 1), line.c_str());
        growWide(row);
        insert(row);
        ++y;
    }

    TView *ok = new TButton(TRect((size.x - 12) / 2, size.y - 3, (size.x - 12) / 2 + 12, size.y - 1), "O~K~", cmOK,
                            bfDefault);
    stickBottom(ok);
    insert(ok);

    selectNext(False);
}

} // namespace abraflexitui
