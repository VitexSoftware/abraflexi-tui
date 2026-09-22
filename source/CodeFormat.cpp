#include "abraflexitui/CodeFormat.h"

#include <cctype>
#include <nlohmann/json.hpp>

namespace abraflexitui {

namespace {

std::string editorText(TEditor &editor) {
    if (editor.bufLen == 0) {
        return std::string();
    }

    std::string text(editor.bufLen, '\0');
    const uint n = editor.getText(0, TSpan<char>(text.data(), text.size()));
    text.resize(n);
    return text;
}

std::string trimCopy(const std::string &text) {
    std::size_t begin = 0;

    while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin]))) {
        ++begin;
    }

    std::size_t end = text.size();

    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }

    return text.substr(begin, end - begin);
}

std::string formatJson(const std::string &text, std::string &error) {
    try {
        const nlohmann::json parsed = nlohmann::json::parse(text);
        return parsed.dump(2);
    } catch (const nlohmann::json::parse_error &e) {
        error = std::string("Invalid JSON: ") + e.what();
        return std::string();
    }
}

std::string formatXml(const std::string &text, std::string &error) {
    if (text.find('<') == std::string::npos) {
        error = "Invalid XML: no element found";
        return std::string();
    }

    std::string out;
    int pad = 0;

    for (std::size_t i = 0; i < text.size();) {
        if (text[i] != '<') {
            const std::size_t next = text.find('<', i);
            const std::string raw = text.substr(i, next == std::string::npos ? std::string::npos : next - i);
            const std::string value = trimCopy(raw);

            if (!value.empty()) {
                if (!out.empty() && out.back() != '\n') {
                    out.push_back('\n');
                }

                out.append(static_cast<std::size_t>(pad) * 2, ' ');
                out += value;
            }

            if (next == std::string::npos) {
                break;
            }

            i = next;
            continue;
        }

        const std::size_t close = text.find('>', i);

        if (close == std::string::npos) {
            error = "Invalid XML: unclosed tag";
            return std::string();
        }

        const std::string tag = text.substr(i, close - i + 1);
        const bool closing = tag.size() > 1 && tag[1] == '/';
        const bool special = tag.size() > 1 && (tag[1] == '?' || tag[1] == '!');
        const bool selfClosing = tag.size() > 1 && tag[tag.size() - 2] == '/';

        if (closing && pad > 0) {
            --pad;
        }

        if (!out.empty() && out.back() != '\n') {
            out.push_back('\n');
        }

        out.append(static_cast<std::size_t>(pad) * 2, ' ');
        out += tag;

        if (!closing && !special && !selfClosing) {
            ++pad;
        }

        i = close + 1;
    }

    if (!out.empty() && out.back() != '\n') {
        out.push_back('\n');
    }

    return out;
}

bool replaceText(TEditor &editor, const std::string &text, std::string &error) {
    const uint needed = static_cast<uint>(text.size()) + 1;

    if (needed > editor.bufSize && !editor.setBufSize(needed)) {
        error = "Formatted text does not fit in the editor";
        return false;
    }

    editor.setSelect(0, editor.bufLen, True);
    editor.deleteSelect();

    if (!text.empty() && !editor.insertText(text.data(), static_cast<uint>(text.size()), False)) {
        error = "Formatted text does not fit in the editor";
        return false;
    }

    return true;
}

} // namespace

bool formatEditorText(TEditor &editor, bool xml, std::string &error) {
    error.clear();
    const std::string text = trimCopy(editorText(editor));

    if (text.empty()) {
        return true;
    }

    const std::string formatted = xml ? formatXml(text, error) : formatJson(text, error);

    if (!error.empty()) {
        return false;
    }

    return replaceText(editor, formatted, error);
}

} // namespace abraflexitui
