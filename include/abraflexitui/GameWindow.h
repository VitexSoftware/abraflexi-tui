#pragma once

#include "abraflexitui/TV.h"

#include <string>

namespace abraflexitui {

class GameView;

// Non-modal window hosting the Arkanoid-style GameView, opened from the
// "Nápověda" (Help) menu (cmShowGame). Non-modal so it keeps receiving its
// timer broadcast and keyboard input while other windows are open.
class GameWindow : public TWindow {
public:
    GameWindow();

    const char *getTitle(short maxSize) override;

    // Called by GameView when it gains/loses focus, to reflect the paused
    // state directly in the window's title bar.
    void updateTitle(bool paused);

    // Starts the game's animation timer. Must be called only after this
    // window itself has an owner (i.e. after deskTop->insert(win)) - see the
    // comment in the constructor.
    void startGame();

private:
    GameView *game_;
    std::string baseTitle_;
    std::string titleText_;
};

} // namespace abraflexitui
