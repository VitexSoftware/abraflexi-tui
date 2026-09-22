// Standalone smoke test for CliClient: exercises process spawning + JSON
// parsing without starting tvision's terminal event loop, so it can be run
// from a script/CI. Usage:
//   abraflexi-tui-cliclient-check <cli-binary> [envfile] -- <cli-args...>
#include "abraflexitui/CliClient.h"

#include <iostream>

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << " <cli-binary> [envfile] -- <cli-args...>\n";
        return 2;
    }

    std::string binary = argv[1];
    std::string envFile;
    std::vector<std::string> args;

    int i = 2;

    if (i < argc && std::string(argv[i]) != "--") {
        envFile = argv[i];
        ++i;
    }

    if (i < argc && std::string(argv[i]) == "--") {
        ++i;
    }

    for (; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    if (args.empty()) {
        args.push_back("status");
    }

    abraflexitui::CliClient client(binary, envFile);
    abraflexitui::CliClient::Result result = client.runJson(args);

    std::cout << "ok=" << (result.ok ? "true" : "false") << " exitCode=" << result.exitCode << "\n";

    if (!result.errorMessage.empty()) {
        std::cout << "error=" << result.errorMessage << "\n";
    }

    std::cout << result.data.dump(2) << std::endl;

    return result.ok ? 0 : 1;
}
