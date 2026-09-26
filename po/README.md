# Translating abraflexi-tui

Source strings (`msgid`) are English. Translated catalogs live in this
directory as `<lang>.po` (currently `cs.po` for Czech and `de.po` for
German) and are compiled to `.mo` files at build time.

## Keep the `~X~` hotkey marker

Turbo Vision underlines one letter per menu label as its keyboard-navigation
hotkey, marked in the source string as `~X~` around that letter, e.g.
`"~F~ind..."` underlines the F. When translating:

- Keep **exactly one** `~X~` pair somewhere in your translation - pick
  whichever letter makes sense in your language, it does not have to be the
  same letter as the English source.
- Make sure the hotkey letter you choose does not repeat another hotkey
  already used by a **sibling** item in the same submenu (items inside a
  different submenu, or the submenu's own top-level shortcut key, don't need
  to avoid collision, since they are only ever visible one at a time).
- `%d`/`%s` placeholders (in the game's HUD text) must stay in the same
  relative order; only the surrounding words need translating.
- Plain language names in the Language submenu ("English", "Čeština",
  "Deutsch") are hardcoded in the source, not run through gettext - by
  convention a language names itself in its own tongue, so these should stay
  untranslated in every catalog.

## Updating a catalog after source strings change

From the build directory:

```
cmake --build . --target pot-update   # regenerates po/abraflexi-tui.pot from source
cmake --build . --target update-po-cs # merges new/changed strings into po/cs.po
cmake --build . --target update-po-de # same, for po/de.po
```

Then fill in any new or `#, fuzzy` entries by hand and rebuild.

## Adding a new language

1. Add its two-letter code to `ABRAFLEXI_TUI_LANGUAGES` in `CMakeLists.txt`.
2. `msginit --input=po/abraflexi-tui.pot --locale=<code> --output=po/<code>.po`
3. Translate every `msgstr`, then `msgfmt --check --statistics po/<code>.po`
   to verify.
4. Add a menu entry (and `cmLang<Name>` command) in `source/AppShell.cpp` /
   `include/abraflexitui/Commands.h` so it's selectable from the app's
   Language menu, not just picked up automatically from the system locale.

## Known limitation of the in-app Language menu

Switching language from the menu (Language submenu) immediately rebuilds
only the menu bar and status line. Any window or dialog already open at the
time of the switch keeps its old-language captions until it is closed and
reopened - tvision widgets store their label text at construction time and
have no mechanism to observe a later language change. This is intentional
for now: rebuilding every open view from scratch on every language switch
would be a much larger, more invasive change than the switch itself
warrants.

## Only `AppShell.cpp`/`GameView.cpp` are translated so far

`po/POTFILES.in` currently lists only the two files with the confirmed
mixed-language bug (main menu bar/status line, and the Easter-egg game's HUD
text). The rest of the application's ~300-400 English UI strings across
other windows/dialogs are not yet wrapped in `_()` - add a file to
`po/POTFILES.in` as you convert it, then run `pot-update`/`update-po-*`
and translate the newly appearing entries.
