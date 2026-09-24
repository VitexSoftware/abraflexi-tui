#include "abraflexitui/TV.h"
#include "abraflexitui/AppShell.h"
#include "abraflexitui/Commands.h"
#include "abraflexitui/i18n.h"
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
#include "abraflexitui/GameWindow.h"
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

    // Must happen here, not before AbraFlexiApp is constructed: tvision's
    // own Platform::initLocale() runs lazily on first use (triggered during
    // construction) and calls setlocale(LC_ALL, "") itself, which would
    // silently undo an earlier setLanguage() call. Applying the saved
    // language only after construction, then rebuilding the menu bar/status
    // line that were already built (in the wrong language) during
    // construction, is the only ordering that sticks.
    if (!store_.language().empty()) {
        setLanguage(store_.language());
        reloadMenuAndStatusLine();
    }
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

    if (pendingMenuReload_) {
        pendingMenuReload_ = false;
        reloadMenuAndStatusLine();
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
            new RecordListView(client_, session_, win.evidence, "id,kod,nazev", 20, win.focusedId, bounds, win.company));
    }
}

TMenuBar *AbraFlexiApp::initMenuBar(TRect r) {
    r.b.y = r.a.y + 1;

    return new TMenuBar(
        r, *new TSubMenu(_("~C~ompany"), kbAltI) + *new TMenuItem(_("~S~tatus..."), cmShowStatus, kbAltS) +
               *new TMenuItem(_("~C~ompanies..."), cmShowCompanies, kbAltC) +
               *new TMenuItem(_("Ser~v~ers..."), cmShowServerConfig, kbAltV) + newLine() +
               *new TMenuItem(_("E~x~it"), cmQuit, cmQuit, hcNoContext, "Alt-X") +
           *new TSubMenu(_("~A~ddress book"), kbAltA) +
               *new TMenuItem(_("~A~ddresses"), cmOpenAdresy, kbNoKey) +
               *new TMenuItem(_("~C~ontacts"), cmOpenKontakty, kbNoKey) +
           *new TSubMenu(_("~S~ales"), kbAltR) +
               *new TMenuItem(_("~I~ssued invoices"), cmOpenFakturaVydana, kbNoKey) +
               *new TMenuItem(_("Received ~o~rders"), cmOpenObjednavkaPrijata, kbNoKey) +
               *new TMenuItem(_("Other ~r~eceivables"), cmOpenPohledavka, kbNoKey) +
           *new TSubMenu(_("~P~urchase"), kbAltK) +
               *new TMenuItem(_("Rece~i~ved invoices"), cmOpenFakturaPrijata, kbNoKey) +
               *new TMenuItem(_("~I~ssued orders"), cmOpenObjednavkaVydana, kbNoKey) +
               *new TMenuItem(_("Other ~l~iabilities"), cmOpenZavazek, kbNoKey) +
           *new TSubMenu(_("~G~oods"), kbAltZ) +
               *new TMenuItem(_("~P~rice list"), cmOpenCenik, kbNoKey) +
               *new TMenuItem(_("~S~tock cards"), cmOpenSkladovaKarta, kbNoKey) +
               *new TMenuItem(_("~W~arehouses"), cmOpenSklad, kbNoKey) +
               *new TMenuItem(_("Stock ~m~ovements"), cmOpenSkladovyPohyb, kbNoKey) +
           *new TSubMenu(_("~M~oney"), kbAltP) +
               *new TMenuItem(_("~B~ank"), cmOpenBanka, kbNoKey) +
               *new TMenuItem(_("Bank acc~o~unts"), cmOpenBankovniUcet, kbNoKey) +
               *new TMenuItem(_("Cas~h~ register"), cmOpenPokladna, kbNoKey) +
               *new TMenuItem(_("Cash doc~u~ments"), cmOpenPokladniPohyb, kbNoKey) +
           *new TSubMenu(_("Acco~u~nting"), kbAltU) +
               *new TMenuItem(_("~J~ournal"), cmOpenUcetniDenik, kbNoKey) +
               *new TMenuItem(_("Acc~o~unts"), cmOpenUcet, kbNoKey) +
               *new TMenuItem(_("Cost ~c~enters"), cmOpenStredisko, kbNoKey) +
               *new TMenuItem(_("~C~ontracts"), cmOpenZakazka, kbNoKey) +
               *new TMenuItem(_("Sa~l~do"), cmOpenSaldo, kbNoKey) +
           *new TSubMenu(_("~T~ools"), kbAltT) +
               *new TMenuItem(_("~E~vidence..."), cmShowEvidences, kbAltE) +
               *new TMenuItem(_("~F~ind..."), cmShowSearch, kbAltF) +
               *new TMenuItem(_("~Q~uery..."), cmShowQuery, kbAltQ) +
               *new TMenuItem(_("Chan~g~es..."), cmShowChanges, kbAltG) +
               *new TMenuItem(_("~B~rowser QR..."), cmShowWebQr, kbAltB, hcNoContext, "Alt-B") +
           *new TSubMenu(_("~W~indow"), kbAltW) +
               *new TMenuItem(_("~S~ize/move"), cmResize, kbCtrlF5, hcNoContext, "Ctrl-F5") +
               *new TMenuItem(_("~Z~oom"), cmZoom, kbNoKey) +
               *new TMenuItem(_("~T~ile"), cmTile, kbNoKey) +
               *new TMenuItem(_("C~a~scade"), cmCascade, kbNoKey) +
               *new TMenuItem(_("~N~ext"), cmNext, kbF6, hcNoContext, "F6") +
               *new TMenuItem(_("~P~revious"), cmPrev, kbShiftF6, hcNoContext, "Shift-F6") +
               *new TMenuItem(_("~M~inimize all"), cmMinimizeAll, kbNoKey) +
               *new TMenuItem(_("~R~estore"), cmRestoreWindows, kbNoKey) + newLine() +
               *new TMenuItem(_("~C~lose"), cmClose, kbAltF3, hcNoContext, "Alt-F3") +
               *new TMenuItem(_("Close a~l~l"), cmCloseAll, kbNoKey) +
           *new TSubMenu(_("~L~anguage"), kbAltL) +
               *new TMenuItem(_("~S~ystem default"), cmLangSystem, kbNoKey) + newLine() +
               *new TMenuItem("English", cmLangEnglish, kbNoKey) +
               *new TMenuItem("~Č~eština", cmLangCzech, kbNoKey) +
               *new TMenuItem("~D~eutsch", cmLangGerman, kbNoKey) +
           *new TSubMenu(_("~H~elp"), kbAltH) +
               *new TMenuItem(_("~G~ame..."), cmShowGame, kbNoKey) +
               *new TMenuItem(_("~A~bout..."), cmShowAbout, kbF1, hcNoContext, "F1"));
}

