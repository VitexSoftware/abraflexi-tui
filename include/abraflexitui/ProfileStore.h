#pragma once

#include <map>
#include <string>
#include <vector>

namespace abraflexitui {

// One saved AbraFlexi server. Passwords and session tokens are stored in
// plaintext, same as abraflexi-cli's .env; the file itself is mode 0600.
enum class AuthMethod { Password, Token };

struct ServerProfile {
    std::string name;
    std::string url;
    std::string login;
    std::string password;
    std::string company;
    std::string authSessionId;
    AuthMethod authMethod = AuthMethod::Password;

    // Environment injected into the abraflexi-cli child. Token mode sets
    // ABRAFLEXI_AUTHSESSID and blanks login/password; password mode does the
    // opposite. Empty values are still set, so a later Dotenv load cannot
    // fill the unused method back in from a .env file.
    std::map<std::string, std::string> processEnvironment() const;

    static ServerProfile fromEnvText(const std::string &text);
    static ServerProfile fromEnvFile(const std::string &path);
    static ServerProfile fromProcessEnvironment();

    // Public AbraFlexi demo used when the first launch has no configuration.
    static ServerProfile officialDemo();
};

enum class LoadResult { Missing, Ok, Error };

class ProfileStore {
public:
    // Empty path uses $XDG_CONFIG_HOME/abraflexi-tui/servers.json, or
    // ~/.config/abraflexi-tui/servers.json.
    explicit ProfileStore(std::string path = {});

    const std::string &path() const { return path_; }
    const std::vector<ServerProfile> &profiles() const { return profiles_; }
    bool empty() const { return profiles_.empty(); }

    const std::string &activeName() const { return active_; }
    const ServerProfile *active() const;
    const ServerProfile *find(const std::string &name) const;

    LoadResult load(std::string &error);
    bool save(std::string &error) const;

    void add(ServerProfile profile);
    void update(const std::string &oldName, ServerProfile profile);
    void remove(const std::string &name);
    void setActive(const std::string &name);

    enum class QueryFormat { Json, Xml };

    QueryFormat queryFormat() const { return queryFormat_; }
    void setQueryFormat(QueryFormat format) { queryFormat_ = format; }

    // "" follows the system locale; otherwise an ISO 639-1 code such as
    // "en", "cs", "de" - see abraflexitui::setLanguage() in i18n.h, which
    // this value is applied through at startup and after an in-app switch.
    const std::string &language() const { return language_; }
    void setLanguage(std::string lang) { language_ = std::move(lang); }

    static std::string defaultConfigPath();

private:
    std::string path_;
    std::string active_;
    std::vector<ServerProfile> profiles_;
    QueryFormat queryFormat_ = QueryFormat::Json;
    std::string language_;
};

} // namespace abraflexitui
