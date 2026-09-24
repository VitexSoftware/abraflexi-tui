#pragma once

#include "abraflexitui/TV.h"
#include "abraflexitui/CliClient.h"

#include <string>
#include <utility>
#include <vector>

namespace abraflexitui {

// Modal "download this record" dialog: pick an export format (from
// AbraFlexi's Formats.json catalog, see FormatsCatalog) and a destination
// path on disk, then fetch the record in that format and save it there.
// Modeled on PrintDialog, which does the same GET-and-save-raw-bytes dance
// but always to a temp PDF handed to CUPS instead of a chosen path.
class DownloadDialog : public TDialog {
public:
    DownloadDialog(CliClient &client, std::string evidence, std::string recordId, std::string company = {});

    void handleEvent(TEvent &event) override;
    TColorAttr mapColor(uchar index) override;

private:
    void updateDefaultPath();
    void browseForPath();
    void doDownload();

    CliClient &client_;
    std::string evidence_;
    std::string recordId_;
    std::string company_;

    std::vector<std::pair<std::string, std::string>> formats_; // {label, extension}

    TListViewer *formatList_ = nullptr;
    TScrollBar *formatBar_ = nullptr;
    TInputLine *pathInput_ = nullptr;
};

} // namespace abraflexitui
