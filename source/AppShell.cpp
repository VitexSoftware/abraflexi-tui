#include "abraflexitui/TV.h"
#include "abraflexitui/AppShell.h"
#include "abraflexitui/Commands.h"
#include "abraflexitui/JsonFormat.h"
#include "abraflexitui/ServerConfigView.h"
#include "abraflexitui/StatusView.h"
#include "abraflexitui/CompanyListView.h"
#include "abraflexitui/EvidenceListView.h"
#include "abraflexitui/RecordListView.h"
#include "abraflexitui/QueryView.h"
#include "abraflexitui/SearchView.h"
#include "abraflexitui/ChangesView.h"
#include "abraflexitui/AboutView.h"
#include "abraflexitui/QrCodeView.h"
#include "abraflexitui/DisplayUrl.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace abraflexitui {

AbraFlexiApp::AbraFlexiApp()
    : TProgInit(&AbraFlexiApp::initStatusLine, &AbraFlexiApp::initMenuBar, &AbraFlexiApp::initDeskTop) {
    statusLine_ = dynamic_cast<AppStatusLine *>(statusLine);
}

void AbraFlexiApp::configure(std::string cliBinary, std::string envFile, ProfileStore store) {
    client_.setBinaryPath(std::move(cliBinary));
    client_.setEnvFile(std::move(envFile));
    store_ = std::move(store);
    session_.load();
    client_.setRequestObserver([this](const std::string &url) {
        if (statusLine_ != nullptr) {
            statusLine_->setCurrentUrl(url);
        }
    });
    applyActiveProfile();

    if (store_.active() != nullptr) {
        pendingStatusDialog_ = true;
        pendingSessionRestore_ = true;
    }
}

void AbraFlexiApp::applyActiveProfile() {
    const ServerProfile *active = store_.active();

    if (active != nullptr) {
        offerServerChoice_ = true;
        client_.setProfileEnvironment(active->url, active->company, active->processEnvironment());

        if (statusLine_ != nullptr) {
            statusLine_->setCompany(active->company);
            showQueryMode();
            statusLine_->setCurrentUrl(active->url);
        }
    } else {
        client_.clearProfileEnvironment();

        if (statusLine_ != nullptr) {
            statusLine_->setCompany(std::string());
            showQueryMode();
            statusLine_->setCurrentUrl(std::string());
        }

        if (offerServerChoice_ && !serverDialogOpen_) {
            offerServerChoice_ = false;
            pendingServerDialog_ = true;
        }
    }
}

void AbraFlexiApp::showQueryMode() {
    if (statusLine_ == nullptr) {
        return;
    }

    statusLine_->setQueryMode(store_.queryFormat() == ProfileStore::QueryFormat::Xml ? "XML" : "JSON");
}

void AbraFlexiApp::chooseCompany(const std::string &company) {
    const ServerProfile *active = store_.active();

    if (active == nullptr) {
        messageBox("Choose a server profile before selecting a company.", mfError | mfOKButton);
        return;
    }

    ServerProfile updated = *active;
    updated.company = company;
    store_.update(active->name, std::move(updated));
    std::string error;

    if (!store_.save(error)) {
        messageBox(error.empty() ? std::string("Could not save the company") : error, mfError | mfOKButton);
        return;
    }

    applyActiveProfile();
}

void AbraFlexiApp::openServerConfig() {
    if (serverDialogOpen_) {
        return;
    }

    pendingServerDialog_ = false;
    serverDialogOpen_ = true;
    executeDialog(new ServerConfigView(store_, client_, [this]() { applyActiveProfile(); }));
    serverDialogOpen_ = false;
}

void AbraFlexiApp::openStatus() {
    if (statusDialogOpen_) {
        return;
    }

    pendingStatusDialog_ = false;
    statusDialogOpen_ = true;
    executeDialog(new StatusView(client_));
    statusDialogOpen_ = false;
}

void AbraFlexiApp::openWebQr() {
    const std::string web = webInterfaceUrl(statusLine_ == nullptr ? std::string() : statusLine_->currentUrl());

    if (web.empty()) {
        messageBox("The status line has no web address to show.", mfError | mfOKButton);
        return;
    }

    try {
        executeDialog(new QrCodeDialog(web));
    } catch (const std::exception &error) {
        messageBox(error.what(), mfError | mfOKButton);
    }
}

void AbraFlexiApp::idle() {
    TApplication::idle();

    if (pendingServerDialog_) {
        openServerConfig();
    }

    // openStatus() below is a modal executeDialog() call: it runs its own
    // nested event loop, which calls this same idle() method again while
    // still "inside" the outer call. Only check pendingSessionRestore_
    // AFTER openStatus() has actually returned (dialog dismissed), or a
    // restored window could get inserted underneath/alongside the still-open
    // modal status check instead of after it, as happened when both flags
    // were checked unconditionally in the same idle() tick.
    if (pendingStatusDialog_) {
        openStatus();
    }

    if (!statusDialogOpen_ && !serverDialogOpen_ && pendingSessionRestore_) {
        pendingSessionRestore_ = false;
        restoreSessionWindows();
    }

    updateWindowCommands();
}

