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

namespace abraflexitui {

AbraFlexiApp::AbraFlexiApp()
    : TProgInit(&AbraFlexiApp::initStatusLine, &AbraFlexiApp::initMenuBar, &AbraFlexiApp::initDeskTop) {
    statusLine_ = dynamic_cast<AppStatusLine *>(statusLine);
}

void AbraFlexiApp::configure(std::string cliBinary, std::string envFile, ProfileStore store) {
    client_.setBinaryPath(std::move(cliBinary));
    client_.setEnvFile(std::move(envFile));
    store_ = std::move(store);
    client_.setRequestObserver([this](const std::string &url) {
        if (statusLine_ != nullptr) {
            statusLine_->setCurrentUrl(url);
        }
    });
    applyActiveProfile();

    if (store_.active() != nullptr) {
        pendingStatusDialog_ = true;
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

void AbraFlexiApp::idle() {
    TApplication::idle();

    if (pendingServerDialog_) {
        openServerConfig();
    }

    if (pendingStatusDialog_) {
        openStatus();
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
               *new TMenuItem("Chan~g~es...", cmShowChanges, kbAltG) + newLine() +
               *new TMenuItem("E~x~it", cmQuit, cmQuit, hcNoContext, "Alt-X") +
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
        EvidenceListView *win = new EvidenceListView(client_);
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
        executeDialog(new SearchView(client_));
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

    case cmOpenRecordList: {
        auto *evidence = static_cast<nlohmann::json *>(event.message.infoPtr);

        if (evidence != nullptr) {
            std::string path = jsonField(*evidence, "path");

            if (!path.empty()) {
                RecordListView *win = new RecordListView(client_, path);
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

} // namespace abraflexitui
