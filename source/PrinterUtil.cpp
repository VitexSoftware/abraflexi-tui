#include "abraflexitui/PrinterUtil.h"
#include "abraflexitui/CliClient.h"

#include <algorithm>
#include <sstream>

namespace abraflexitui {

std::vector<PrinterInfo> PrinterUtil::getAvailablePrinters() {
    std::vector<PrinterInfo> printers;

    // Get default printer first
    ProcessResult defRes = ProcessRunner::run({"lpstat", "-d"});
    std::string defaultPrinter;
    if (defRes.exitCode == 0 && !defRes.stdOut.empty()) {
        // Output format: "system default destination: PrinterName"
        std::size_t pos = defRes.stdOut.find("system default destination: ");
        if (pos != std::string::npos) {
            std::string name = defRes.stdOut.substr(pos + 28);
            // strip trailing whitespace or newline
            std::size_t end = name.find_first_of(" \t\r\n");
            if (end != std::string::npos) {
                name = name.substr(0, end);
            }
            defaultPrinter = name;
        }
    }

    // Get printer list via lpstat -e (lists queue names) or lpstat -p
    ProcessResult listRes = ProcessRunner::run({"lpstat", "-e"});
    if (listRes.exitCode == 0 && !listRes.stdOut.empty()) {
        std::istringstream iss(listRes.stdOut);
        std::string line;
        while (std::getline(iss, line)) {
            // trim line
            std::size_t start = line.find_first_not_of(" \t\r\n");
            std::size_t end = line.find_last_not_of(" \t\r\n");
            if (start != std::string::npos && end != std::string::npos) {
                std::string name = line.substr(start, end - start + 1);
                if (!name.empty()) {
                    PrinterInfo info;
                    info.name = name;
                    info.isDefault = (!defaultPrinter.empty() && name == defaultPrinter);
                    printers.push_back(info);
                }
            }
        }
    }

    // Fallback: if lpstat -e returned nothing, try lpstat -p
    if (printers.empty()) {
        ProcessResult pRes = ProcessRunner::run({"lpstat", "-p"});
        if (pRes.exitCode == 0 && !pRes.stdOut.empty()) {
            std::istringstream iss(pRes.stdOut);
            std::string line;
            while (std::getline(iss, line)) {
                // "printer PrinterName is idle..."
                if (line.rfind("printer ", 0) == 0) {
                    std::size_t spacePos = line.find(' ', 8);
                    std::string name = (spacePos != std::string::npos) ? line.substr(8, spacePos - 8) : line.substr(8);
                    if (!name.empty()) {
                        PrinterInfo info;
                        info.name = name;
                        info.isDefault = (!defaultPrinter.empty() && name == defaultPrinter);
                        printers.push_back(info);
                    }
                }
            }
        }
    }

    // If still no printers found, add a fallback default printer entry
    if (printers.empty()) {
        PrinterInfo info;
        info.name = defaultPrinter.empty() ? "(default system printer)" : defaultPrinter;
        info.isDefault = true;
        printers.push_back(info);
    } else if (!defaultPrinter.empty()) {
        // Ensure the default printer is first in the list
        auto it = std::find_if(printers.begin(), printers.end(), [&](const PrinterInfo &p) { return p.isDefault; });
        if (it != printers.end() && it != printers.begin()) {
            std::rotate(printers.begin(), it, it + 1);
        }
    }

    return printers;
}

bool PrinterUtil::printPdfFile(const std::string &pdfFilePath, const std::string &printerName, std::string &errorMessage) {
    std::vector<std::string> args = {"lp"};

    if (!printerName.empty() && printerName != "(default system printer)") {
        args.push_back("-d");
        args.push_back(printerName);
    }

    args.push_back(pdfFilePath);

    ProcessResult res = ProcessRunner::run(args);

    if (res.spawnFailed) {
        errorMessage = "Failed to execute 'lp'. Make sure CUPS / lp client is installed.";
        return false;
    }

    if (res.exitCode != 0) {
        errorMessage = res.stdErr.empty() ? ("lp exited with code " + std::to_string(res.exitCode)) : res.stdErr;
        return false;
    }

    return true;
}

} // namespace abraflexitui
