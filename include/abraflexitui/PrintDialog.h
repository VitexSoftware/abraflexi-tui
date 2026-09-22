#pragma once

#include "abraflexitui/TV.h"
#include "abraflexitui/CliClient.h"

#include <string>
#include <vector>

namespace abraflexitui {

struct ReportTemplate {
    std::string id;
    std::string name;
};

struct AttachmentInfo {
    std::string id;
    std::string name;
    std::string contentType;
};

class PrintDialog : public TDialog {
public:
    PrintDialog(CliClient &client, std::string evidence, std::string recordId, std::string company = {});

    void handleEvent(TEvent &event) override;

private:
    void loadReportsAndAttachments();
    void updatePrinterList();
    void doPrint();
    bool downloadRaw(const std::string &queryPath, std::string &outFilePath, std::string &errMsg);

    CliClient &client_;
    std::string evidence_;
    std::string recordId_;
    std::string company_;

    std::vector<ReportTemplate> reports_;
    std::vector<AttachmentInfo> attachments_;
    std::vector<std::string> printerNames_;

    TRadioButtons *sourceRadios_ = nullptr;
    TListViewer *templateList_ = nullptr;
    TListViewer *attachmentList_ = nullptr;
    TListViewer *printerList_ = nullptr;

    TScrollBar *templateBar_ = nullptr;
    TScrollBar *attachmentBar_ = nullptr;
    TScrollBar *printerBar_ = nullptr;
};

} // namespace abraflexitui
