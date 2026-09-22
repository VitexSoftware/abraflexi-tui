cmake_minimum_required(VERSION 3.16)
project(abraflexi-tui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Vendor tvision as a submodule; disable its own example/test targets.
set(TV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(TV_BUILD_TESTS OFF CACHE BOOL "" FORCE)
# libgpm is optional console-mouse support. tvision enables it by default and
# warns when the library is absent; turn it off unless it is actually present.
find_library(GPM gpm)
if(NOT GPM)
    set(TV_BUILD_USING_GPM OFF CACHE BOOL "" FORCE)
endif()
add_subdirectory(vendor/tvision)

find_package(nlohmann_json REQUIRED)
find_package(Threads REQUIRED)

add_executable(abraflexi-tui
    source/main.cpp
    source/CliClient.cpp
    source/DisplayUrl.cpp
    source/ProfileStore.cpp
    source/SessionStore.cpp
    source/SimpleListViewer.cpp
    source/AppShell.cpp
    source/AppStatusLine.cpp
    source/AppButton.cpp
    source/ServerConfigView.cpp
    source/StatusView.cpp
    source/CompanyListView.cpp
    source/EvidenceListView.cpp
    source/EvidenceSchema.cpp
    source/RecordDetailView.cpp
    source/RecordListView.cpp
    source/RecordFieldForm.cpp
    source/RecordCreateForm.cpp
    source/RecordEditForm.cpp
    source/EvidenceInfoView.cpp
    source/QueryView.cpp
    source/SyntaxMemo.cpp
    source/CodeFormat.cpp
    source/SearchView.cpp
    source/ChangesView.cpp
    source/AboutView.cpp
    source/QrCodeView.cpp
    source/DocumentPreview.cpp
    source/PrinterUtil.cpp
    source/PrintDialog.cpp
    vendor/qrcodegen/qrcodegen.cpp
)

target_include_directories(abraflexi-tui PRIVATE include vendor/qrcodegen)
target_link_libraries(abraflexi-tui PRIVATE
    tvision::tvision
    nlohmann_json::nlohmann_json
    Threads::Threads
)

install(TARGETS abraflexi-tui RUNTIME DESTINATION bin)

# Small standalone smoke-test binary for CliClient (process spawning + JSON
# parsing) that does not require a terminal/tvision event loop, so it can be
# exercised from scripts/CI.
add_executable(abraflexi-tui-cliclient-check
    tools/cliclient_check.cpp
    source/CliClient.cpp
    source/DisplayUrl.cpp
)
target_include_directories(abraflexi-tui-cliclient-check PRIVATE include)
target_link_libraries(abraflexi-tui-cliclient-check PRIVATE
    nlohmann_json::nlohmann_json
    Threads::Threads
)

add_executable(abraflexi-tui-profile-check
    tools/profile_check.cpp
    source/CliClient.cpp
    source/DisplayUrl.cpp
    source/ProfileStore.cpp
)
target_include_directories(abraflexi-tui-profile-check PRIVATE include)
target_link_libraries(abraflexi-tui-profile-check PRIVATE
    nlohmann_json::nlohmann_json
    Threads::Threads
)
