#include "abraflexitui/TV.h"
#include "abraflexitui/GameWindow.h"
#include "abraflexitui/GameView.h"

namespace abraflexitui {

GameWindow::GameWindow()
    : TWindowInit(&TWindow::initFrame),
      TWindow(TRect(2, 1, 64, 26), "Hra - Arkanoid", wnNoNumber),
      baseTitle_("Hra - Arkanoid"),
      titleText_(baseTitle_) {
    options |= ofCentered;
    // Fixed size: no grow handle, no zoom - the brick grid/paddle geometry
    // is laid out once for this size and isn't meant to reflow on resize.
    // wfMove stays set so the window can still be dragged around the desk.
    flags &= static_cast<uchar>(~(wfGrow | wfZoom));

    TRect inner = getExtent();
    inner.grow(-1, -1);
    game_ = new GameView(inner, this);
    insert(game_);
    selectNext(False);
    // NOT game_->start() here: TView::setTimer() bubbles up via
    // `owner->setTimer(...)` at every level, and this window itself has no
    // owner yet (it isn't inserted into deskTop until after this
    // constructor returns), so the timer would silently fail to register.
    // Call startGame() once this window has actually been inserted - see
    // AppShell.cpp's cmShowGame handler.
}

void GameWindow::startGame() {
    if (game_ != nullptr) {
        game_->start();
    }
}

const char *GameWindow::getTitle(short) {
    return titleText_.c_str();
}

void GameWindow::updateTitle(bool paused) {
    titleText_ = paused ? baseTitle_ + " [Paused]" : baseTitle_;

    if (frame != nullptr) {
        frame->drawView();
    }
}

} // namespace abraflexitui
