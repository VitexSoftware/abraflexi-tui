#pragma once

#include "abraflexitui/TV.h"
#include "abraflexitui/CliClient.h"
#include "abraflexitui/ProfileStore.h"
#include "abraflexitui/SimpleListViewer.h"
#include "abraflexitui/SyntaxMemo.h"

#include <functional>

namespace abraflexitui {

// Flexplorer query form: method, path and optional body, response below.
class QueryView : public TDialog {
public:
    QueryView(CliClient &client, ProfileStore &store, std::function<void()> onFormatChanged);

    void handleEvent(TEvent &event) override;
    TColorAttr mapColor(uchar index) override;

private:
    void send();
    void setFormat(ProfileStore::QueryFormat format);
    void formatBody();

    CliClient &client_;
    ProfileStore &store_;
    std::function<void()> onFormatChanged_;
    TInputLine *method_;
    TInputLine *path_;
    TRadioButtons *format_;
    SyntaxMemo *body_;
    SimpleListViewer *result_;
};

} // namespace abraflexitui
