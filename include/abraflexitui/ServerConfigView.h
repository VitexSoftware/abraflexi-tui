#pragma once

#include "abraflexitui/TV.h"
#include "abraflexitui/CliClient.h"
#include "abraflexitui/ProfileStore.h"
#include "abraflexitui/SimpleListViewer.h"

#include <functional>

namespace abraflexitui {

// Modal list of saved server profiles. Add/Edit open ServerProfileForm.
// onChanged runs after every successful save so the app can retarget CliClient.
class ServerConfigView : public TDialog {
public:
    ServerConfigView(ProfileStore &store, CliClient &client, std::function<void()> onChanged);

    void handleEvent(TEvent &event) override;
    TColorAttr mapColor(uchar index) override;

private:
    void refreshList();
    void addProfile();
    void editProfile();
    void deleteProfile();
    void activateSelected();
    bool persist();
    const ServerProfile *selected() const;

    ProfileStore &store_;
    CliClient &client_;
    std::function<void()> onChanged_;
    SimpleListViewer *list_ = nullptr;
};

} // namespace abraflexitui