TStatusLine *AbraFlexiApp::initStatusLine(TRect r) {
    r.a.y = r.b.y - 1;

    return new AppStatusLine(r, *new TStatusDef(0, 0xFFFF) + *new TStatusItem(_("~F1~ About"), kbF1, cmShowAbout) +
                                      *new TStatusItem(_("~Alt-X~ Exit"), kbAltX, cmQuit) +
                                      *new TStatusItem(_("~F10~ Menu"), kbF10, cmMenu));
}

void AbraFlexiApp::selectLanguage(const std::string &lang) {
    setLanguage(lang);
    store_.setLanguage(lang);
    std::string error;

    if (!store_.save(error)) {
        messageBox(error.empty() ? std::string("Could not save the language") : error, mfError | mfOKButton);
    }

    pendingMenuReload_ = true;
}

void AbraFlexiApp::reloadMenuAndStatusLine() {
    // Only the menu bar and status line are rebuilt here: they are the two
    // views AppShell itself owns and constructs from _() calls. Already-open
    // windows/dialogs keep their old-language captions until closed and
    // reopened - tvision widgets store a copied label at construction time
    // with no mechanism to observe a later language change, and rebuilding
    // every open view from scratch would be far more invasive than this
    // menu-driven language switch warrants.
    TRect menuBounds = menuBar->getBounds();
    TRect statusBounds = statusLine->getBounds();

    remove(menuBar);
    delete menuBar;
    menuBar = initMenuBar(menuBounds);
    insert(menuBar);

    remove(statusLine);
    delete statusLine;
    statusLine = initStatusLine(statusBounds);
    statusLine_ = dynamic_cast<AppStatusLine *>(statusLine);
    insert(statusLine);

    redraw();
}

