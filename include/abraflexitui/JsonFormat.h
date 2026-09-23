#pragma once

#include "abraflexitui/EvidenceSchema.h"

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

// Extracts the human-readable label from an AbraFlexi "{ref, showAs}"-shaped
// object (or a nested currency object of the same shape). Falls back to
// "ref" when "showAs" is null/absent, and to empty when both are null -
// matching AbraFlexi's own convention for unset relation fields.
inline std::string jsonShowAs(const nlohmann::json &value) {
    if (!value.is_object()) {
        return std::string();
    }

    if (value.contains("showAs") && value.at("showAs").is_string()) {
        return value.at("showAs").get<std::string>();
    }

    if (value.contains("ref") && value.at("ref").is_string()) {
        return value.at("ref").get<std::string>();
    }

    return std::string();
}

// Strips a trailing UTC/zone offset ("+01:00", "Z", ...) from an AbraFlexi
// ISO date/datetime string, leaving just the date/time portion for display.
inline std::string stripDateOffset(const std::string &date) {
    auto plus = date.find_last_of('+');

    if (plus != std::string::npos) {
        return date.substr(0, plus);
    }

    if (!date.empty() && date.back() == 'Z') {
        return date.substr(0, date.size() - 1);
    }

    return date;
}

// Drops the "00:00:00.000000"-style zero time-of-day AbraFlexi still
// includes on pure date fields, leaving just the date portion for display.
inline std::string stripTimeOfDay(const std::string &date) {
    auto spacePos = date.find(' ');

    if (spacePos != std::string::npos) {
        return date.substr(0, spacePos);
    }

    return date;
}

// Shape-aware rendering of an AbraFlexi field value: recognizes the nested
// object shapes AbraFlexi uses for dates ({"date":..,"timezone":..} or
// {"date":..,"showAsDate":..}), relations/references
// ({"ref":..,"showAs":..}), and money
// ({"value":..,"currency":{"ref":..,"showAs":..}}), rendering each as a
// single human-readable line. "field" is an optional schema hint: for a
// "date" (not "datetime") column, it is used to strip the zero
// time-of-day via stripTimeOfDay(). Any object shape not recognized above
// falls back to jsonDisplay()'s raw dump.
inline std::string jsonDisplay(const nlohmann::json &value, const FieldSchema *field) {
    const bool isDateField = field != nullptr && field->type == "date";

    if (!value.is_object()) {
        std::string display = jsonDisplay(value);

        if (isDateField && value.is_string()) {
            display = stripTimeOfDay(display);
        }

        return display;
    }

    if (value.contains("showAs")) {
        return jsonShowAs(value);
    }

    if (value.contains("date")) {
        const auto &dateVal = value.at("date");

        if (value.contains("showAsDate") && value.at("showAsDate").is_string()) {
            return value.at("showAsDate").get<std::string>();
        }

        if (dateVal.is_string()) {
            std::string display = stripDateOffset(dateVal.get<std::string>());

            if (isDateField) {
                display = stripTimeOfDay(display);
            }

            return display;
        }

        return jsonDisplay(dateVal);
    }

    if (value.contains("value") && value.contains("currency")) {
        std::string amount = jsonDisplay(value.at("value"));
        std::string currency = jsonShowAs(value.at("currency"));

        return currency.empty() ? amount : amount + " " + currency;
    }

    return jsonDisplay(value);
}

inline std::string jsonField(const nlohmann::json &obj, const char *key) {
    if (!obj.is_object() || !obj.contains(key)) {
        return std::string();
    }

    return jsonDisplay(obj.at(key));
}

inline std::string jsonField(const nlohmann::json &obj, const char *key, const FieldSchema *field) {
    if (!obj.is_object() || !obj.contains(key)) {
        return std::string();
    }

    return jsonDisplay(obj.at(key), field);
}

// Resolves the identifier to use for AbraFlexi record operations (show/
// update/delete) and for remembering the focused row across refreshes.
// Every evidence carries a numeric "id", but a caller may have requested a
// column list that doesn't include it (many of this app's default column
// presets show "kod" instead) - "id" is still fetched behind the scenes
// (see RecordListView::refresh()) so it is normally present here regardless
// of what's displayed. If it's ever missing, fall back to AbraFlexi's own
// "code:KOD" record-identifier convention (see AbraFlexi\Code::code()),
// which evidences that key records by "kod" accept anywhere a numeric id
// is expected.
inline std::string recordIdentifier(const nlohmann::json &record) {
    std::string id = jsonField(record, "id");

    if (!id.empty()) {
        return id;
    }

    std::string kod = jsonField(record, "kod");

    if (!kod.empty()) {
        return "code:" + kod;
    }

    return std::string();
}

// Number of Unicode code points (== terminal display columns, since the
// diacritics used here are all narrow characters) in a UTF-8 string.
inline std::size_t utf8DisplayWidth(const std::string &value) {
    std::size_t width = 0;

    for (unsigned char ch : value) {
        if ((ch & 0xC0) != 0x80) {
            ++width;
        }
    }

    return width;
}

// Byte offset in `value` at which exactly `width` display columns have been
// consumed, always landing on a UTF-8 code point boundary.
inline std::size_t utf8ByteOffsetForWidth(const std::string &value, std::size_t width) {
    if (width == 0) {
        return 0;
    }

    std::size_t seen = 0;

    for (std::size_t i = 0; i < value.size(); ++i) {
        if ((static_cast<unsigned char>(value[i]) & 0xC0) != 0x80) {
            if (seen == width) {
                return i;
            }

            ++seen;
        }
    }

    return value.size();
}

inline std::string utf8TruncateToWidth(const std::string &value, std::size_t width) {
    return value.substr(0, utf8ByteOffsetForWidth(value, width));
}

// Left-pads a column value with spaces/truncates to a fixed width, for
// simple fixed-width table rendering in a SimpleListViewer row. Width is
// measured in display columns (UTF-8 code points), not bytes, so diacritics
// don't throw off column alignment.
inline std::string fitColumn(const std::string &value, std::size_t width) {
    std::string v = value;

    for (auto &ch : v) {
        if (ch == '\n' || ch == '\r' || ch == '\t') {
            ch = ' ';
        }
    }

    const std::size_t displayWidth = utf8DisplayWidth(v);

    if (displayWidth > width) {
        if (width > 1) {
            v = utf8TruncateToWidth(v, width - 1) + "~";
        } else {
            v = utf8TruncateToWidth(v, width);
        }
    } else if (displayWidth < width) {
        v.append(width - displayWidth, ' ');
    }

    return v;
}

} // namespace abraflexitui
