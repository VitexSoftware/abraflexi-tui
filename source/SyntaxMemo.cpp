#include "abraflexitui/TV.h"
#include "abraflexitui/SyntaxMemo.h"

#include <cctype>
#include <string>
#include <vector>

namespace abraflexitui {

namespace {

enum class Kind : unsigned char { Plain, Punct, String, Key, Number, Literal, Name, Comment };

TColorAttr tint(TColorAttr base, unsigned char biosForeground) {
    base.setForeground(TColor(TColorBIOS(biosForeground)));
    return base;
}

TColorAttr colorFor(Kind kind, TColorAttr normal) {
    switch (kind) {
    case Kind::Key:
    case Kind::Name:
        return tint(normal, 0x0E);
    case Kind::String:
        return tint(normal, 0x0A);
    case Kind::Number:
        return tint(normal, 0x0C);
    case Kind::Literal:
        return tint(normal, 0x0D);
    case Kind::Comment:
        return tint(normal, 0x06);
    case Kind::Punct:
        return tint(normal, 0x0F);
    case Kind::Plain:
        return normal;
    }

    return normal;
}

void paint(std::vector<Kind> &kinds, std::size_t begin, std::size_t end, Kind kind) {
    if (end > kinds.size()) {
        end = kinds.size();
    }

    for (std::size_t i = begin; i < end; ++i) {
        kinds[i] = kind;
    }
}

void scanJson(const std::string &text, std::vector<Kind> &kinds) {
    std::size_t i = 0;

    while (i < text.size()) {
        const unsigned char c = static_cast<unsigned char>(text[i]);

        if (std::isspace(c)) {
            ++i;
            continue;
        }

        if (text[i] == '"' || text[i] == '\'') {
            const char quote = text[i];
            const std::size_t start = i++;

            while (i < text.size()) {
                if (text[i] == '\\' && i + 1 < text.size()) {
                    i += 2;
                    continue;
                }

                if (text[i] == quote) {
                    ++i;
                    break;
                }

                ++i;
            }

            std::size_t look = i;

            while (look < text.size() && std::isspace(static_cast<unsigned char>(text[look]))) {
                ++look;
            }

            paint(kinds, start, i, look < text.size() && text[look] == ':' ? Kind::Key : Kind::String);
            continue;
        }

        if (text[i] == '{' || text[i] == '}' || text[i] == '[' || text[i] == ']' || text[i] == ':' || text[i] == ',') {
            kinds[i++] = Kind::Punct;
            continue;
        }

        if (text[i] == '-' || std::isdigit(c)) {
            const std::size_t start = i++;

            while (i < text.size() && (std::isdigit(static_cast<unsigned char>(text[i])) || text[i] == '.' ||
                                       text[i] == 'e' || text[i] == 'E' || text[i] == '+' || text[i] == '-')) {
                ++i;
            }

            paint(kinds, start, i, Kind::Number);
            continue;
        }

        if (std::isalpha(c)) {
            const std::size_t start = i++;

            while (i < text.size() && std::isalpha(static_cast<unsigned char>(text[i]))) {
                ++i;
            }

            paint(kinds, start, i, Kind::Literal);
            continue;
        }

        ++i;
    }
}

void scanXml(const std::string &text, std::vector<Kind> &kinds) {
    std::size_t i = 0;

    while (i < text.size()) {
        if (text.compare(i, 4, "<!--") == 0) {
            const std::size_t start = i;
            const std::size_t end = text.find("-->", i + 4);
            i = end == std::string::npos ? text.size() : end + 3;
            paint(kinds, start, i, Kind::Comment);
            continue;
        }

        if (text[i] != '<') {
            ++i;
            continue;
        }

        kinds[i++] = Kind::Punct;

        if (i < text.size() && (text[i] == '/' || text[i] == '?' || text[i] == '!')) {
            kinds[i++] = Kind::Punct;
        }

        if (i < text.size() && (std::isalpha(static_cast<unsigned char>(text[i])) || text[i] == '_' || text[i] == ':')) {
            const std::size_t start = i++;

            while (i < text.size() && text[i] != '>' && text[i] != '/' && text[i] != ' ' && text[i] != '\t' &&
                   text[i] != '\n' && text[i] != '\r') {
                ++i;
            }

            paint(kinds, start, i, Kind::Name);
        }

        while (i < text.size() && text[i] != '>') {
            if (text[i] == '"' || text[i] == '\'') {
                const char quote = text[i];
                const std::size_t start = i++;

                while (i < text.size() && text[i] != quote) {
                    ++i;
                }

                if (i < text.size()) {
                    ++i;
                }

                paint(kinds, start, i, Kind::String);
                continue;
            }

            if (std::isalpha(static_cast<unsigned char>(text[i])) || text[i] == '_' || text[i] == ':') {
                const std::size_t start = i++;

                while (i < text.size() && text[i] != '=' && text[i] != ' ' && text[i] != '\t' && text[i] != '>' &&
                       text[i] != '/') {
                    ++i;
                }

                paint(kinds, start, i, Kind::Key);
                continue;
            }

            if (text[i] == '=' || text[i] == '/' || text[i] == '?') {
                kinds[i++] = Kind::Punct;
                continue;
            }

            ++i;
        }

        if (i < text.size() && text[i] == '>') {
            kinds[i++] = Kind::Punct;
        }
    }
}

void formatHighlighted(TEditor &editor, TDrawBuffer &b, uint linePtr, int hScroll, int width, TAttrPair colors,
                       const std::vector<Kind> &kinds) {
    hScroll = hScroll < 0 ? 0 : hScroll;
    width = width < 0 ? 0 : width;

    const TColorAttr normal(colors);
    const TColorAttr selected(colors >> 8);
    uint P = linePtr;
    int pos = 0;
    int x = 0;

    while (P < editor.bufLen) {
        uint nextP = P;
        int nextPos = pos;

        if (!editor.nextCharAndPos(nextP, nextPos)) {
            break;
        }

        if (x > width || (x == width && pos < nextPos)) {
            break;
        }

        char buf[4];
        const uint charLen = nextP - P;
        editor.getText(P, TSpan<char>(buf, charLen > 4 ? 4 : charLen));

        if (buf[0] == '\r' || buf[0] == '\n') {
            break;
        }

        if (nextPos > hScroll) {
            const bool marked = editor.selStart <= P && P < editor.selEnd;
            const Kind kind = P < kinds.size() ? kinds[P] : Kind::Plain;
            const TColorAttr color = marked ? selected : colorFor(kind, normal);
            const int charWidth = nextPos - (pos > hScroll ? pos : hScroll);

            if (buf[0] == '\t' || pos < hScroll) {
                b.moveChar(static_cast<ushort>(x), ' ', color, static_cast<ushort>(charWidth));
            } else {
                b.moveStr(static_cast<ushort>(x), TStringView(buf, charLen), color);
            }

            x += charWidth;
        }

        P = nextP;
        pos = nextPos;
    }

    if (x < width) {
        b.moveChar(static_cast<ushort>(x), ' ', normal, static_cast<ushort>(width - x));
    }
}

} // namespace

SyntaxMemo::SyntaxMemo(const TRect &bounds, TScrollBar *hScrollBar, TScrollBar *vScrollBar, TIndicator *indicator,
                       ushort bufSize) noexcept
    : TMemo(bounds, hScrollBar, vScrollBar, indicator, bufSize) {
}

void SyntaxMemo::setMode(EditorMode mode) {
    if (mode_ == mode) {
        return;
    }

    mode_ = mode;

    if (owner != nullptr) {
        drawView();
    }
}

void SyntaxMemo::draw() {
    if (drawLine != delta.y) {
        drawPtr = lineMove(drawPtr, delta.y - drawLine);
        drawLine = delta.y;
    }

    std::string text(bufLen, '\0');

    if (bufLen > 0) {
        getText(0, TSpan<char>(text.data(), text.size()));
    }

    std::vector<Kind> kinds(text.size(), Kind::Plain);

    if (mode_ == EditorMode::Xml) {
        scanXml(text, kinds);
    } else {
        scanJson(text, kinds);
    }

    TDrawBuffer b;
    const TAttrPair colors = getColor(0x0201);
    uint linePtr = drawPtr;

    for (int y = 0; y < size.y; ++y) {
        formatHighlighted(*this, b, linePtr, delta.x, size.x, colors, kinds);
        writeBuf(0, static_cast<short>(y), size.x, 1, b);
        linePtr = nextLine(linePtr);
    }
}

} // namespace abraflexitui
