#include "abraflexitui/PrintDialog.h"
#include "abraflexitui/AppButton.h"
#include "abraflexitui/PrinterUtil.h"
#include "abraflexitui/JsonFormat.h"

#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <unistd.h>

namespace abraflexitui {

namespace {

constexpr ushort cmDoPrint = 1090;

class SimpleStringListBox : public TListViewer {
public:
    SimpleStringListBox(const TRect &bounds, TScrollBar *bar) noexcept : TListViewer(bounds, 1, nullptr, bar) {
    }

    void setItems(std::vector<std::string> items) {
        items_ = std::move(items);
        setRange(static_cast<short>(items_.size()));
        drawView();
    }

    void getText(char *dest, short item, short maxLen) override {
        if (item >= 0 && static_cast<std::size_t>(item) < items_.size()) {
            std::strncpy(dest, items_[static_cast<std::size_t>(item)].c_str(), static_cast<std::size_t>(maxLen));
            dest[maxLen] = '\0';
        } else {
            dest[0] = '\0';
        }
    }

    const std::vector<std::string> &items() const { return items_; }

private:
    std::vector<std::string> items_;
};

} // namespace

PrintDialog::PrintDialog(CliClient &client, std::string evidence, std::string recordId, std::string company)
    : TWindowInit(&TDialog::initFrame),
      TDialog(TRect(0, 0, 72, 22), ("Print: " + evidence + " " + recordId).c_str()),
      client_(client), evidence_(std::move(evidence)), recordId_(std::move(recordId)),
      company_(company.empty() ? client.company() : std::move(company)) {
    options |= ofCentered;

    short x = 2;
    short y = 2;

    // Source Selection Radios
    insert(new TLabel(TRect(x, y, x + 16, y + 1), "Print ~S~ource:", nullptr));
    sourceRadios_ = new TRadioButtons(TRect(x + 16, y, x + 68, y + 1),
                                      new TSItem("Document PDF", new TSItem("Attachment PDF", nullptr)));
    insert(sourceRadios_);
    y += 2;

    // Report Templates Section
    insert(new TLabel(TRect(x, y, x + 32, y + 1), "~R~eport Template:", nullptr));
    y++;
    templateBar_ = standardScrollBar(sbVertical | sbHandleKeyboard);
    auto *tplBox = new SimpleStringListBox(TRect(x, y, x + 32, y + 5), templateBar_);
    insert(templateBar_);
    insert(tplBox);
    templateList_ = tplBox;

    // Attachments Section
    insert(new TLabel(TRect(x + 35, y - 1, x + 68, y), "PDF ~A~ttachment:", nullptr));
    attachmentBar_ = standardScrollBar(sbVertical | sbHandleKeyboard);
    auto *attBox = new SimpleStringListBox(TRect(x + 35, y, x + 68, y + 5), attachmentBar_);
    insert(attachmentBar_);
    insert(attBox);
    attachmentList_ = attBox;

    y += 6;

    // Printers Section
    insert(new TLabel(TRect(x, y, x + 68, y + 1), "Target CUPS ~P~rinter:", nullptr));
    y++;
    printerBar_ = standardScrollBar(sbVertical | sbHandleKeyboard);
    auto *prtBox = new SimpleStringListBox(TRect(x, y, x + 68, y + 5), printerBar_);
    insert(printerBar_);
    insert(prtBox);
    printerList_ = prtBox;

    y += 6;

    // Buttons
    insert(new AppButton(TRect(x, y, x + 16, y + 2), "~P~rint", cmDoPrint, bfDefault));
    insert(new AppButton(TRect(x + 18, y, x + 32, y + 2), "Cancel", cmCancel, bfNormal));

    loadReportsAndAttachments();
    updatePrinterList();

    selectNext(False);
}

void PrintDialog::loadReportsAndAttachments() {
    // 1. Fetch available report templates
    reports_.clear();
    reports_.push_back({"default", "(Default Template)"});

    std::string reportsPath = evidence_ + "/reports.json";
    CliClient::Result rResult = client_.runJsonForCompany({"query", reportsPath, "--method=GET"}, company_);
    if (rResult.ok) {
        const nlohmann::json *body = &rResult.data;
        if (body->is_object() && body->contains("body")) {
            body = &body->at("body");
        }
        if (body->is_object() && body->contains("winstrom")) {
            body = &body->at("winstrom");
        }
        if (body->is_object() && body->contains("report")) {
            const auto &reps = body->at("report");
            if (reps.is_array()) {
                for (const auto &rep : reps) {
                    std::string id = jsonField(rep, "reportId");
                    if (id.empty()) id = jsonField(rep, "id");
                    std::string name = jsonField(rep, "nazev");
                    if (name.empty()) name = id;
                    if (!id.empty()) {
                        reports_.push_back({id, name + " [" + id + "]"});
                    }
                }
            } else if (reps.is_object()) {
                std::string id = jsonField(reps, "reportId");
                if (id.empty()) id = jsonField(reps, "id");
                std::string name = jsonField(reps, "nazev");
                if (name.empty()) name = id;
                if (!id.empty()) {
                    reports_.push_back({id, name + " [" + id + "]"});
                }
            }
        }
    }

    std::vector<std::string> tplDisplay;
    for (const auto &r : reports_) {
        tplDisplay.push_back(r.name);
    }
    static_cast<SimpleStringListBox *>(templateList_)->setItems(tplDisplay);

    // 2. Fetch attachments for the record
    attachments_.clear();
    std::string attPath = evidence_ + "/" + recordId_ + "/prilohy.json";
    CliClient::Result aResult = client_.runJsonForCompany({"query", attPath, "--method=GET"}, company_);
    if (aResult.ok) {
        const nlohmann::json *body = &aResult.data;
        if (body->is_object() && body->contains("body")) {
            body = &body->at("body");
        }
        if (body->is_object() && body->contains("winstrom")) {
            body = &body->at("winstrom");
        }
        if (body->is_object() && body->contains("priloha")) {
            const auto &atts = body->at("priloha");
            auto processAtt = [&](const nlohmann::json &att) {
                std::string id = jsonField(att, "id");
                std::string name = jsonField(att, "nazev");
                if (name.empty()) name = jsonField(att, "filename");
                std::string contentType = jsonField(att, "contentType");
                attachments_.push_back({id, name, contentType});
            };

            if (atts.is_array()) {
                for (const auto &att : atts) processAtt(att);
            } else if (atts.is_object()) {
                processAtt(atts);
            }
        }
    }

    std::vector<std::string> attDisplay;
    if (attachments_.empty()) {
        attDisplay.push_back("(No attachments)");
    } else {
        for (const auto &a : attachments_) {
            attDisplay.push_back(a.name.empty() ? ("Attachment #" + a.id) : a.name);
        }
    }
    static_cast<SimpleStringListBox *>(attachmentList_)->setItems(attDisplay);
}

void PrintDialog::updatePrinterList() {
    std::vector<PrinterInfo> printers = PrinterUtil::getAvailablePrinters();
    printerNames_.clear();
    std::vector<std::string> display;

    for (const auto &p : printers) {
        printerNames_.push_back(p.name);
        display.push_back(p.name + (p.isDefault ? " (default)" : ""));
    }

    static_cast<SimpleStringListBox *>(printerList_)->setItems(display);
}

bool PrintDialog::downloadRaw(const std::string &queryPath, std::string &outFilePath, std::string &errMsg) {
    char tmpPattern[] = "/tmp/abraflexi_print_XXXXXX.pdf";
    int fd = mkstemps(tmpPattern, 4);
    if (fd < 0) {
        errMsg = "Failed to create temporary PDF file.";
        return false;
    }
    ::close(fd);
    outFilePath = tmpPattern;

    // Use ProcessRunner to call abraflexi-cli query path directly
    std::vector<std::string> argv = {client_.binaryPath()};
    if (!client_.envFile().empty()) {
        argv.push_back("--envfile=" + client_.envFile());
    }
    argv.push_back("query");
    argv.push_back(queryPath);
    argv.push_back("--method=GET");

    std::map<std::string, std::string> env;
    if (!company_.empty()) {
        env["ABRAFLEXI_COMPANY"] = company_;
    }

    ProcessResult pr = ProcessRunner::run(argv, env);
    if (pr.spawnFailed || pr.exitCode != 0) {
        errMsg = pr.stdErr.empty() ? ("Failed to fetch PDF (exit code " + std::to_string(pr.exitCode) + ")") : pr.stdErr;
        std::remove(outFilePath.c_str());
        return false;
    }

    std::ofstream ofs(outFilePath, std::ios::binary);
    if (!ofs) {
        errMsg = "Failed to write PDF data to temporary file.";
        std::remove(outFilePath.c_str());
        return false;
    }
    ofs.write(pr.stdOut.data(), pr.stdOut.size());
    ofs.close();

    return true;
}

void PrintDialog::doPrint() {
    short selPrinter = printerList_->focused;
    std::string printerName;
    if (selPrinter >= 0 && static_cast<std::size_t>(selPrinter) < printerNames_.size()) {
        printerName = printerNames_[static_cast<std::size_t>(selPrinter)];
    }

    ushort sourceChoice = 0;
    if (sourceRadios_ != nullptr) {
        sourceRadios_->getData(&sourceChoice);
    }

    short selAtt = attachmentList_->focused;
    short selTpl = templateList_->focused;

    std::string pdfPath;
    std::string errMsg;

    if (sourceChoice == 1) {
        // Print selected attachment
        if (attachments_.empty() || selAtt < 0 || static_cast<std::size_t>(selAtt) >= attachments_.size()) {
            messageBox("No attachment selected.", mfError | mfOKButton);
            return;
        }
        const auto &att = attachments_[static_cast<std::size_t>(selAtt)];
        std::string path = "priloha/" + att.id + "/content";
        if (!downloadRaw(path, pdfPath, errMsg)) {
            messageBox("Attachment download failed: " + errMsg, mfError | mfOKButton);
            return;
        }
    } else {
        // Print document report PDF
        std::string reportParam;
        if (selTpl >= 0 && static_cast<std::size_t>(selTpl) < reports_.size()) {
            if (reports_[static_cast<std::size_t>(selTpl)].id != "default") {
                reportParam = "?report-name=" + reports_[static_cast<std::size_t>(selTpl)].id;
            }
        }
        std::string path = evidence_ + "/" + recordId_ + ".pdf" + reportParam;
        if (!downloadRaw(path, pdfPath, errMsg)) {
            messageBox("Document PDF download failed: " + errMsg, mfError | mfOKButton);
            return;
        }
    }

    // Send PDF to printer via CUPS
    std::string printErr;
    bool ok = PrinterUtil::printPdfFile(pdfPath, printerName, printErr);

    std::remove(pdfPath.c_str());

    if (!ok) {
        messageBox("Print error: " + printErr, mfError | mfOKButton);
    } else {
        messageBox("Print job submitted successfully to " + (printerName.empty() ? "default printer" : printerName),
                   mfInformation | mfOKButton);
        endModal(cmOK);
    }
}

void PrintDialog::handleEvent(TEvent &event) {
    TDialog::handleEvent(event);

    if (event.what == evCommand && event.message.command == cmDoPrint) {
        doPrint();
        clearEvent(event);
    }
}

} // namespace abraflexitui