void AbraFlexiApp::restoreSessionWindows() {
    // Re-open whatever record-list windows were open last time, each back
    // on its previously selected record and at its previous position, so
    // restarting the app returns to the same documents instead of an empty
    // desktop.
    //
    // Snapshot first: each RecordListView constructed below re-registers
    // itself via session_.openWindow(), which appends to the very vector
    // session_.windows() returns a reference to - iterating that live
    // vector while it can reallocate under us would be undefined behavior.
    // The stale entries this snapshot is replacing are removed as each new
    // window's own destructor eventually calls session_.closeWindow(), same
    // as any other RecordListView.
    const std::vector<WindowSession> saved = session_.windows();

    // Drop the loaded entries before re-opening: each new RecordListView
    // below registers its own fresh entry, and leaving the old ones in
    // place too would double the remembered window count on every restart.
    for (const WindowSession &win : saved) {
        session_.closeWindow(win.id);
    }

    for (const WindowSession &win : saved) {
        const WindowBounds *bounds = win.hasBounds ? &win.bounds : nullptr;
        TProgram::deskTop->insert(
            new RecordListView(client_, session_, win.evidence, "id,kod,nazev", 20, win.focusedId, bounds));
    }
}

TMenuBar *AbraFlexiApp::initMenuBar(TRect r) {
    r.b.y = r.a.y + 1;

    return new TMenuBar(
        r, *new TSubMenu("~A~braFlexi", kbAltA) + *new TMenuItem("~S~tatus...", cmShowStatus, kbAltS) +
               *new TMenuItem("~C~ompanies...", cmShowCompanies, kbAltC) +
               *new TMenuItem("~E~vidences...", cmShowEvidences, kbAltE) +
               *new TMenuItem("Ser~v~ers...", cmShowServerConfig, kbAltV) +
               *new TMenuItem("~Q~uery...", cmShowQuery, kbAltQ) +
               *new TMenuItem("~F~ind...", cmShowSearch, kbAltF) +
               *new TMenuItem("Chan~g~es...", cmShowChanges, kbAltG) +
               *new TMenuItem("~B~rowser QR...", cmShowWebQr, kbAltB, hcNoContext, "Alt-B") + newLine() +
               *new TMenuItem("E~x~it", cmQuit, cmQuit, hcNoContext, "Alt-X") +
           *new TSubMenu("~W~indow", kbAltW) +
               *new TMenuItem("~S~ize/move", cmResize, kbCtrlF5, hcNoContext, "Ctrl-F5") +
               *new TMenuItem("~Z~oom", cmZoom, kbNoKey) +
               *new TMenuItem("~T~ile", cmTile, kbNoKey) +
               *new TMenuItem("C~a~scade", cmCascade, kbNoKey) +
               *new TMenuItem("~N~ext", cmNext, kbF6, hcNoContext, "F6") +
               *new TMenuItem("~P~revious", cmPrev, kbShiftF6, hcNoContext, "Shift-F6") +
               *new TMenuItem("~M~inimize all", cmMinimizeAll, kbNoKey) +
               *new TMenuItem("~R~estore", cmRestoreWindows, kbNoKey) + newLine() +
               *new TMenuItem("~C~lose", cmClose, kbAltF3, hcNoContext, "Alt-F3") +
               *new TMenuItem("Close a~l~l", cmCloseAll, kbNoKey) +
           *new TSubMenu("~H~elp", kbAltH) + *new TMenuItem("~A~bout...", cmShowAbout, kbF1, hcNoContext, "F1"));
}

TStatusLine *AbraFlexiApp::initStatusLine(TRect r) {
    r.a.y = r.b.y - 1;

    return new AppStatusLine(r, *new TStatusDef(0, 0xFFFF) + *new TStatusItem("~F1~ About", kbF1, cmShowAbout) +
                                      *new TStatusItem("~Alt-X~ Exit", kbAltX, cmQuit) +
                                      *new TStatusItem("~F10~ Menu", kbF10, cmMenu));
}

