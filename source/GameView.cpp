#include "abraflexitui/TV.h"
#include "abraflexitui/GameView.h"
#include "abraflexitui/GameWindow.h"
#include "abraflexitui/i18n.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <string>

namespace abraflexitui {

namespace {

struct BrickStyle {
    const char *glyph; // UTF-8, one display cell wide
    uint32_t rgb;
};

// One color+texture combination per brick row, cycled if there are ever more
// rows than styles. TDrawBuffer::moveChar/putChar only take a single byte,
// so a multi-byte UTF-8 glyph has to go through moveStr() instead (see
// repeatedGlyph() below).
constexpr BrickStyle kBrickStyles[] = {
    {"█", 0xE05C5C}, // █ full block, red
    {"▓", 0xE0A83C}, // ▓ dark shade, orange
    {"▒", 0xE0D23C}, // ▒ medium shade, yellow
    {"◆", 0x4CAE4C}, // ◆ diamond, green
    {"▲", 0x4C9CE0}, // ▲ triangle, blue
};

constexpr const char *kBallGlyph = "●"; // ●

constexpr uint32_t kInk = 0x101010;
constexpr uint32_t kFg = 0xF0F0F0;
constexpr uint32_t kBg = 0x14181F;
constexpr uint32_t kPaddleBg = 0xC0C0C0;
constexpr uint32_t kBallFg = 0xFFFFFF;

std::string repeatedGlyph(const char *glyph, ushort count) {
    std::string out;

    for (ushort i = 0; i < count; ++i) {
        out += glyph;
    }

    return out;
}

} // namespace

GameView::GameView(const TRect &bounds, GameWindow *window) noexcept
    : TView(bounds),
      window_(window),
      paddleX_(0),
      paddleWidth_(8),
      paddleRow_(0),
      ballX_(0.0),
      ballY_(0.0),
      ballVx_(0.0),
      ballVy_(0.0),
      launched_(false),
      brickRows_(kBrickRows),
      brickCols_(1),
      brickCellWidth_(4),
      brickTop_(2),
      lives_(kStartLives),
      level_(1),
      speedFactor_(1.0),
      gameOver_(false),
      missPaused_(false),
      missPauseTicksLeft_(0),
      focusPaused_(false),
      tickTimer_(nullptr) {
    options |= ofSelectable;
    eventMask |= evBroadcast;
    growMode = gfGrowHiX | gfGrowHiY;

    paddleRow_ = static_cast<short>(size.y > 2 ? size.y - 2 : size.y);

    if (paddleWidth_ > size.x) {
        paddleWidth_ = size.x > 0 ? size.x : 1;
    }

    resetBricksForLevel();
    resetBall();
}

GameView::~GameView() {
    if (tickTimer_ != nullptr) {
        killTimer(tickTimer_);
    }
}

void GameView::start() {
    if (tickTimer_ == nullptr) {
        tickTimer_ = setTimer(kTickIntervalMs, kTickIntervalMs);
    }
}

void GameView::resetBricksForLevel() {
    brickCols_ = brickCellWidth_ > 0 ? static_cast<short>(size.x / brickCellWidth_) : 0;

    if (brickCols_ < 1) {
        brickCols_ = 1;
    }

    bricks_.assign(static_cast<std::size_t>(brickRows_) * static_cast<std::size_t>(brickCols_), Brick{false, 0});

    for (short row = 0; row < brickRows_; ++row) {
        for (short col = 0; col < brickCols_; ++col) {
            bricks_[static_cast<std::size_t>(row) * static_cast<std::size_t>(brickCols_) +
                     static_cast<std::size_t>(col)] = Brick{true, row};
        }
    }
}

void GameView::resetBall() {
    paddleX_ = static_cast<short>((size.x - paddleWidth_) / 2);

    if (paddleX_ < 0) {
        paddleX_ = 0;
    }

    ballX_ = paddleX_ + paddleWidth_ / 2.0;
    ballY_ = paddleRow_ - 1.0;
    ballVx_ = 0.0;
    ballVy_ = 0.0;
    launched_ = false;
}

void GameView::launchBall() {
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<double> dist(-1.0, 1.0);
    const double offset = dist(rng);

    ballVx_ = offset * kMaxHSpeed * speedFactor_ * 0.6;
    ballVy_ = -std::fabs(kBaseSpeed * speedFactor_);
    launched_ = true;
}

bool GameView::allBricksCleared() const {
    for (const Brick &brick : bricks_) {
        if (brick.alive) {
            return false;
        }
    }

    return true;
}

void GameView::step() {
    if (gameOver_ || focusPaused_) {
        return;
    }

    if (missPaused_) {
        if (--missPauseTicksLeft_ <= 0) {
            missPaused_ = false;
        }

        return;
    }

    if (!launched_) {
        // Keep the ball riding the paddle until the player launches it.
        ballX_ = paddleX_ + paddleWidth_ / 2.0;
        ballY_ = paddleRow_ - 1.0;
        return;
    }

    ballX_ += ballVx_;
    ballY_ += ballVy_;

    if (ballX_ <= 0.0) {
        ballX_ = 0.0;
        ballVx_ = -ballVx_;
    } else if (ballX_ >= size.x - 1) {
        ballX_ = size.x - 1;
        ballVx_ = -ballVx_;
    }

    if (ballY_ <= brickTop_ - 1.0) {
        ballY_ = brickTop_ - 1.0;
        ballVy_ = -ballVy_;
    }

    const short row = static_cast<short>(std::lround(ballY_));
    const short col = static_cast<short>(std::lround(ballX_));

    if (row >= brickTop_ && row < brickTop_ + brickRows_ && brickCellWidth_ > 0) {
        const short brickCol = static_cast<short>(col / brickCellWidth_);

        if (brickCol >= 0 && brickCol < brickCols_) {
            const std::size_t idx = static_cast<std::size_t>(row - brickTop_) * static_cast<std::size_t>(brickCols_) +
                                     static_cast<std::size_t>(brickCol);

            if (idx < bricks_.size() && bricks_[idx].alive) {
                bricks_[idx].alive = false;
                ballVy_ = -ballVy_;

                if (allBricksCleared()) {
                    nextLevel();
                    return;
                }
            }
        }
    }

    if (row >= paddleRow_) {
        if (ballVy_ > 0 && col >= paddleX_ && col < paddleX_ + paddleWidth_) {
            const double center = paddleX_ + paddleWidth_ / 2.0;
            const double offset = paddleWidth_ > 0 ? (ballX_ - center) / (paddleWidth_ / 2.0) : 0.0;
            ballVx_ = offset * kMaxHSpeed * speedFactor_;
            ballVy_ = -std::fabs(kBaseSpeed * speedFactor_);
            ballY_ = paddleRow_ - 1.0;
        } else if (row >= size.y - 1) {
            loseLife();
        }
    }
}

void GameView::loseLife() {
    --lives_;

    if (lives_ <= 0) {
        gameOver();
        return;
    }

    missPaused_ = true;
    missPauseTicksLeft_ = kMissPauseTicks;
    resetBall();
}

void GameView::nextLevel() {
    ++level_;
    ++lives_;
    speedFactor_ += kSpeedIncrement;
    resetBricksForLevel();
    resetBall();
}

void GameView::gameOver() {
    gameOver_ = true;

    if (tickTimer_ != nullptr) {
        killTimer(tickTimer_);
        tickTimer_ = nullptr;
    }
}

void GameView::movePaddle(short dx) {
    short next = static_cast<short>(paddleX_ + dx);
    const short maxX = static_cast<short>(size.x - paddleWidth_);

    if (next < 0) {
        next = 0;
    }

    if (next > maxX) {
        next = maxX;
    }

    if (next != paddleX_) {
        paddleX_ = next;
        drawView();
    }
}

void GameView::handleEvent(TEvent &event) {
    TView::handleEvent(event);

    if (event.what == evKeyDown) {
        switch (event.keyDown.keyCode) {
        case kbLeft:
            movePaddle(-2);
            clearEvent(event);
            return;

        case kbRight:
            movePaddle(2);
            clearEvent(event);
            return;

        default:
            break;
        }

        // Space bar (there's no dedicated kbSpace constant - matched the same
        // way TButton matches it in tbutton.cpp) launches the ball while it's
        // still waiting on the paddle.
        if (event.keyDown.charScan.charCode == ' ' && !launched_ && !gameOver_) {
            launchBall();
            drawView();
            clearEvent(event);
            return;
        }
    } else if (event.what == evBroadcast && event.message.command == cmTimerExpired && tickTimer_ != nullptr &&
               event.message.infoPtr == tickTimer_) {
        step();
        drawView();
        clearEvent(event);
    }
}

void GameView::setState(ushort aState, Boolean enable) {
    TView::setState(aState, enable);

    if (aState == sfFocused) {
        // An unfocused view never receives key events, so the paddle already
        // stops responding to arrows on its own; this flag additionally
        // freezes the ball/physics and reflects the pause in the window
        // title (see GameWindow::updateTitle).
        focusPaused_ = (enable == False);

        if (window_ != nullptr) {
            window_->updateTitle(focusPaused_);
        }
    }
}

void GameView::draw() {
    TDrawBuffer b;
    const TColorAttr bgColor = TColorAttr(TColor(TColorRGB(kFg)), TColor(TColorRGB(kBg)));

    const short ballRow = static_cast<short>(std::lround(ballY_));
    const short ballCol = static_cast<short>(std::lround(ballX_));

    for (short y = 0; y < size.y; ++y) {
        b.moveChar(0, ' ', bgColor, static_cast<ushort>(size.x));

        if (y == 0) {
            char text[128];

            if (gameOver_) {
                std::snprintf(text, sizeof(text), _("Game Over - Level %d - close the window (Alt-F3)"), level_);
            } else if (!launched_) {
                std::snprintf(text, sizeof(text), _("Lives: %d   Level: %d   Space launches the ball%s"), lives_,
                               level_, focusPaused_ ? _("   [Paused]") : "");
            } else {
                std::snprintf(text, sizeof(text), _("Lives: %d   Level: %d%s"), lives_, level_,
                               focusPaused_ ? _("   [Paused]") : "");
            }

            b.moveStr(0, text, bgColor, static_cast<ushort>(size.x));
        } else if (y >= brickTop_ && y < brickTop_ + brickRows_) {
            const short row = static_cast<short>(y - brickTop_);
            const BrickStyle &style = kBrickStyles[row % static_cast<short>(sizeof(kBrickStyles) / sizeof(kBrickStyles[0]))];
            const TColorAttr brickColor = TColorAttr(TColor(TColorRGB(kInk)), TColor(TColorRGB(style.rgb)));

            for (short col = 0; col < brickCols_; ++col) {
                const std::size_t idx = static_cast<std::size_t>(row) * static_cast<std::size_t>(brickCols_) +
                                         static_cast<std::size_t>(col);

                if (idx >= bricks_.size() || !bricks_[idx].alive) {
                    continue;
                }

                const short startX = static_cast<short>(col * brickCellWidth_);

                if (startX >= size.x) {
                    continue;
                }

                const short rawWidth = static_cast<short>(brickCellWidth_ > 1 ? brickCellWidth_ - 1 : 1);
                const ushort width = static_cast<ushort>(std::min<short>(rawWidth, static_cast<short>(size.x - startX)));
                const std::string glyphs = repeatedGlyph(style.glyph, width);
                b.moveStr(static_cast<ushort>(startX), glyphs, brickColor, width);
            }
        }

        if (y == paddleRow_ && paddleWidth_ > 0) {
            const TColorAttr paddleColor = TColorAttr(TColor(TColorRGB(kInk)), TColor(TColorRGB(kPaddleBg)));
            const ushort width = static_cast<ushort>(std::min<short>(paddleWidth_, static_cast<short>(size.x - paddleX_)));
            b.moveChar(static_cast<ushort>(paddleX_), ' ', paddleColor, width);
        }

        if (y == ballRow && ballCol >= 0 && ballCol < size.x) {
            const TColorAttr ballColor = TColorAttr(TColor(TColorRGB(kBallFg)), TColor(TColorRGB(kBg)));
            b.moveStr(static_cast<ushort>(ballCol), kBallGlyph, ballColor, 1);
        }

        writeLine(0, y, size.x, 1, b);
    }
}

} // namespace abraflexitui
