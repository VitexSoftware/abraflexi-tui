#pragma once

#include "abraflexitui/CliClient.h"

#include <string>

namespace abraflexitui {

// Calls `abraflexi-cli query <queryPath> --method=GET` and writes the raw
// stdout bytes to `outFilePath`. Shared by PrintDialog (which downloads to a
// throwaway temp file before handing it to CUPS) and DownloadDialog (which
// downloads straight to a user-chosen destination path).
bool downloadRawToFile(CliClient &client, const std::string &queryPath, const std::string &company,
                        const std::string &outFilePath, std::string &errMsg);

} // namespace abraflexitui
