#include "abraflexitui/i18n.h"

#include <cstdlib>
#include <cstring>
#include <initializer_list>

#include <climits>
#include <sys/stat.h>
#include <unistd.h>

#if defined(__GLIBC__)
// glibc's gettext() caches resolved translations per "locale generation
// counter"; changing LANGUAGE alone does not invalidate that cache within
// the same process. Incrementing this internal counter is the technique
// documented in the GNU gettext manual ("13.3 Additional functions for the
// format string language") for switching languages at runtime without
// re-executing the process. It must be declared at global scope: it names
// glibc's actual global symbol, not one local to this namespace.
extern "C" int _nl_msg_cat_cntr;
#endif

namespace abraflexitui {

namespace {

bool isDirectory(const std::string &path) {
    struct stat st{};
    return ::stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

// The directory the running executable lives in, or "" if it can't be
// determined (e.g. /proc is unavailable). Linux-only, which is fine: this
// app only ships as a Debian package.
std::string executableDir() {
    char buf[PATH_MAX];
    const ssize_t len = ::readlink("/proc/self/exe", buf, sizeof(buf) - 1);

    if (len <= 0) {
        return std::string();
    }

    buf[len] = '\0';
    const std::string exe(buf);
    const std::size_t slash = exe.find_last_of('/');
    return slash == std::string::npos ? std::string() : exe.substr(0, slash);
}

std::string localeDir() {
    // Lets a developer run the freshly built binary straight out of the
    // build tree (e.g. build/abraflexi-tui) without installing anything
    // system-wide first: CMake compiles po/*.po next to the binary, under
    // build/locale, which is not on the compiled-in install path
    // (ABRAFLEXI_TUI_LOCALEDIR, e.g. /usr/local/share/locale or
    // /usr/share/locale for a real package) - so running the raw build
    // output found no catalogs and silently stayed in English no matter
    // what language was picked.
    if (const char *override_ = std::getenv("ABRAFLEXI_TUI_LOCALEDIR")) {
        return override_;
    }

    const std::string exeDir = executableDir();

    if (!exeDir.empty() && isDirectory(exeDir + "/locale")) {
        return exeDir + "/locale";
    }

    return ABRAFLEXI_TUI_LOCALEDIR;
}

} // namespace

namespace {

// glibc treats the "C"/"POSIX" locale - and, by design, "C.UTF-8" as well,
// since it exists only to add UTF-8 handling on top of the C locale - as "no
// locale is configured", and in that state gettext() always returns the
// msgid unmodified, ignoring LANGUAGE entirely. That is harmless for the
// initial English-by-default case (msgid already is the English text), but
// it means the in-app Czech/German switch below needs LC_MESSAGES to
// actually be a real, non-C locale to have any effect - setting LANGUAGE
// alone is not enough once the process is running under C/C.UTF-8. Try a
// handful of common spellings for each target language's locale name so the
// switch still works without requiring the user to have their session
// locale preconfigured; if none of them are installed, the switch silently
// has no effect and the UI keeps whatever language was active before.
bool trySetLocale(std::initializer_list<const char *> candidates) {
    for (const char *candidate : candidates) {
        if (setlocale(LC_ALL, candidate) != nullptr) {
            return true;
        }
    }

    return false;
}

} // namespace

void initI18n() {
    setlocale(LC_ALL, "");

    // With no locale configured in the environment (LC_ALL/LC_MESSAGES/LANG
    // all unset or "C"/"POSIX"), fall back to C.UTF-8 so tvision's own
    // UTF-8/character-width handling still works. Message translation stays
    // off in that case, which is fine: msgid is already the English source
    // text, so the app is correctly displayed in English.
    const char *current = setlocale(LC_ALL, nullptr);

    if (current == nullptr || std::strcmp(current, "C") == 0 || std::strcmp(current, "POSIX") == 0) {
        setlocale(LC_ALL, "C.UTF-8");
    }

    bindtextdomain(ABRAFLEXI_TUI_GETTEXT_DOMAIN, localeDir().c_str());
    bind_textdomain_codeset(ABRAFLEXI_TUI_GETTEXT_DOMAIN, "UTF-8");
    textdomain(ABRAFLEXI_TUI_GETTEXT_DOMAIN);
}

void setLanguage(const std::string &lang) {
    if (lang.empty()) {
        unsetenv("LANGUAGE");
        // Re-derive the locale from the environment, in case a previous
        // call here had pinned it to one specific language's locale.
        setlocale(LC_ALL, "");
    } else {
        setenv("LANGUAGE", lang.c_str(), 1);

        if (lang == "cs") {
            trySetLocale({"cs_CZ.UTF-8", "cs_CZ.utf8", "cs_CZ"});
        } else if (lang == "de") {
            trySetLocale({"de_DE.UTF-8", "de_DE.utf8", "de_DE"});
        } else if (lang == "en") {
            trySetLocale({"en_US.UTF-8", "en_US.utf8", "en_GB.UTF-8", "en_GB.utf8"});
        }
    }

#if defined(__GLIBC__)
    ++_nl_msg_cat_cntr;
#endif
}

} // namespace abraflexitui
