#pragma once

#include <string>
#include <vector>

namespace abraflexitui {

// Approximate AbraFlexi REST URL for the status line. abraflexi-tui never
// calls the API itself; this only mirrors the request that the CLI arguments
// correspond to ({url}/c/{company}/{evidence}[/{id}].json[?query]).
std::string buildDisplayUrl(const std::string &baseUrl, const std::string &company,
                            const std::vector<std::string> &cliArgs);

} // namespace abraflexitui
