#include "abraflexitui/SessionStore.h"

#include <fcntl.h>
#include <unistd.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>

#include <nlohmann/json.hpp>

namespace abraflexitui {

namespace {

std::string jsonString(const nlohmann::json &obj, const char *key) {
    if (!obj.is_object() || !obj.contains(key) || !obj.at(key).is_string()) {
        return std::string();
    }

    return obj.at(key).get<std::string>();
}

int jsonInt(const nlohmann::json &obj, const char *key, int fallback) {
    if (!obj.is_object() || !obj.contains(key) || !obj.at(key).is_number_integer()) {
        return fallback;
    }

    return obj.at(key).get<int>();
}

} // namespace

std::string SessionStore::defaultStatePath() {
    const char *xdg = std::getenv("XDG_STATE_HOME");
    std::string dir;

    if (xdg != nullptr && xdg[0] != '\0') {
        dir = xdg;
    } else {
        const char *home = std::getenv("HOME");

        if (home == nullptr || home[0] == '\0') {
            return std::string();
        }

        dir = std::string(home) + "/.local/state";
    }

    return dir + "/abraflexi-tui/session.json";
}

SessionStore::SessionStore(std::string path) : path_(std::move(path)) {
    if (path_.empty()) {
        path_ = defaultStatePath();
    }
}

void SessionStore::load() {
    windows_.clear();
    searchTexts_.clear();
    nextId_ = 1;

    if (path_.empty()) {
        return;
    }

    std::ifstream in(path_);

    if (!in) {
        return;
    }

    nlohmann::json doc;

    try {
        in >> doc;
    } catch (const nlohmann::json::parse_error &) {
        return;
    }

    if (!doc.is_object() || !doc.contains("windows") || !doc.at("windows").is_array()) {
        return;
    }

    for (const auto &item : doc.at("windows")) {
        if (!item.is_object()) {
            continue;
        }

        WindowSession win;
        win.evidence = jsonString(item, "evidence");

        if (win.evidence.empty()) {
            continue;
        }

        win.company = jsonString(item, "company");
        win.focusedId = jsonString(item, "focusedId");
        win.id = nextId_++;

        if (item.contains("bounds") && item.at("bounds").is_object()) {
            const auto &b = item.at("bounds");
            win.bounds.x1 = jsonInt(b, "x1", 0);
            win.bounds.y1 = jsonInt(b, "y1", 0);
            win.bounds.x2 = jsonInt(b, "x2", 0);
            win.bounds.y2 = jsonInt(b, "y2", 0);
            win.hasBounds = win.bounds.x2 > win.bounds.x1 && win.bounds.y2 > win.bounds.y1;
        }

        windows_.push_back(std::move(win));
    }

    if (doc.contains("searchTexts") && doc.at("searchTexts").is_object()) {
        for (const auto &[key, value] : doc.at("searchTexts").items()) {
            if (value.is_string()) {
                searchTexts_[key] = value.get<std::string>();
            }
        }
    }
}

bool SessionStore::save() const {
    if (path_.empty()) {
        return false;
    }

    std::filesystem::path file(path_);
    std::error_code ec;
    std::filesystem::create_directories(file.parent_path(), ec);

    if (ec) {
        return false;
    }

    nlohmann::json windows = nlohmann::json::array();

    for (const auto &win : windows_) {
        nlohmann::json entry = {{"evidence", win.evidence}, {"company", win.company}, {"focusedId", win.focusedId}};

        if (win.hasBounds) {
            entry["bounds"] = {
                {"x1", win.bounds.x1}, {"y1", win.bounds.y1}, {"x2", win.bounds.x2}, {"y2", win.bounds.y2}};
        }

        windows.push_back(std::move(entry));
    }

    nlohmann::json searchTexts = nlohmann::json::object();

    for (const auto &[key, value] : searchTexts_) {
        searchTexts[key] = value;
    }

    nlohmann::json doc = {{"windows", std::move(windows)}, {"searchTexts", std::move(searchTexts)}};
    const std::string body = doc.dump(2) + "\n";

    const int fd = ::open(path_.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);

    if (fd < 0) {
        return false;
    }

    std::size_t written = 0;

    while (written < body.size()) {
        const ssize_t n = ::write(fd, body.data() + written, body.size() - written);

        if (n < 0) {
            ::close(fd);
            return false;
        }

        written += static_cast<std::size_t>(n);
    }

    ::close(fd);
    return true;
}

int SessionStore::openWindow(const std::string &evidence, const std::string &company, const std::string &focusedId, const WindowBounds *bounds) {
    WindowSession win;
    win.id = nextId_++;
    win.evidence = evidence;
    win.company = company;
    win.focusedId = focusedId;

    if (bounds != nullptr) {
        win.bounds = *bounds;
        win.hasBounds = true;
    }

    windows_.push_back(win);
    save();
    return win.id;
}

void SessionStore::updateFocused(int handle, const std::string &focusedId) {
    for (auto &win : windows_) {
        if (win.id == handle) {
            if (win.focusedId == focusedId) {
                return;
            }

            win.focusedId = focusedId;
            save();
            return;
        }
    }
}

void SessionStore::updateBounds(int handle, const WindowBounds &bounds) {
    for (auto &win : windows_) {
        if (win.id == handle) {
            win.bounds = bounds;
            win.hasBounds = true;
            save();
            return;
        }
    }
}

std::string SessionStore::searchText(const std::string &key) const {
    auto it = searchTexts_.find(key);
    return it == searchTexts_.end() ? std::string() : it->second;
}

void SessionStore::setSearchText(const std::string &key, const std::string &value) {
    auto &slot = searchTexts_[key];

    if (slot == value) {
        return;
    }

    slot = value;
    save();
}

void SessionStore::closeWindow(int handle) {
    for (auto it = windows_.begin(); it != windows_.end(); ++it) {
        if (it->id == handle) {
            windows_.erase(it);
            save();
            return;
        }
    }
}

} // namespace abraflexitui
