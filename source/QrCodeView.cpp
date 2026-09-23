#include "abraflexitui/TV.h"
#include "abraflexitui/QrCodeView.h"
#include "abraflexitui/WindowColors.h"

#include "qrcodegen.hpp"

#include <string>
#include <vector>

namespace abraflexitui {

namespace {

class QrCodeView : public TView {
public:
    QrCodeView(const TRect &bounds, std::vector<std::string> rows) noexcept
        : TView(bounds), rows_(std::move(rows)) {
    }

    void draw() override {
        const TColorAttr ink = 0xF0;

        for (short y = 0; y < size.y; ++y) {
            TDrawBuffer buffer;
            buffer.moveChar(0, ' ', ink, static_cast<ushort>(size.x));

            if (y >= 0 && static_cast<std::size_t>(y) < rows_.size()) {
                buffer.moveStr(0, rows_[static_cast<std::size_t>(y)], ink, static_cast<ushort>(size.x));
            }

            writeLine(0, y, static_cast<ushort>(size.x), 1, buffer);
        }
    }

private:
    std::vector<std::string> rows_;
};

bool module(const qrcodegen::QrCode &code, int quiet, int x, int y, int side) {
    if (x < quiet || y < quiet || x >= side - quiet || y >= side - quiet) {
        return false;
    }

    return code.getModule(x - quiet, y - quiet);
}

std::string halfBlock(bool top, bool bottom) {
    if (top && bottom) {
        return "\u2588";
    }

    if (top) {
        return "\u2580";
    }

    if (bottom) {
        return "\u2584";
    }

    return " ";
}

std::string fitCaption(const std::string &url, int width) {
    if (width < 1) {
        return std::string();
    }

    if (static_cast<int>(url.size()) <= width) {
        return url;
    }

    if (width <= 3) {
        return url.substr(url.size() - static_cast<std::size_t>(width));
    }

    return "..." + url.substr(url.size() - static_cast<std::size_t>(width - 3));
}

int quietZone(int modules) {
    TRect desk = TProgram::deskTop->getExtent();
    const int maxSide = desk.b.x - desk.a.x - 2;
    const int maxRows = desk.b.y - desk.a.y - 3;

    for (int quiet : {4, 2, 1, 0}) {
        const int side = modules + quiet * 2;
        const int rows = (side + 1) / 2;

        if (side <= maxSide && rows <= maxRows) {
            return quiet;
        }
    }

    return 0;
}

} // namespace

QrCodeDialog::QrCodeDialog(const std::string &url)
    : TWindowInit(&TDialog::initFrame), TDialog(TRect(0, 0, 10, 10), "Web") {
    const qrcodegen::QrCode code = qrcodegen::QrCode::encodeText(url.c_str(), qrcodegen::QrCode::Ecc::LOW);
    const int modules = code.getSize();
    const int quiet = quietZone(modules);
    const int side = modules + quiet * 2;
    const short rows = static_cast<short>((side + 1) / 2);
    const short width = static_cast<short>(side + 2);
    const short height = static_cast<short>(rows + 3);
    TRect bounds(0, 0, width, height);
    locate(bounds);
    options |= ofCentered;

    std::vector<std::string> lines;
    lines.reserve(static_cast<std::size_t>(rows));

    for (int row = 0; row < rows; ++row) {
        std::string line;
        line.reserve(static_cast<std::size_t>(side * 3));
        const int y = row * 2;

        for (int x = 0; x < side; ++x) {
            line += halfBlock(module(code, quiet, x, y, side), module(code, quiet, x, y + 1, side));
        }

        lines.push_back(std::move(line));
    }

    insert(new QrCodeView(TRect(1, 1, static_cast<short>(1 + side), static_cast<short>(1 + rows)), std::move(lines)));
    insert(new TStaticText(TRect(1, static_cast<short>(1 + rows), static_cast<short>(width - 1), static_cast<short>(2 + rows)),
                           fitCaption(url, side).c_str()));
}

TColorAttr QrCodeDialog::mapColor(uchar index) {
    TColorAttr color;
    return windowColor(index, color) ? color : TDialog::mapColor(index);
}

} // namespace abraflexitui
