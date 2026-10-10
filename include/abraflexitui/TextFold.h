#pragma once

#include <cctype>
#include <string>

namespace abraflexitui {

// ASCII lowercase plus Czech diacritics folded to ASCII, so a query typed
// without háčky still matches names stored with them.
inline std::string foldText(const std::string &text) {
    std::string out;

    for (std::size_t i = 0; i < text.size();) {
        const auto c = static_cast<unsigned char>(text[i]);

        if (c < 0x80) {
            out.push_back(static_cast<char>(std::tolower(c)));
            ++i;
            continue;
        }

        if ((c & 0xE0) == 0xC0 && i + 1 < text.size()) {
            const unsigned code = (static_cast<unsigned>(c & 0x1F) << 6) |
                                  (static_cast<unsigned char>(text[i + 1]) & 0x3F);
            const char *folded = nullptr;

            switch (code) {
            case 0x00E1:
            case 0x00C1:
                folded = "a";
                break;
            case 0x010D:
            case 0x010C:
                folded = "c";
                break;
            case 0x010F:
            case 0x010E:
                folded = "d";
                break;
            case 0x00E9:
            case 0x00C9:
            case 0x011B:
            case 0x011A:
                folded = "e";
                break;
            case 0x00ED:
            case 0x00CD:
                folded = "i";
                break;
            case 0x0148:
            case 0x0147:
                folded = "n";
                break;
            case 0x00F3:
            case 0x00D3:
                folded = "o";
                break;
            case 0x0159:
            case 0x0158:
                folded = "r";
                break;
            case 0x0161:
            case 0x0160:
                folded = "s";
                break;
            case 0x0165:
            case 0x0164:
                folded = "t";
                break;
            case 0x00FA:
            case 0x00DA:
            case 0x016F:
            case 0x016E:
                folded = "u";
                break;
            case 0x00FD:
            case 0x00DD:
                folded = "y";
                break;
            case 0x017E:
            case 0x017D:
                folded = "z";
                break;
            default:
                break;
            }

            if (folded != nullptr) {
                out += folded;
            }

            i += 2;
            continue;
        }

        out.push_back(static_cast<char>(c));
        ++i;
    }

    return out;
}

inline bool foldedContains(const std::string &haystack, const std::string &needle) {
    if (needle.empty()) {
        return true;
    }

    return foldText(haystack).find(foldText(needle)) != std::string::npos;
}

inline void popUtf8(std::string &text) {
    while (!text.empty() && (static_cast<unsigned char>(text.back()) & 0xC0) == 0x80) {
        text.pop_back();
    }

    if (!text.empty()) {
        text.pop_back();
    }
}

} // namespace abraflexitui
