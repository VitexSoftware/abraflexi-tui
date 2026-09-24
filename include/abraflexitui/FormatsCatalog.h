#pragma once

#include <string>
#include <utility>
#include <vector>

namespace abraflexitui {

// Reads AbraFlexi's own export-format catalog
// (/usr/share/php/AbraFlexi/static/Formats.json: evidence name -> {label:
// extension}), so the Download dialog offers exactly the formats AbraFlexi's
// REST API actually supports for a given evidence. Falls back to the
// universal {CSV, HTML, JSON, XML} set (present for every evidence in the
// catalog) when the file is missing or doesn't list `evidence`.
class FormatsCatalog {
public:
    // {label, extension} pairs in the catalog's own order.
    static std::vector<std::pair<std::string, std::string>> forEvidence(const std::string &evidence);
};

} // namespace abraflexitui
