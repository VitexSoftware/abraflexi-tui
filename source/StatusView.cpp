#include "abraflexitui/TV.h"
#include "abraflexitui/AppButton.h"
#include "abraflexitui/StatusView.h"
#include "abraflexitui/WindowLayout.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace abraflexitui {

namespace {

bool containsCaseInsensitive(const std::string &haystack, const char *needle) {
    std::string lower = haystack;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
    return lower.find(needle) != std::string::npos;
}

// abraflexi-cli's status --format=json does not carry a machine-readable
// error kind (see BaseCommand::writeJsonError in abraflexi-cli), just a
// human-readable message string, so this pattern-matches the two cases
// StatusCommand.php actually produces: a dedicated "parameters are
// missing" message for an incomplete profile, and, for everything that
// gets far enough to hit the server, whatever AbraFlexi\Exception says -
// live-tested against the demo server, a bad password surfaces as
// "...Je potreba autorizace..." (Czech: "authorization is required"; the
// server also accepts English installs, hence the "unauthorized"/"401"/
// "credential" checks too), while a genuinely unreachable host surfaces as
// a curl-level error ("Could not resolve host", connection refused, ...).
bool looksLikeCredentialsProblem(const std::string &message) {
    // "parameters are missing" is StatusCommand's own wording (BaseCommand
    // has nothing configured at all yet) - also a credentials/setup
    // problem, not a network one, so it gets the same orange treatment.
    static const char *needles[] = {"autorizace", "unauthorized", "401", "credential", "authent", "missing"};

    for (const char *needle : needles) {
        if (containsCaseInsensitive(message, needle)) {
            return true;
        }
    }

    return false;
}

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

    if (result.ok) {
        signal_ = Signal::Ok;
    } else if (looksLikeCredentialsProblem(result.errorMessage)) {
        signal_ = Signal::BadCredentials;
    } else {
        signal_ = Signal::Unreachable;
    }

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

    TView *ok = new AppButton(TRect((size.x - 12) / 2, size.y - 3, (size.x - 12) / 2 + 12, size.y - 1), "O~K~", cmOK,
                            bfDefault);
    stickBottom(ok);
    insert(ok);

    selectNext(False);
}

// Same "don't trust the resolved palette color under an arbitrary terminal
// theme" reasoning as AppButton: this dialog is a reassuring "everything is
// fine" (green) or "can't reach the server" (red) signal at a glance, so
// its background must stay that color regardless of what the terminal's
// ANSI palette does with tvision's default dialog colors (reported: showed
// up plain white). Frame/StaticText/Label indices (see TDialog's palette
// layout in dialogs.h) are overridden directly with explicit RGB; Button
// indices (10+) are left alone since AppButton already colors itself and
// falls through to here only for its shadow, which should legitimately
// match this dialog's own background.
TColorAttr StatusView::mapColor(uchar index) {
    uint32_t bg = 0x6B1E1E; // Unreachable: red.

    if (signal_ == Signal::Ok) {
        bg = 0x1E5C34; // Green.
    } else if (signal_ == Signal::BadCredentials) {
        bg = 0x8A5A12; // Orange.
    }

    const uint32_t fg = 0xF0F0F0;

    switch (index) {
    case 1: // Frame passive
    case 2: // Frame active
    case 3: // Frame icon
    case 6: // StaticText
    case 7: // Label normal
    case 8: // Label selected
    case 9: // Label shortcut
    case 15: // Button shadow (AppButton deliberately falls through to here)
        return TColorAttr(TColor(TColorRGB(fg)), TColor(TColorRGB(bg)));
    default:
        return TDialog::mapColor(index);
    }
}

} // namespace abraflexitui
