# AbraFlexi TUI

A terminal user interface for [abraflexi-cli](https://github.com/VitexSoftware/abraflexi-cli), built with the [Turbo Vision](https://github.com/magiblot/tvision) (tvision) library.

![Logo](abraflexi-tui.svg?raw=true)

## Features

- **Status screen**: configured connection info and server/company reachability (`abraflexi-cli status`)
- **Company list**: all companies on the configured server (`abraflexi-cli list-companies`)
- **Evidence browser**: all AbraFlexi evidence catalogues (`abraflexi-cli list-evidences`); Enter/double-click opens its record list
- **Generic record browser**: one evidence-parameterized list/detail screen that works for *any* AbraFlexi evidence, with editable filter, columns, limit and order fields
- **Record detail**: embedded field-by-field panel that follows the highlighted row, fetching the full record on Enter
- **Record creation**: a JSON payload editor pre-annotated with the evidence's mandatory fields, with Dry-Run, Submit and a Force checkbox
- **Record edit and delete**: the list can open the current record as JSON (`record update`) and remove it after confirmation (`record delete`)
- **Evidence structure**: columns, relations and labels for the open evidence (`record properties`), from the record list or F2 in the evidence browser
- **Find**: search one evidence, or evidence names when the evidence field is empty
- **Query**: raw REST call (method, path, optional body) through `abraflexi-cli query`
- **Changes API**: status, enable/disable, and webhook register/unregister
- **Server profiles**: several AbraFlexi servers saved in `~/.config/abraflexi-tui/servers.json` (mode 0600), switched from the Servers screen, with either a password or a session token
- **Request URL**: the status line shows the AbraFlexi URL implied by the current `abraflexi-cli` call. Click that address, or use `Alt+B` / AbraFlexi → Browser QR, to show a QR code of the same address opened in the web interface (without `.json` or a query string)

PDF preview, company backup/restore/clone, the custom-button designer and a webhook HTTP listener stay in Flexplorer. They need a browser or an HTTP endpoint.

![Screenshot](screenshot.png?raw=true)

## Keyboard Reference

### Global

| Key | Action |
|-----|--------|
| `Alt+A` | Open the AbraFlexi menu |
| `Alt+S` | Status |
| `Alt+C` | Companies. Enter on a row chooses that company; it then appears on the status line |
| `Alt+E` | Evidences |
| `Alt+V` | Servers |
| `Alt+Q` | Query |
| `Alt+F` | Find |
| `Alt+G` | Changes API |
| `Alt+B` | QR code of the web address currently shown on the status line |
| `Alt+H` | Help menu |
| `Alt+W` | Window menu |
| `F1` | About, with links to the libraries and utilities |
| `Alt+X` | Quit |
| `F6` / `Shift+F6` | Next / previous window |
| `Ctrl+F5` | Move or resize the active window |
| `Alt+F3` | Close the active window |
| `F10` | Menu |

### Window menu

Lists, record windows and the evidence browser take part in tiling. Dialogs do not.

| Command | Action |
|---------|--------|
| Tile | Split the open windows across the desktop |
| Cascade | Stack them with a stepped offset |
| Minimize all | Shrink every open window to a title bar along the bottom |
| Restore | Put minimized windows back |
| Close all | Close tiled windows and minimized title bars |
| Zoom | The frame's zoom control fills the desktop |

### Evidence Browser

| Key | Action |
|-----|--------|
| `↑/↓`, mouse wheel | Move selection |
| Type | Narrow the list by path, name or description. Backspace deletes the last character |
| `Enter`, double-click | Open the record list for the selected evidence |
| `F2` | Columns, relations and labels of the selected evidence |

### Record List

| Key | Action |
|-----|--------|
| `↑/↓` | Move selection (updates the detail panel from already-fetched row data) |
| `Enter`, double-click | Fetch and show the full record in the lower pane (`record <evidence> show <id>`) |
| `F4`, Preview | Open a read-only window. A document shows its header and line items, with Filter and Sort; anything else shows the field list. A second Preview of the same id brings that window forward |
| `F5` | Re-run the list query with the current filter/columns/limit/order |
| `F2` | Evidence structure (columns, relations, labels) |
| Filter / Columns / Limit / Order fields | Map 1:1 to `record <evidence> list -f -c -l -o` |
| Refresh / New / Edit / Delete / Info / Preview | Reload, create, edit the selected row, delete it after confirmation, open the structure window, or open a read-only preview |

### Document preview

`F4` on an invoice opens its line items. On an address-book row it opens that company's contacts (`jmeno`, `prijmeni`, `email`, `tel`). Filter and Sort apply to that listing. A company with no contacts shows `no Contacts`.

| Control | Action |
|---------|--------|
| Filter | Dialog for an item filter, for example `nazev BEGINS 'A'` |
| Sort | Dialog for item order, for example `nazev@A` or `sumCelkem@D` |
| Refresh, `F5` | Reload the line items of this document |

### New Record

| Key | Action |
|-----|--------|
| Edit the JSON text area | The record payload for `--data` |
| `Dry-~R~un` | `record <evidence> create --dry-run --data=...` |
| `~S~ubmit` | `record <evidence> create --data=...` |
| Force checkbox | Adds `--force` (skip mandatory-field validation) |
| `Format` | Pretty-print the JSON |
| `Cancel` / `Esc` | Close without creating |

### Edit Record

| Key | Action |
|-----|--------|
| Edit the JSON text area | Payload sent as `record <evidence> update <id> --data=...` |
| `Dry-Run` / `Save` | Same dry-run and submit flow as New Record |
| `Format` | Pretty-print the JSON |

### Find (`Alt+F`)

| Key | Action |
|-----|--------|
| Evidence empty | Filter evidence names from `list-evidences` |
| Evidence filled | `record <evidence> search --query=...` |
| Open | Empty id opens that evidence's record list; otherwise shows the record |

### Query (`Alt+Q`)

| Key | Action |
|-----|--------|
| Method / Path / Body | `abraflexi-cli query <path> --method=... --body=...` |
| JSON / XML | Preferred body highlighting. The choice is saved and shown on the status line |
| Format | Pretty-print the body as JSON or XML |
| Send | A relative path is sent under `/c/<company>/` |
| Zoom | The frame's maximize control fills the desktop; the editor grows with it |

### Changes (`Alt+G`)

| Key | Action |
|-----|--------|
| Enable / Disable | `abraflexi-cli changes enable` or `disable` |
| Register / Unregister | Add a webhook URL, or remove the selected hook |

### Servers

| Key | Action |
|-----|--------|
| `Add` / `Edit` / `Delete` | Change saved server profiles |
| `Set Active`, Enter, double-click | Use that profile and open the status dialog |
| `Get Token...` | Exchange the typed login and password for an `authSessionId` (not written to disk until OK) |
| Password / Token | Password sends `ABRAFLEXI_LOGIN`/`ABRAFLEXI_PASSWORD`; token sends `ABRAFLEXI_AUTHSESSID` only |

## Prerequisites

- `abraflexi-cli` installed and available on `PATH` (or pointed to via `--cli=`/`ABRAFLEXI_TUI_CLI`)
- A C++17 compiler, CMake ≥ 3.16, `libncurses-dev` (or `libncursesw5-dev` on older distributions), `nlohmann-json3-dev`

## Installation

### From Source

```bash
git clone --recurse-submodules https://github.com/VitexSoftware/abraflexi-tui.git
cd abraflexi-tui
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
sudo cmake --install build
```

### Debian Package

```bash
git submodule update --init --recursive
dpkg-buildpackage -us -uc
sudo dpkg -i ../abraflexi-tui_*.deb
```

## Usage

```bash
abraflexi-tui
```

The application opens with the AbraFlexi menu. Use `Alt+S`/`Alt+C`/`Alt+E`/`Alt+V` or the mouse to navigate.

On the first start, if `~/.config/abraflexi-tui/servers.json` does not exist yet, a `default` profile is imported from `--envfile=` when that file contains `ABRAFLEXI_URL`, otherwise from `ABRAFLEXI_*` variables already in the environment. When neither source defines a server, the public demo is saved and activated: `https://demo.flexibee.eu:5434`, login `winstrom`, company `demo`. If a profile is already active, the Status dialog opens at startup. If no profile is active, the Servers dialog opens so a configuration can be chosen. Later runs use the saved profile. `--envfile=` is still forwarded to `abraflexi-cli` when no profile is active.

To run against a development checkout of `abraflexi-cli` instead of a system install:

```bash
ABRAFLEXI_TUI_CLI=/path/to/abraflexi-cli/bin/abraflexi-cli \
    abraflexi-tui --envfile=/path/to/abraflexi-cli/.env
```

## Project Structure

```
abraflexi-tui/
├── CMakeLists.txt
├── include/abraflexitui/
│   ├── TV.h                 # Shared tvision Uses_ macro list + <tvision/tv.h>
│   ├── CliClient.h           # fork+execvp process runner + JSON wrapper
│   ├── DisplayUrl.h          # Status-line URL built from CLI arguments
│   ├── ProfileStore.h        # ~/.config/abraflexi-tui/servers.json
│   ├── SimpleListViewer.h    # Read-only TListViewer over a vector<string> of rows
│   ├── JsonFormat.h          # Small JSON -> display-string helpers
│   ├── Commands.h            # App-wide command ids
│   ├── AppShell.h            # TApplication subclass, menu bar, status line
│   ├── AppStatusLine.h       # Status line with the current request URL
│   ├── ServerConfigView.h    # Saved server profiles
│   ├── StatusView.h
│   ├── CompanyListView.h
│   ├── EvidenceListView.h
│   ├── RecordDetailView.h
│   ├── RecordListView.h      # The generic, evidence-parameterized browser
│   └── RecordCreateForm.h
├── source/                   # One .cpp per header above
├── tools/cliclient_check.cpp # Standalone CliClient smoke test (no terminal needed)
├── vendor/tvision/           # tvision, as a git submodule
├── man/abraflexi-tui.1
└── debian/                   # Debian packaging
```

## Architecture

`abraflexi-tui` never talks to the AbraFlexi REST API directly. `CliClient`
(`include/abraflexitui/CliClient.h`) runs `abraflexi-cli` via `fork()`+`execvp()`
with an argv vector (never a shell string), always appending `--format=json`,
and parses the result with [nlohmann/json](https://github.com/nlohmann/json).
Schema and validation stay in `abraflexi-cli`. Connection settings live in
the TUI as named profiles (`~/.config/abraflexi-tui/servers.json`, mode
`0600`, plaintext, same level as a `.env` file) and are injected into the
child as `ABRAFLEXI_*` variables. Password profiles set login and password
and blank `ABRAFLEXI_AUTHSESSID`. Token profiles set `ABRAFLEXI_AUTHSESSID`
and blank the password variables. The status line shows an approximate REST
URL for the call in progress (`buildDisplayUrl`).

A session token expires if it is not kept alive. This version does not ping
`keep-alive` in the background (FlexiBee expects that about once a minute).
When a token stops working, open the profile and use **Get Token...** again.

Unlike a per-entity command-line tool (compare
[multiflexi-cli](https://github.com/VitexSoftware/multiflexi-cli), which has
one Symfony-console subcommand per entity), `abraflexi-cli` exposes a single
generic `record <evidence> list|show|create` command parameterized by
evidence name at runtime. `abraflexi-tui` mirrors that: there is **one**
evidence-parameterized `RecordListView`/`RecordDetailView`/`RecordCreateForm`
set, not a source file per AbraFlexi evidence type (there are close to 250).
Adding support for a new evidence requires no code changes at all — it just
shows up in the Evidence browser.

`Update`/`Delete` are intentionally not implemented, because
`abraflexi-cli record` does not implement them either.

## Development

```bash
cmake -B build && cmake --build build -j"$(nproc)"   # Build
./build/abraflexi-tui-cliclient-check <cli-path> [envfile] -- status  # CliClient smoke test
./build/abraflexi-tui-profile-check                                  # Profiles, display URL, env injection
```

There is no automated UI test suite — tvision apps take over the terminal, so
verification is manual (see the man page's EXAMPLES section). The
`abraflexi-tui-cliclient-check` binary exercises the process-spawning and
JSON-parsing logic in isolation, without starting the terminal event loop.

## Package Information

- **Section**: utils
- **Priority**: optional
- **Maintainer**: Vitex Software <info@vitexsoftware.cz>
- **Homepage**: https://github.com/VitexSoftware/abraflexi-tui
- **Runtime dependency**: `abraflexi-cli`

## License

MIT License — see [LICENSE](LICENSE).

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## Support

For issues and questions visit the [GitHub repository](https://github.com/VitexSoftware/abraflexi-tui) or contact Vitex Software at <info@vitexsoftware.cz>.
