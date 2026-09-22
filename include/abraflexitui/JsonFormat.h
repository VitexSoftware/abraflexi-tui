#pragma once

#include <string>

#include <nlohmann/json.hpp>

namespace abraflexitui {

// Renders a JSON value as a display string: strings pass through as-is,
// null becomes empty, everything else (numbers, nested objects/arrays,
// booleans) is dumped as compact JSON.
inline std::string jsonDisplay(const nlohmann::json &value) {
    if (value.is_null()) {
        return std::string();
    }

    if (value.is_string()) {
        return value.get<std::string>();
    }

    return value.dump();
}

inline std::string jsonField(const nlohmann::json &obj, const char *key) {
    if (!obj.is_object() || !obj.contains(key)) {
        return std::string();
    }

    return jsonDisplay(obj.at(key));
}

// Left-pads a column value with spaces/truncates to a fixed width, for
// simple fixed-width table rendering in a SimpleListViewer row.
inline std::string fitColumn(const std::string &value, std::size_t width) {
    std::string v = value;

    for (auto &ch : v) {
        if (ch == '\n' || ch == '\r' || ch == '\t') {
            ch = ' ';
        }
    }

    if (v.size() > width) {
        if (width > 1) {
            v = v.substr(0, width - 1) + "~";
        } else {
            v = v.substr(0, width);
        }
    } else {
        v.append(width - v.size(), ' ');
    }

    return v;
}

} // namespace abraflexitui
