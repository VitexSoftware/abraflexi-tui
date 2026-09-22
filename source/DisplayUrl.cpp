#include "abraflexitui/DisplayUrl.h"

#include <cctype>

namespace abraflexitui {

namespace {

std::string urlEncode(const std::string &value) {
    static const char *hex = "0123456789ABCDEF";
    std::string out;
    out.reserve(value.size());

    for (unsigned char c : value) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out.push_back(static_cast<char>(c));
        } else {
            out.push_back('%');
            out.push_back(hex[c >> 4]);
            out.push_back(hex[c & 0x0F]);
        }
    }

    return out;
}

std::string trimTrailingSlashes(std::string url) {
    while (!url.empty() && url.back() == '/') {
        url.pop_back();
    }

    return url;
}

std::string optionValue(const std::vector<std::string> &args, const char *longName, const char *shortName) {
    const std::string flag = std::string("--") + longName;
    const std::string eq = flag + "=";
    const std::string sh = std::string("-") + shortName;

    for (std::size_t i = 0; i < args.size(); ++i) {
        if (args[i].compare(0, eq.size(), eq) == 0) {
            return args[i].substr(eq.size());
        }

        if ((args[i] == flag || args[i] == sh) && i + 1 < args.size()) {
            return args[i + 1];
        }
    }

    return std::string();
}

std::string companyPrefix(const std::string &base, const std::string &company) {
    if (company.empty()) {
        return base;
    }

    return base + "/c/" + urlEncode(company);
}

std::string withQuery(const std::string &path, const std::vector<std::string> &args) {
    const char *keys[][2] = {{"filter", "f"}, {"order", "o"}, {"limit", "l"}, {"start", "s"}};
    std::string query;

    for (const auto &key : keys) {
        const std::string value = optionValue(args, key[0], key[1]);

        if (value.empty()) {
            continue;
        }

        if (!query.empty()) {
            query += '&';
        }

        query += key[0];
        query += '=';
        query += urlEncode(value);
    }

    if (query.empty()) {
        return path;
    }

    return path + "?" + query;
}

std::string joinArgs(const std::vector<std::string> &args) {
    std::string out;

    for (std::size_t i = 0; i < args.size(); ++i) {
        if (i != 0) {
            out += ' ';
        }

        out += args[i];
    }

    return out;
}

} // namespace

std::string buildDisplayUrl(const std::string &baseUrl, const std::string &company,
                            const std::vector<std::string> &cliArgs) {
    if (baseUrl.empty()) {
        return joinArgs(cliArgs);
    }

    const std::string base = trimTrailingSlashes(baseUrl);
    const std::string command = cliArgs.empty() ? std::string() : cliArgs[0];

    if (command == "status") {
        return companyPrefix(base, company);
    }

    if (command == "list-companies") {
        return base + "/c";
    }

    if (command == "list-evidences") {
        return companyPrefix(base, company) + "/evidences";
    }

    if (command == "login") {
        return "POST " + base + "/login-logout/login";
    }

    if (command == "keep-alive") {
        return base + "/login-logout/session-keep-alive.js";
    }

    if (command == "record" && cliArgs.size() >= 3) {
        const std::string evidence = urlEncode(cliArgs[1]);
        const std::string action = cliArgs[2];
        const std::string root = companyPrefix(base, company) + "/" + evidence;

        if (action == "show") {
            if (cliArgs.size() >= 4 && !cliArgs[3].empty() && cliArgs[3][0] != '-') {
                return root + "/" + urlEncode(cliArgs[3]) + ".json";
            }

            return root + ".json";
        }

        if (action == "create") {
            return "POST " + root + ".json";
        }

        if (action == "update" || action == "delete") {
            const std::string verb = action == "update" ? "PUT " : "DELETE ";
            std::string id;

            if (cliArgs.size() >= 4 && !cliArgs[3].empty() && cliArgs[3][0] != '-') {
                id = urlEncode(cliArgs[3]);
            }

            if (id.empty()) {
                return verb + root + ".json";
            }

            return verb + root + "/" + id + ".json";
        }

        if (action == "properties") {
            return root + "/properties.json";
        }

        if (action == "search") {
            const std::string term = optionValue(cliArgs, "query", "q");

            if (term.empty()) {
                return root + ".json";
            }

            return root + ".json?q=" + urlEncode(term);
        }

        if (action == "list") {
            return withQuery(root + ".json", cliArgs);
        }
    }

    if (command == "query" && cliArgs.size() >= 2) {
        std::string method = optionValue(cliArgs, "method", "m");

        if (method.empty()) {
            method = "GET";
        }

        const std::string &path = cliArgs[1];

        if (path.compare(0, 4, "http") == 0) {
            return method + " " + path;
        }

        if (!path.empty() && path[0] == '/') {
            return method + " " + base + path;
        }

        return method + " " + companyPrefix(base, company) + "/" + path;
    }

    if (command == "changes") {
        const std::string action = cliArgs.size() >= 2 ? cliArgs[1] : "status";
        const std::string path = companyPrefix(base, company) + "/changes";

        if (action == "enable" || action == "register") {
            return "POST " + path;
        }

        if (action == "disable" || action == "unregister") {
            return "DELETE " + path;
        }

        return path;
    }

    return base + " " + joinArgs(cliArgs);
}

std::string webInterfaceUrl(const std::string &displayUrl) {
    std::size_t start = displayUrl.find_first_not_of(" \t");

    if (start == std::string::npos) {
        return std::string();
    }

    std::string url = displayUrl.substr(start);
    const char *verbs[] = {"GET ", "POST ", "PUT ", "DELETE ", "PATCH "};

    for (const char *verb : verbs) {
        const std::size_t length = std::char_traits<char>::length(verb);

        if (url.compare(0, length, verb) == 0) {
            url.erase(0, length);
            break;
        }
    }

    if (url.compare(0, 7, "http://") != 0 && url.compare(0, 8, "https://") != 0) {
        return std::string();
    }

    const std::size_t query = url.find_first_of("?#");

    if (query != std::string::npos) {
        url.resize(query);
    }

    if (url.size() >= 5 && url.compare(url.size() - 5, 5, ".json") == 0) {
        url.resize(url.size() - 5);
    } else if (url.size() >= 4 && url.compare(url.size() - 4, 4, ".xml") == 0) {
        url.resize(url.size() - 4);
    }

    const std::string properties = "/properties";

    if (url.size() >= properties.size() &&
        url.compare(url.size() - properties.size(), properties.size(), properties) == 0) {
        url.resize(url.size() - properties.size());
    }

    return url;
}

} // namespace abraflexitui
