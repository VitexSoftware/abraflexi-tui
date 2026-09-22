#include "abraflexitui/TV.h"
#include "abraflexitui/AppButton.h"
#include "abraflexitui/AboutView.h"
#include "abraflexitui/CliClient.h"
#include "abraflexitui/WindowLayout.h"

#include <string>
#include <vector>

namespace abraflexitui {

namespace {

constexpr unsigned short cmAboutOpen = 1038;

struct Link {
    const char *name;
    const char *url;
};

// Direct build and runtime dependencies, plus the AbraFlexi library the CLI calls.
constexpr Link kLinks[] = {
    {"abraflexi-tui", "https://github.com/VitexSoftware/abraflexi-tui"},
    {"abraflexi-cli", "https://github.com/VitexSoftware/abraflexi-cli"},
    {"php-abraflexi", "https://github.com/Spoje-NET/php-abraflexi"},
    {"Turbo Vision", "https://github.com/magiblot/tvision"},
    {"nlohmann/json", "https://github.com/nlohmann/json"},
    {"ncurses", "https://invisible-island.net/ncurses/"},
};

std::string padName(const char *name) {
    std::string out = name;

    while (out.size() < 16) {
        out += ' ';
    }

    return out;
}

} // namespace

AboutView::AboutView()
    : TWindowInit(&TDialog::initFrame), TDialog(TRect(4, 2, 76, 20), "About abraflexi-tui") {
    options |= ofCentered;
    makeMaximizable(*this);

    insert(new TStaticText(TRect(2, 2, 70, 3), "abraflexi-tui 1.0.0    MIT    Vitex Software"));
    insert(new TStaticText(TRect(2, 3, 70, 4), "Enter opens the selected link in a browser."));

    std::vector<std::string> rows;
    rows.reserve(sizeof(kLinks) / sizeof(kLinks[0]));

    for (const Link &link : kLinks) {
        rows.push_back(padName(link.name) + link.url);
        urls_.push_back(link.url);
    }

    TScrollBar *bar = standardScrollBar(sbVertical | sbHandleKeyboard);
    links_ = new SimpleListViewer(TRect(2, 5, 70, 14), bar);
    growFill(links_);
    links_->setRows(std::move(rows));
    insert(links_);

    TView *open = new AppButton(TRect(2, 15, 16, 17), "~O~pen", cmAboutOpen, bfDefault);
    stickBottom(open);
    insert(open);
    TView *close = new AppButton(TRect(54, 15, 70, 17), "Close", cmCancel, bfNormal);
    stickCorner(close);
    insert(close);
    selectNext(False);
}

void AboutView::openSelected() {
    const short index = links_->focused;

    if (index < 0 || static_cast<std::size_t>(index) >= urls_.size()) {
        return;
    }

    const std::string &url = urls_[static_cast<std::size_t>(index)];
    ProcessResult opened = ProcessRunner::run({"xdg-open", url});

    if (opened.spawnFailed || opened.exitCode != 0) {
        messageBox("Could not open the link. xdg-open is not available.", mfError | mfOKButton);
    }
}

void AboutView::handleEvent(TEvent &event) {
    TDialog::handleEvent(event);

    if (event.what == evCommand && event.message.command == cmAboutOpen) {
        openSelected();
        clearEvent(event);
    }
}

} // namespace abraflexitui
