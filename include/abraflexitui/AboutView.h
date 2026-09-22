#pragma once

#include "abraflexitui/TV.h"
#include "abraflexitui/SimpleListViewer.h"

#include <string>
#include <vector>

namespace abraflexitui {

// Version and homepages of the libraries and utilities this program uses.
class AboutView : public TDialog {
public:
    AboutView();

    void handleEvent(TEvent &event) override;

private:
    void openSelected();

    SimpleListViewer *links_;
    std::vector<std::string> urls_;
};

} // namespace abraflexitui