namespace {

struct QuickOpen {
    unsigned short cmd;
    const char *evidence;
    const char *columns;
};

// Menu-to-evidence wiring for the module menus' quick-open items (Adresář,
// Prodej, Nákup, Zboží, Peníze, Účetnictví). Each opens a RecordListView
// pinned to its evidence with a default column list suited to that document
// type, bypassing the schema's own "inSummary" columns (preferExplicitColumns
// below), since those are meant as reasonable defaults, not overridable
// hints.
constexpr QuickOpen quickOpens[] = {
    {cmOpenAdresy, "adresar", "kod,nazev,ic,dic,mesto,email"},
    {cmOpenKontakty, "kontakt", "kod,nazev,ic,dic,mesto,email"},
    {cmOpenFakturaVydana, "faktura-vydana", "kod,datVyst,datSplat,firma,sumCelkem,mena,stavUhrK"},
    {cmOpenObjednavkaPrijata, "objednavka-prijata", "kod,datVyst,datSplat,firma,sumCelkem,mena,stavUhrK"},
    {cmOpenPohledavka, "pohledavka", "kod,datVyst,datSplat,firma,sumCelkem,mena,stavUhrK"},
    {cmOpenFakturaPrijata, "faktura-prijata", "kod,datVyst,datSplat,cisDosle,firma,sumCelkem,stavUhrK"},
    {cmOpenObjednavkaVydana, "objednavka-vydana", "kod,datVyst,datSplat,cisDosle,firma,sumCelkem,stavUhrK"},
    {cmOpenZavazek, "zavazek", "kod,datVyst,datSplat,cisDosle,firma,sumCelkem,stavUhrK"},
    {cmOpenCenik, "cenik", "kod,nazev,eanKod,cenaZakl,nakupCena"},
    {cmOpenSkladovaKarta, "skladova-karta", "kod,nazev,eanKod,cenaZakl,nakupCena"},
    {cmOpenSklad, "sklad", "kod,nazev"},
    {cmOpenSkladovyPohyb, "skladovy-pohyb", "kod,datVyst,typPohybuK,sklad,firma,sumCelkem"},
    {cmOpenBanka, "banka", "kod,datVyst,typPohybuK,varSym,firma,sumCelkem,buc"},
    {cmOpenBankovniUcet, "bankovni-ucet", "kod,nazev,buc"},
    {cmOpenPokladna, "pokladna", "kod,nazev"},
    {cmOpenPokladniPohyb, "pokladni-pohyb", "kod,datVyst,typPohybuK,varSym,firma,sumCelkem"},
    {cmOpenUcetniDenik, "ucetni-denik", "kod,datUcPr,ucetMd,ucetD,stredisko,castka"},
    {cmOpenUcet, "ucet", "kod,nazev"},
    {cmOpenStredisko, "stredisko", "kod,nazev"},
    {cmOpenZakazka, "zakazka", "kod,nazev"},
    {cmOpenSaldo, "saldo", "firma,doklad,zbyvaUhrK"},
};

const QuickOpen *findQuickOpen(unsigned short cmd) {
    for (const QuickOpen &entry : quickOpens) {
        if (entry.cmd == cmd) {
            return &entry;
        }
    }

    return nullptr;
}

} // namespace

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

    case cmLangSystem: {
        selectLanguage(std::string());
        clearEvent(event);
        break;
    }

    case cmLangEnglish: {
        selectLanguage("en");
        clearEvent(event);
        break;
    }

    case cmLangCzech: {
        selectLanguage("cs");
        clearEvent(event);
        break;
    }

    case cmLangGerman: {
        selectLanguage("de");
        clearEvent(event);
        break;
    }

    case cmShowGame: {
        GameWindow *win = new GameWindow();
        deskTop->insert(win);
        // Only safe once win has an owner (deskTop) - see the comment in
        // GameWindow's constructor for why this can't happen any earlier.
        win->startGame();
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

    default: {
        if (const QuickOpen *entry = findQuickOpen(event.message.command)) {
            deskTop->insert(new RecordListView(client_, session_, entry->evidence, entry->columns, 20, "", nullptr,
                                                "", true));
            clearEvent(event);
        }

        break;
    }
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
