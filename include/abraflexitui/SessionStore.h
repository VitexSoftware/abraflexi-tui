#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace abraflexitui {

// Persisted position/size of one window, in the same units as TRect
// (character cells).
struct WindowBounds {
    int x1 = 0;
    int y1 = 0;
    int x2 = 0;
    int y2 = 0;
};

// One remembered "record list" window: which evidence it was browsing,
// which record was selected in it, and where it was on screen.
struct WindowSession {
    int id = 0;
    std::string evidence;
    std::string company;
    std::string focusedId;
    WindowBounds bounds;
    bool hasBounds = false;
};

// Remembers which evidence/record-list windows were open and which record
// was selected in each, across restarts of abraflexi-tui - so re-opening
// the app returns to the same documents instead of an empty desktop.
// Mirrors ProfileStore's on-disk JSON + load/save pattern, but under
// $XDG_STATE_HOME (runtime/session state, not user configuration - same
// directory CliClient already uses for its failure log).
class SessionStore {
public:
    explicit SessionStore(std::string path = {});

    static std::string defaultStatePath();

    void load();
    bool save() const;

    const std::vector<WindowSession> &windows() const { return windows_; }

    // Registers a newly opened window and returns an opaque handle
    // (WindowSession::id) to use with the update*/close calls below.
    int openWindow(const std::string &evidence, const std::string &company, const std::string &focusedId, const WindowBounds *bounds);
    void updateFocused(int handle, const std::string &focusedId);
    void updateBounds(int handle, const WindowBounds &bounds);
    void closeWindow(int handle);

    // Last text typed into a named search/typeahead box (e.g. "evidenceFind"
    // for EvidenceListView's "Find:" box), restored as a starting value the
    // next time that box is shown.
    std::string searchText(const std::string &key) const;
    void setSearchText(const std::string &key, const std::string &value);

private:
    std::string path_;
    std::vector<WindowSession> windows_;
    std::map<std::string, std::string> searchTexts_;
    int nextId_ = 1;
};

} // namespace abraflexitui
