#include "abraflexitui/RawDownload.h"

#include <fstream>

namespace abraflexitui {

bool downloadRawToFile(CliClient &client, const std::string &queryPath, const std::string &company,
                        const std::string &outFilePath, std::string &errMsg) {
    std::vector<std::string> argv = {client.binaryPath()};
    if (!client.envFile().empty()) {
        argv.push_back("--envfile=" + client.envFile());
    }
    argv.push_back("query");
    argv.push_back(queryPath);
    argv.push_back("--method=GET");

    std::map<std::string, std::string> env;
    if (!company.empty()) {
        env["ABRAFLEXI_COMPANY"] = company;
    }

    ProcessResult pr = ProcessRunner::run(argv, env);
    if (pr.spawnFailed || pr.exitCode != 0) {
        errMsg = pr.stdErr.empty() ? ("Request failed (exit code " + std::to_string(pr.exitCode) + ")") : pr.stdErr;
        return false;
    }

    std::ofstream ofs(outFilePath, std::ios::binary);
    if (!ofs) {
        errMsg = "Failed to write to " + outFilePath;
        return false;
    }
    ofs.write(pr.stdOut.data(), static_cast<std::streamsize>(pr.stdOut.size()));
    ofs.close();

    return true;
}

} // namespace abraflexitui
