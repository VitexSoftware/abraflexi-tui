#include "abraflexitui/FormatsCatalog.h"

#include <fstream>

#include <nlohmann/json.hpp>

namespace abraflexitui {

namespace {

constexpr const char *kFormatsJsonPath = "/usr/share/php/AbraFlexi/static/Formats.json";

const nlohmann::json &loadCatalog() {
    static nlohmann::json catalog = [] {
        nlohmann::json data;
        std::ifstream in(kFormatsJsonPath);

        if (in) {
            try {
                in >> data;
            } catch (const nlohmann::json::exception &) {
                data = nlohmann::json();
            }
        }

        return data;
    }();

    return catalog;
}

std::vector<std::pair<std::string, std::string>> defaultFormats() {
    return {{"CSV", "csv"}, {"HTML", "html"}, {"JSON", "json"}, {"XML", "xml"}};
}

// Formats.json only lists AbraFlexi's plain data-export formats; the PDF
// report rendering (evidence/id.pdf, the same endpoint PrintDialog's
// document-PDF option already downloads) is a separate facility that works
// for every evidence but isn't listed there. Always offer it explicitly so
// e.g. faktura-vydana can be downloaded as PDF even though it's absent from
// the catalog file.
void ensurePdf(std::vector<std::pair<std::string, std::string>> &formats) {
    for (const auto &fmt : formats) {
        if (fmt.second == "pdf") {
            return;
        }
    }
    formats.emplace_back("PDF", "pdf");
}

} // namespace

std::vector<std::pair<std::string, std::string>> FormatsCatalog::forEvidence(const std::string &evidence) {
    const nlohmann::json &catalog = loadCatalog();

    if (!catalog.is_object() || !catalog.contains(evidence) || !catalog.at(evidence).is_object()) {
        std::vector<std::pair<std::string, std::string>> formats = defaultFormats();
        ensurePdf(formats);
        return formats;
    }

    std::vector<std::pair<std::string, std::string>> formats;

    for (const auto &entry : catalog.at(evidence).items()) {
        if (entry.value().is_string()) {
            formats.emplace_back(entry.key(), entry.value().get<std::string>());
        }
    }

    if (formats.empty()) {
        formats = defaultFormats();
    }

    ensurePdf(formats);
    return formats;
}

} // namespace abraflexitui