void AbraFlexiApp::handleEvent(TEvent &event) {
    TApplication::handleEvent(event);

    if (event.what != evCommand) {
        return;
    }

    switch (event.message.command) {
    case cmShowStatus: {
        openStatus();
        clearEvent(event);
        break;
    }

    case cmShowCompanies: {
        CompanyListView *win = new CompanyListView(client_, [this](const std::string &company) { chooseCompany(company); });
        deskTop->insert(win);
        clearEvent(event);
        break;
    }

    case cmShowEvidences: {
        EvidenceListView *win = new EvidenceListView(client_, session_);
        deskTop->insert(win);
        clearEvent(event);
        break;
    }

    case cmShowServerConfig: {
        openServerConfig();
        clearEvent(event);
        break;
    }

    case cmShowQuery: {
        executeDialog(new QueryView(client_, store_, [this]() { showQueryMode(); }));
        clearEvent(event);
        break;
    }

    case cmShowSearch: {
        executeDialog(new SearchView(client_, session_));
        clearEvent(event);
        break;
    }

    case cmShowChanges: {
        executeDialog(new ChangesView(client_));
        clearEvent(event);
        break;
    }

    case cmShowAbout: {
        executeDialog(new AboutView());
        clearEvent(event);
        break;
    }

    case cmShowWebQr: {
        openWebQr();
        clearEvent(event);
        break;
    }

    case cmMinimizeAll: {
        minimizeAll();
        clearEvent(event);
        break;
    }

    case cmRestoreWindows: {
        restoreWindows();
        clearEvent(event);
        break;
    }

    case cmCloseAll: {
        closeAllWindows();
        clearEvent(event);
        break;
    }

    case cmOpenRecordList: {
        auto *evidence = static_cast<nlohmann::json *>(event.message.infoPtr);

        if (evidence != nullptr) {
            std::string path = jsonField(*evidence, "path");

            if (!path.empty()) {
                RecordListView *win = new RecordListView(client_, session_, path);
                deskTop->insert(win);
            }
        }

        clearEvent(event);
        break;
    }

    default:
        break;
    }
}

namespace {

bool isArrangeable(TView *view) {
    return (view->options & ofTileable) != 0 && (view->state & sfVisible) != 0 && (view->state & sfModal) == 0;
}

void collectArrangeable(TView *view, void *arg) {
    if (isArrangeable(view)) {
        static_cast<std::vector<TWindow *> *>(arg)->push_back(static_cast<TWindow *>(view));
    }
}

void collectLive(TView *view, void *arg) {
    static_cast<std::vector<TView *> *>(arg)->push_back(view);
}

} // namespace

void AbraFlexiApp::updateWindowCommands() {
    if (deskTop == nullptr) {
        return;
    }

    std::vector<TView *> live;
    deskTop->forEach(collectLive, &live);
    minimized_.erase(std::remove_if(minimized_.begin(), minimized_.end(),
                                     [&live](const MinimizedWindow &saved) {
                                         return std::find(live.begin(), live.end(), saved.window) == live.end();
                                     }),
                     minimized_.end());

    std::vector<TWindow *> open;
    deskTop->forEach(collectArrangeable, &open);
    const Boolean hasWindows = open.empty() ? False : True;
    const Boolean hasMinimized = minimized_.empty() ? False : True;

    if (hasWindows) {
        enableCommand(cmTile);
        enableCommand(cmCascade);
        enableCommand(cmCloseAll);
        enableCommand(cmMinimizeAll);
    } else {
        disableCommand(cmTile);
        disableCommand(cmCascade);
        disableCommand(cmCloseAll);
        disableCommand(cmMinimizeAll);
    }

    if (hasMinimized) {
        enableCommand(cmRestoreWindows);
    } else {
        disableCommand(cmRestoreWindows);
    }
}

void AbraFlexiApp::minimizeAll() {
    std::vector<TWindow *> open;
    deskTop->forEach(collectArrangeable, &open);

    TRect desk = deskTop->getExtent();
    short x = 0;
    short y = static_cast<short>(desk.b.y - 2);
    const short width = 22;

    deskTop->lock();

    for (TWindow *window : open) {
        if (x + width > desk.b.x) {
            x = 0;
            y = static_cast<short>(y - 2);
        }

        if (y < desk.a.y) {
            break;
        }

        MinimizedWindow saved;
        saved.window = window;
        saved.bounds = window->getBounds();
        window->options &= ~ofTileable;
        TRect bar(x, y, static_cast<short>(x + width), static_cast<short>(y + 2));
        window->locate(bar);
        minimized_.push_back(saved);
        x = static_cast<short>(x + width);
    }

    deskTop->unlock();
}

void AbraFlexiApp::restoreWindows() {
    std::vector<TView *> live;
    deskTop->forEach(collectLive, &live);
    deskTop->lock();

    for (const MinimizedWindow &saved : minimized_) {
        if (std::find(live.begin(), live.end(), saved.window) == live.end()) {
            continue;
        }

        saved.window->options |= ofTileable;
        TRect bounds = saved.bounds;
        saved.window->locate(bounds);
    }

    minimized_.clear();
    deskTop->unlock();
}

void AbraFlexiApp::closeAllWindows() {
    std::vector<TWindow *> open;
    deskTop->forEach(collectArrangeable, &open);
    std::vector<TWindow *> minimized;

    for (const MinimizedWindow &saved : minimized_) {
        minimized.push_back(saved.window);
    }

    for (TWindow *window : open) {
        message(window, evCommand, cmClose, window);
    }

    for (TWindow *window : minimized) {
        message(window, evCommand, cmClose, window);
    }

    minimized_.clear();
}

} // namespace abraflexitui
