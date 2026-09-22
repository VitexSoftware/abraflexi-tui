#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

// Deliberately independent of tvision (no TV.h include) so it can be built
// and exercised as a standalone binary without a terminal/event loop.
namespace abraflexitui {

struct ProcessResult {
    int exitCode = -1;
    std::string stdOut;
    std::string stdErr;
    bool spawnFailed = false;
};

// Runs a child process from an argv vector via fork+execvp (never a shell),
// so no argument -- filter strings, JSON payloads, evidence names -- can be
// interpreted as shell syntax. extraEnv is applied with setenv in the child
// only; the parent environment is left unchanged. An empty value is still
// set, so it overrides a variable inherited from the parent.
class ProcessRunner {
public:
    static ProcessResult run(const std::vector<std::string> &argv,
                             const std::map<std::string, std::string> &extraEnv = {});
};

// Thin JSON-returning wrapper around abraflexi-cli. This is the only place
// that appends --format=json; there is no text-mode parsing anywhere in this
// codebase.
class CliClient {
public:
    explicit CliClient(std::string binaryPath = "abraflexi-cli", std::string envFilePath = "");

    struct Result {
        bool ok = false;
        nlohmann::json data;
        std::string errorMessage;
        int exitCode = -1;
    };

    Result runJson(const std::vector<std::string> &args) const;

    // One-shot environment for calls that must not use the saved profile
    // (the "Get Token" login). Skips --envfile. Not persisted.
    Result runJsonWithEnv(const std::vector<std::string> &args,
                          const std::map<std::string, std::string> &extraEnv) const;

    void setBinaryPath(std::string path);
    void setEnvFile(std::string path);
    void setProfileEnvironment(std::string url, std::string company, std::map<std::string, std::string> env);
    void clearProfileEnvironment();
    void setRequestObserver(std::function<void(const std::string &)> observer);

    const std::string &binaryPath() const { return binaryPath_; }
    const std::string &envFile() const { return envFilePath_; }

private:
    Result runJsonImpl(const std::vector<std::string> &args, const std::map<std::string, std::string> *overrideEnv) const;
    void notifyRequest(const std::map<std::string, std::string> &env, const std::vector<std::string> &args) const;

    std::string binaryPath_;
    std::string envFilePath_;
    std::string displayUrl_;
    std::string displayCompany_;
    std::map<std::string, std::string> extraEnv_;
    bool useProfileEnv_ = false;
    mutable std::function<void(const std::string &)> requestObserver_;
};

} // namespace abraflexitui
