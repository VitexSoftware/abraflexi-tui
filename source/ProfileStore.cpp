#include "abraflexitui/ProfileStore.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdlib>
#include <fstream>
#include <sstream>

#include <filesystem>
#include <nlohmann/json.hpp>

namespace abraflexitui {

namespace {

std::string trim(const std::string &s) {
    const auto start = s.find_first_not_of(" \t\r\n");

    if (start == std::string::npos) {
        return std::string();
    }

    const auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string jsonString(const nlohmann::json &obj, const char *key) {
    if (!obj.is_object() || !obj.contains(key) || !obj.at(key).is_string()) {
        return std::string();
    }

    return obj.at(key).get<std::string>();
}

std::map<std::string, std::string> parseEnvText(const std::string &text) {
    std::map<std::string, std::string> out;
    std::istringstream in(text);
    std::string line;

    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        std::string trimmed = trim(line);

        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }

        if (trimmed.compare(0, 7, "export ") == 0) {
            trimmed = trim(trimmed.substr(7));
        }

        const auto eq = trimmed.find('=');

        if (eq == std::string::npos || eq == 0) {
            continue;
        }

        std::string key = trim(trimmed.substr(0, eq));
        std::string value = trim(trimmed.substr(eq + 1));

        if (value.size() >= 2 && ((value.front() == '"' && value.back() == '"') ||
                                  (value.front() == '\'' && value.back() == '\''))) {
            value = value.substr(1, value.size() - 2);
        }

        out[key] = value;
    }

    return out;
}

ServerProfile profileFromMap(const std::map<std::string, std::string> &env) {
    ServerProfile profile;
    auto get = [&](const char *key) {
        auto it = env.find(key);
        return it == env.end() ? std::string() : it->second;
    };

    profile.url = get("ABRAFLEXI_URL");
    profile.login = get("ABRAFLEXI_LOGIN");

    if (profile.login.empty()) {
        profile.login = get("ABRAFLEXI_USER");
    }

    profile.password = get("ABRAFLEXI_PASSWORD");
    profile.company = get("ABRAFLEXI_COMPANY");
    profile.authSessionId = get("ABRAFLEXI_AUTHSESSID");
    profile.authMethod = profile.authSessionId.empty() ? AuthMethod::Password : AuthMethod::Token;
    return profile;
}

} // namespace

std::map<std::string, std::string> ServerProfile::processEnvironment() const {
    std::map<std::string, std::string> env;
    env["ABRAFLEXI_URL"] = url;
    env["ABRAFLEXI_COMPANY"] = company;

    if (authMethod == AuthMethod::Token) {
        env["ABRAFLEXI_AUTHSESSID"] = authSessionId;
        env["ABRAFLEXI_LOGIN"] = "";
        env["ABRAFLEXI_USER"] = "";
        env["ABRAFLEXI_PASSWORD"] = "";
    } else {
        env["ABRAFLEXI_LOGIN"] = login;
        env["ABRAFLEXI_PASSWORD"] = password;
        env["ABRAFLEXI_USER"] = "";
        env["ABRAFLEXI_AUTHSESSID"] = "";
    }

    return env;
}

ServerProfile ServerProfile::fromEnvText(const std::string &text) {
    return profileFromMap(parseEnvText(text));
}

ServerProfile ServerProfile::fromEnvFile(const std::string &path) {
    std::ifstream in(path);

    if (!in) {
        return ServerProfile();
    }

    std::ostringstream ss;
    ss << in.rdbuf();
    return fromEnvText(ss.str());
}

ServerProfile ServerProfile::fromProcessEnvironment() {
    std::map<std::string, std::string> env;
    const char *keys[] = {"ABRAFLEXI_URL",     "ABRAFLEXI_LOGIN",      "ABRAFLEXI_USER",
                          "ABRAFLEXI_PASSWORD", "ABRAFLEXI_COMPANY",    "ABRAFLEXI_AUTHSESSID"};

    for (const char *key : keys) {
        const char *value = std::getenv(key);

        if (value != nullptr) {
            env[key] = value;
        }
    }

    return profileFromMap(env);
}

ServerProfile ServerProfile::officialDemo() {
    ServerProfile profile;
    profile.name = "demo";
    profile.url = "https://demo.flexibee.eu:5434";
    profile.login = "winstrom";
    profile.password = "winstrom";
    profile.company = "demo";
    profile.authMethod = AuthMethod::Password;
    return profile;
}

std::string ProfileStore::defaultConfigPath() {
    const char *xdg = std::getenv("XDG_CONFIG_HOME");
    std::string dir;

    if (xdg != nullptr && xdg[0] != '\0') {
        dir = xdg;
    } else {
        const char *home = std::getenv("HOME");

        if (home == nullptr || home[0] == '\0') {
            return std::string();
        }

        dir = std::string(home) + "/.config";
    }

    return dir + "/abraflexi-tui/servers.json";
}

