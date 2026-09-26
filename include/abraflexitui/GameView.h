#pragma once

#include "abraflexitui/TV.h"

#include <vector>

namespace abraflexitui {

class GameWindow;

// Arkanoid/Breakout-style mini-game shown inside GameWindow, reached from the
// "Nápověda" (Help) menu. Owns all game state (paddle, ball, bricks, lives,
// level) and drives its own animation via a tvision timer broadcast (see
// step()/handleEvent()) instead of hooking into the application's idle().
class GameView : public TView {
public:
    GameView(const TRect &bounds, GameWindow *window) noexcept;
    ~GameView() override;

    // Starts the animation timer. Must be called only after this view has
    // been inserted into its owner (e.g. right after GameWindow's insert()):
    // TView::setTimer() forwards to `owner->setTimer(...)`, which is a no-op
    // while owner is still null, so calling this from the constructor itself
    // - before the view has an owner - would silently create no timer at
    // all, leaving the game permanently frozen.
    void start();

    void draw() override;
    void handleEvent(TEvent &event) override;
    void setState(ushort aState, Boolean enable) override;

private:
    struct Brick {
        bool alive;
        short row;
    };

    void resetBricksForLevel();
    void resetBall();
    void step();
    void movePaddle(short dx);
    void launchBall();
    void loseLife();
    void nextLevel();
    void gameOver();
    bool allBricksCleared() const;

    GameWindow *window_;

    short paddleX_;
    short paddleWidth_;
    short paddleRow_;

    // Ball position/velocity use sub-cell precision (a text grid is far
    // coarser than a real pixel screen), rounded to the nearest cell only
    // when drawing or testing collisions.
    double ballX_;
    double ballY_;
    double ballVx_;
    double ballVy_;
    // While false, the ball rides the paddle (no physics) and waits for the
    // player to press Left/Right, which both moves the paddle and launches
    // the ball upward at a random angle - see launchBall().
    bool launched_;

    std::vector<Brick> bricks_;
    short brickRows_;
    short brickCols_;
    short brickCellWidth_;
    short brickTop_;

    int lives_;
    int level_;
    double speedFactor_;

    bool gameOver_;
    // Brief freeze after losing a ball, so the next one doesn't relaunch
    // instantly.
    bool missPaused_;
    int missPauseTicksLeft_;
    // Freezes the whole game (ball, paddle input has no effect since an
    // unfocused view never receives key events) while the window is not
    // focused; see setState().
    bool focusPaused_;

    TTimerId tickTimer_;

    static constexpr int kTickIntervalMs = 60;
    static constexpr int kStartLives = 5;
    static constexpr int kBrickRows = 5;
    static constexpr double kBaseSpeed = 0.6;
    static constexpr double kSpeedIncrement = 0.12;
    static constexpr double kMaxHSpeed = 0.9;
    static constexpr int kMissPauseTicks = 15;
};

} // namespace abraflexitui
