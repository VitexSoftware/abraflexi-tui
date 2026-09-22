#pragma once

#include <string>
#include <vector>

namespace abraflexitui {

struct PrinterInfo {
    std::string name;
    std::string description;
    bool isDefault = false;
};

class PrinterUtil {
public:
    // Enumerates printers available on the system via CUPS (`lpstat`).
    static std::vector<PrinterInfo> getAvailablePrinters();

    // Sends the PDF file at `pdfFilePath` to CUPS using `lp`.
    // If `printerName` is empty, sends to the default printer.
    static bool printPdfFile(const std::string &pdfFilePath, const std::string &printerName, std::string &errorMessage);
};

} // namespace abraflexitui
