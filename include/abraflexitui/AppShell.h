#pragma once

#include "abraflexitui/TV.h"
#include "abraflexitui/AppStatusLine.h"
#include "abraflexitui/CliClient.h"
#include "abraflexitui/ProfileStore.h"
#include "abraflexitui/SessionStore.h"

#include <vector>

namespace abraflexitui {

class AbraFlexiApp : public TApplication {
public:
    AbraFlexiApp();

    void configure(std::string cliBinary, std::string envFile, ProfileStore store);

    static TMenuBar *initMenuBar(TRect r);
    static TStatusLine *initStatusLine(TRect r);

    void handleEvent(TEvent &event) override;
    void idle() override;

private:
    void applyActiveProfile();
    void chooseCompany(const std::string &company);
    void showQueryMode();
    void openServerConfig();
    void openStatus();
    void openWebQr();
    void restoreSessionWindows();
    void updateWindowCommands();
    void minimizeAll();
    void restoreWindows();
    void closeAllWindows();

    struct MinimizedWindow {
        TWindow *window;
        TRect bounds;
    };

    CliClient client_;
    ProfileStore store_;
    SessionStore session_;
    AppStatusLine *statusLine_ = nullptr;
    bool offerServerChoice_ = true;
    bool pendingServerDialog_ = false;
    bool pendingStatusDialog_ = false;
    bool pendingSessionRestore_ = false;
    bool serverDialogOpen_ = false;
    bool statusDialogOpen_ = false;
    std::vector<MinimizedWindow> minimized_;
};

} // namespace abraflexitui
