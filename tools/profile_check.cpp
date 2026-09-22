#include "abraflexitui/CliClient.h"
#include "abraflexitui/DisplayUrl.h"
#include "abraflexitui/ProfileStore.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace {

int failures = 0;

void check(bool ok, const char *expr, int line) {
    if (!ok) {
        std::cerr << "FAIL line " << line << ": " << expr << "\n";
        ++failures;
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

bool envHasLine(const std::string &text, const std::string &line) {
    const std::string needle = line + "\n";
    return text.find(needle) != std::string::npos || (text.size() >= line.size() && text.compare(text.size() - line.size(), line.size(), line) == 0);
}

} // namespace

int main() {
    using abraflexitui::AuthMethod;
    using abraflexitui::ServerProfile;
    using abraflexitui::buildDisplayUrl;
    using abraflexitui::webInterfaceUrl;

    CHECK(buildDisplayUrl("https://demo.flexibee.eu:5434/", "demo", {"status"}) ==
          "https://demo.flexibee.eu:5434/c/demo");
    CHECK(buildDisplayUrl("https://demo.flexibee.eu", "demo", {"list-companies"}) == "https://demo.flexibee.eu/c");
    CHECK(buildDisplayUrl("https://demo.flexibee.eu", "demo", {"list-evidences"}) ==
          "https://demo.flexibee.eu/c/demo/evidences");
    CHECK(buildDisplayUrl("https://demo.flexibee.eu", "demo", {"record", "faktura-vydana", "show", "2"}) ==
          "https://demo.flexibee.eu/c/demo/faktura-vydana/2.json");
    CHECK(buildDisplayUrl("https://demo.flexibee.eu", "demo",
                          {"record", "adresar", "list", "--order=kod", "--limit=20"}) ==
          "https://demo.flexibee.eu/c/demo/adresar.json?order=kod&limit=20");
    CHECK(buildDisplayUrl("https://demo.flexibee.eu", "demo",
                          {"record", "adresar", "list", "--filter=nazev BEGINS 'A'", "--start=5"}) ==
          "https://demo.flexibee.eu/c/demo/adresar.json?filter=nazev%20BEGINS%20%27A%27&start=5");
    CHECK(buildDisplayUrl("https://demo.flexibee.eu", "demo", {"record", "adresar", "create"}) ==
          "POST https://demo.flexibee.eu/c/demo/adresar.json");
    CHECK(buildDisplayUrl("https://demo.flexibee.eu", "demo", {"record", "adresar", "update", "7"}) ==
          "PUT https://demo.flexibee.eu/c/demo/adresar/7.json");
    CHECK(buildDisplayUrl("https://demo.flexibee.eu", "demo", {"record", "adresar", "delete", "7"}) ==
          "DELETE https://demo.flexibee.eu/c/demo/adresar/7.json");
    CHECK(buildDisplayUrl("https://demo.flexibee.eu", "demo", {"record", "adresar", "properties"}) ==
          "https://demo.flexibee.eu/c/demo/adresar/properties.json");
    CHECK(buildDisplayUrl("https://demo.flexibee.eu", "demo", {"record", "adresar", "search", "--query=Novak"}) ==
          "https://demo.flexibee.eu/c/demo/adresar.json?q=Novak");
    CHECK(buildDisplayUrl("https://demo.flexibee.eu", "demo", {"query", "adresar.json", "--method=GET"}) ==
          "GET https://demo.flexibee.eu/c/demo/adresar.json");
    CHECK(buildDisplayUrl("https://demo.flexibee.eu", "demo", {"query", "/status.json", "--method=GET"}) ==
          "GET https://demo.flexibee.eu/status.json");
    CHECK(buildDisplayUrl("https://demo.flexibee.eu", "demo", {"changes", "status"}) ==
          "https://demo.flexibee.eu/c/demo/changes");
    CHECK(buildDisplayUrl("https://demo.flexibee.eu", "demo", {"changes", "register", "--url=https://example.test/hook"}) ==
          "POST https://demo.flexibee.eu/c/demo/changes");
    CHECK(buildDisplayUrl("https://demo.flexibee.eu", "", {"login"}) == "POST https://demo.flexibee.eu/login-logout/login");
    CHECK(buildDisplayUrl("", "", {"status"}) == "status");
    CHECK(webInterfaceUrl("https://demo.flexibee.eu:5434/c/demo") == "https://demo.flexibee.eu:5434/c/demo");
    CHECK(webInterfaceUrl("https://demo.flexibee.eu/c/demo/faktura-vydana/2.json") ==
          "https://demo.flexibee.eu/c/demo/faktura-vydana/2");
    CHECK(webInterfaceUrl("GET https://demo.flexibee.eu/c/demo/adresar.json?limit=5") ==
          "https://demo.flexibee.eu/c/demo/adresar");
    CHECK(webInterfaceUrl("POST https://demo.flexibee.eu/c/demo/adresar.json") ==
          "https://demo.flexibee.eu/c/demo/adresar");
    CHECK(webInterfaceUrl("https://demo.flexibee.eu/c/demo/adresar/properties.json") ==
          "https://demo.flexibee.eu/c/demo/adresar");
    CHECK(webInterfaceUrl("status").empty());

    ServerProfile official = ServerProfile::officialDemo();
    CHECK(official.name == "demo");
    CHECK(official.url == "https://demo.flexibee.eu:5434");
    CHECK(official.login == "winstrom");
    CHECK(official.password == "winstrom");
    CHECK(official.company == "demo");
    CHECK(official.authMethod == AuthMethod::Password);

    const std::string envText = "export ABRAFLEXI_URL=https://demo.flexibee.eu:5434\n"
                                "ABRAFLEXI_LOGIN=\"winstrom\"\n"
                                "ABRAFLEXI_PASSWORD='secret #1'\n"
                                "ABRAFLEXI_COMPANY=demo_de\n"
                                "# comment\n";
    ServerProfile parsed = ServerProfile::fromEnvText(envText);
    CHECK(parsed.url == "https://demo.flexibee.eu:5434");
    CHECK(parsed.login == "winstrom");
    CHECK(parsed.password == "secret #1");
    CHECK(parsed.company == "demo_de");
    CHECK(parsed.authMethod == AuthMethod::Password);

    const std::string tokenText = "ABRAFLEXI_URL=https://demo.flexibee.eu\nABRAFLEXI_AUTHSESSID=abc\n";
    ServerProfile tokenProfile = ServerProfile::fromEnvText(tokenText);
    CHECK(tokenProfile.authMethod == AuthMethod::Token);
    CHECK(tokenProfile.authSessionId == "abc");

    auto tokenEnv = tokenProfile.processEnvironment();
    CHECK(tokenEnv["ABRAFLEXI_AUTHSESSID"] == "abc");
    CHECK(tokenEnv["ABRAFLEXI_PASSWORD"].empty());
    CHECK(tokenEnv["ABRAFLEXI_LOGIN"].empty());

    parsed.authMethod = AuthMethod::Password;
    auto passwordEnv = parsed.processEnvironment();
    CHECK(passwordEnv["ABRAFLEXI_LOGIN"] == "winstrom");
    CHECK(passwordEnv["ABRAFLEXI_AUTHSESSID"].empty());
    CHECK(passwordEnv["ABRAFLEXI_PASSWORD"] == "secret #1");

    char tmpl[] = "/tmp/abraflexi-tui-profile-XXXXXX";
    char *dir = ::mkdtemp(tmpl);

    if (dir == nullptr) {
        std::cerr << "FAIL mkdtemp\n";
        return 1;
    }

    const std::string path = std::string(dir) + "/servers.json";
    abraflexitui::ProfileStore store(path);
    std::string error;
    CHECK(store.load(error) == abraflexitui::LoadResult::Missing);

    parsed.name = "demo";
    parsed.password = "p@ss \"word\"";
    store.add(parsed);
    ServerProfile second;
    second.name = "other";
    second.url = "https://other.example";
    second.authMethod = AuthMethod::Token;
    second.authSessionId = "tok";
    store.add(second);
    store.setActive("other");
    CHECK(store.save(error));

    struct stat st;
    CHECK(::stat(path.c_str(), &st) == 0);
    CHECK((st.st_mode & 0777) == 0600);

    abraflexitui::ProfileStore loaded(path);
    CHECK(loaded.load(error) == abraflexitui::LoadResult::Ok);
    CHECK(loaded.activeName() == "other");
    CHECK(loaded.profiles().size() == 2);
    const ServerProfile *demo = loaded.find("demo");
    CHECK(demo != nullptr);
    CHECK(demo->password == "p@ss \"word\"");
    CHECK(loaded.active()->authSessionId == "tok");
    CHECK(loaded.queryFormat() == abraflexitui::ProfileStore::QueryFormat::Json);
    loaded.setQueryFormat(abraflexitui::ProfileStore::QueryFormat::Xml);
    CHECK(loaded.save(error));
    abraflexitui::ProfileStore formatted(path);
    CHECK(formatted.load(error) == abraflexitui::LoadResult::Ok);
    CHECK(formatted.queryFormat() == abraflexitui::ProfileStore::QueryFormat::Xml);

    loaded.remove("other");
    CHECK(loaded.activeName() == "demo");
    loaded.update("demo", second);
    CHECK(loaded.find("other") != nullptr);
    CHECK(loaded.activeName() == "other");

    const char *previousPassword = std::getenv("ABRAFLEXI_PASSWORD");
    const bool hadPassword = previousPassword != nullptr;
    const std::string savedPassword = hadPassword ? previousPassword : "";
    ::setenv("ABRAFLEXI_PASSWORD", "parent-secret", 1);

    abraflexitui::ProcessResult ran = abraflexitui::ProcessRunner::run(
        {"/usr/bin/env"}, {{"ABRAFLEXI_URL", "https://example.test"}, {"ABRAFLEXI_PASSWORD", ""}, {"ABRAFLEXI_TUI_CHILD_MARK", "1"}});

    if (hadPassword) {
        ::setenv("ABRAFLEXI_PASSWORD", savedPassword.c_str(), 1);
    } else {
        ::unsetenv("ABRAFLEXI_PASSWORD");
    }

    CHECK(!ran.spawnFailed);
    CHECK(ran.exitCode == 0);
    CHECK(envHasLine(ran.stdOut, "ABRAFLEXI_URL=https://example.test"));
    CHECK(envHasLine(ran.stdOut, "ABRAFLEXI_PASSWORD="));
    CHECK(ran.stdOut.find("ABRAFLEXI_PASSWORD=parent-secret") == std::string::npos);
    CHECK(std::getenv("ABRAFLEXI_TUI_CHILD_MARK") == nullptr);

    ::unlink(path.c_str());
    ::rmdir(dir);

    if (failures != 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }

    std::cout << "ok\n";
    return 0;
}
