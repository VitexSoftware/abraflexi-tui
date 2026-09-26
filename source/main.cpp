#include "abraflexitui/TV.h"
#include "abraflexitui/AppShell.h"
#include "abraflexitui/ProfileStore.h"
#include "abraflexitui/i18n.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

namespace {

std::string envOr(const char *name, const std::string &fallback) {
    const char *v = std::getenv(name);
    return (v != nullptr) ? std::string(v) : fallback;
}

bool takeFlag(const std::string &arg, const char *prefix, std::string &out) {
    std::size_t len = std::strlen(prefix);

    if (arg.compare(0, len, prefix) == 0) {
        out = arg.substr(len);
        return true;
    }

    return false;
}

} // namespace

int main(int argc, char **argv) {
    abraflexitui::initI18n();

    std::string cliBinary = envOr("ABRAFLEXI_TUI_CLI", "abraflexi-cli");
    std::string envFile;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        std::string value;

        if (takeFlag(arg, "--cli=", value)) {
            cliBinary = value;
        } else if (takeFlag(arg, "--envfile=", value)) {
            envFile = value;
        }
    }

    abraflexitui::ProfileStore store;
    std::string storeError;
    const abraflexitui::LoadResult loaded = store.load(storeError);

    if (loaded == abraflexitui::LoadResult::Error) {
        std::cerr << "abraflexi-tui: " << storeError << "\n";
    }

    if (loaded == abraflexitui::LoadResult::Missing) {
        abraflexitui::ServerProfile boot;

        if (!envFile.empty()) {
            boot = abraflexitui::ServerProfile::fromEnvFile(envFile);
        }

        if (boot.url.empty()) {
            boot = abraflexitui::ServerProfile::fromProcessEnvironment();
        }

        if (boot.url.empty()) {
            boot = abraflexitui::ServerProfile::officialDemo();
        } else {
            boot.name = "default";
        }

        store.add(std::move(boot));

        if (!store.save(storeError)) {
            std::cerr << "abraflexi-tui: " << storeError << "\n";
        }
    }

    abraflexitui::AbraFlexiApp app;
    app.configure(cliBinary, envFile, std::move(store));
    app.run();

    return 0;
}