ProfileStore::ProfileStore(std::string path) : path_(std::move(path)) {
    if (path_.empty()) {
        path_ = defaultConfigPath();
    }
}

const ServerProfile *ProfileStore::active() const {
    return find(active_);
}

const ServerProfile *ProfileStore::find(const std::string &name) const {
    if (name.empty()) {
        return nullptr;
    }

    for (const auto &profile : profiles_) {
        if (profile.name == name) {
            return &profile;
        }
    }

    return nullptr;
}

LoadResult ProfileStore::load(std::string &error) {
    error.clear();
    profiles_.clear();
    active_.clear();
    queryFormat_ = QueryFormat::Json;

    if (path_.empty()) {
        error = "HOME is not set; cannot locate servers.json";
        return LoadResult::Error;
    }

    std::ifstream in(path_);

    if (!in) {
        return LoadResult::Missing;
    }

    nlohmann::json doc;

    try {
        in >> doc;
    } catch (const nlohmann::json::parse_error &e) {
        error = std::string("Cannot parse ") + path_ + ": " + e.what();
        return LoadResult::Error;
    }

    if (!doc.is_object()) {
        error = path_ + " is not a JSON object";
        return LoadResult::Error;
    }

    active_ = jsonString(doc, "activeProfile");
    queryFormat_ = jsonString(doc, "queryFormat") == "xml" ? QueryFormat::Xml : QueryFormat::Json;

    if (doc.contains("profiles") && doc.at("profiles").is_array()) {
        for (const auto &item : doc.at("profiles")) {
            if (!item.is_object()) {
                continue;
            }

            ServerProfile profile;
            profile.name = jsonString(item, "name");
            profile.url = jsonString(item, "url");
            profile.login = jsonString(item, "login");
            profile.password = jsonString(item, "password");
            profile.company = jsonString(item, "company");
            profile.authSessionId = jsonString(item, "authSessionId");
            profile.authMethod = jsonString(item, "authMethod") == "token" ? AuthMethod::Token : AuthMethod::Password;

            if (!profile.name.empty()) {
                profiles_.push_back(std::move(profile));
            }
        }
    }

    if (find(active_) == nullptr) {
        active_.clear();
    }

    return LoadResult::Ok;
}

bool ProfileStore::save(std::string &error) const {
    error.clear();

    if (path_.empty()) {
        error = "HOME is not set; cannot write servers.json";
        return false;
    }

    std::filesystem::path file(path_);
    std::error_code ec;
    std::filesystem::create_directories(file.parent_path(), ec);

    if (ec) {
        error = "Cannot create " + file.parent_path().string() + ": " + ec.message();
        return false;
    }

    ::chmod(file.parent_path().c_str(), 0700);

    nlohmann::json profiles = nlohmann::json::array();

    for (const auto &profile : profiles_) {
        profiles.push_back({
            {"name", profile.name},
            {"url", profile.url},
            {"login", profile.login},
            {"password", profile.password},
            {"company", profile.company},
            {"authMethod", profile.authMethod == AuthMethod::Token ? "token" : "password"},
            {"authSessionId", profile.authSessionId},
        });
    }

    nlohmann::json doc = {{"activeProfile", active_},
                          {"queryFormat", queryFormat_ == QueryFormat::Xml ? "xml" : "json"},
                          {"profiles", std::move(profiles)}};
    const std::string body = doc.dump(2) + "\n";

    const int fd = ::open(path_.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);

    if (fd < 0) {
        error = "Cannot write " + path_;
        return false;
    }

    if (::fchmod(fd, 0600) != 0) {
        ::close(fd);
        error = "Cannot set permissions on " + path_;
        return false;
    }

    std::size_t written = 0;

    while (written < body.size()) {
        const ssize_t n = ::write(fd, body.data() + written, body.size() - written);

        if (n < 0) {
            ::close(fd);
            error = "Cannot write " + path_;
            return false;
        }

        written += static_cast<std::size_t>(n);
    }

    ::close(fd);
    return true;
}

void ProfileStore::add(ServerProfile profile) {
    const std::string name = profile.name;
    profiles_.push_back(std::move(profile));

    if (active_.empty()) {
        active_ = name;
    }
}

void ProfileStore::update(const std::string &oldName, ServerProfile profile) {
    const std::string newName = profile.name;
    bool found = false;

    for (auto &existing : profiles_) {
        if (existing.name == oldName) {
            existing = std::move(profile);
            found = true;
            break;
        }
    }

    if (found && active_ == oldName) {
        active_ = newName;
    }
}

void ProfileStore::remove(const std::string &name) {
    for (auto it = profiles_.begin(); it != profiles_.end(); ++it) {
        if (it->name == name) {
            profiles_.erase(it);
            break;
        }
    }

    if (active_ == name) {
        active_ = profiles_.empty() ? std::string() : profiles_.front().name;
    }
}

void ProfileStore::setActive(const std::string &name) {
    if (find(name) != nullptr) {
        active_ = name;
    }
}

} // namespace abraflexitui
