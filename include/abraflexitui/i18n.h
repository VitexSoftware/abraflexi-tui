#pragma once

// GNU gettext wiring for abraflexi-tui. Include "abraflexitui/TV.h" first in
// any translation unit that also needs this header, since tvision headers
// are not expected to ever define an identifier named "_" but the reverse
// order keeps that assumption cheap to verify (grep for "define _(" turns up
// nothing under vendor/tvision).

#include <libintl.h>
#include <locale.h>

#include <string>

#ifndef ABRAFLEXI_TUI_GETTEXT_DOMAIN
#define ABRAFLEXI_TUI_GETTEXT_DOMAIN "abraflexi-tui"
#endif

// Marks a string as translatable and resolves it to the active language's
// catalog entry (or the English msgid itself if no translation is loaded).
#define _(s) gettext(s)

// Marks a string as translatable for extraction without translating it at
// this point (e.g. entries in a static const table); the string is used
// as-is here and translated separately when actually displayed.
#define N_(s) (s)

namespace abraflexitui {

// Sets up the process locale and binds the gettext catalog. Must be called
// once, at the very start of main(), before any TView-derived object (menu
// bar, status line, ...) is constructed, since those already call _() during
// construction.
void initI18n();

// Switches the active language at runtime. lang is "" to follow the system
// locale (LC_ALL/LANG/LANGUAGE), or an ISO 639-1 code such as "en", "cs",
// "de". Callers must rebuild any already-constructed widgets themselves to
// see the new language take effect - this only changes what subsequent _()
// calls resolve to. Best-effort: if the target language has no matching
// locale installed on the system, the switch has no visible effect and the
// previously active language stays in place.
void setLanguage(const std::string &lang);

} // namespace abraflexitui
