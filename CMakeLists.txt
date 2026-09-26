cmake_minimum_required(VERSION 3.16)
project(abraflexi-tui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Use the system tvision package (libtvision-dev) via pkg-config instead of
# building the vendored submodule.
find_package(PkgConfig REQUIRED)
pkg_check_modules(TVISION REQUIRED IMPORTED_TARGET tvision)
add_library(tvision::tvision ALIAS PkgConfig::TVISION)

find_package(nlohmann_json REQUIRED)
find_package(Threads REQUIRED)
find_package(Intl REQUIRED)

# --- Translations (GNU gettext) ---------------------------------------
# Source strings are English; po/cs.po and po/de.po hold the Czech and
# German catalogs. .mo files are compiled at build time and installed to
# the standard gettext location so LANGUAGE/LC_ALL selection works exactly
# like any other Linux application.
find_program(MSGFMT_EXECUTABLE msgfmt REQUIRED)
find_program(MSGMERGE_EXECUTABLE msgmerge)
find_program(XGETTEXT_EXECUTABLE xgettext)

set(ABRAFLEXI_TUI_LOCALEDIR "${CMAKE_INSTALL_PREFIX}/share/locale" CACHE PATH
    "Directory .mo catalogs are installed to and loaded from at runtime")
set(ABRAFLEXI_TUI_LANGUAGES cs de)
set(ABRAFLEXI_TUI_MO_FILES "")

foreach(lang ${ABRAFLEXI_TUI_LANGUAGES})
    set(po_file "${CMAKE_SOURCE_DIR}/po/${lang}.po")
    set(mo_dir "${CMAKE_BINARY_DIR}/locale/${lang}/LC_MESSAGES")
    set(mo_file "${mo_dir}/abraflexi-tui.mo")
    file(MAKE_DIRECTORY "${mo_dir}")
    add_custom_command(
        OUTPUT "${mo_file}"
        COMMAND "${MSGFMT_EXECUTABLE}" --check -o "${mo_file}" "${po_file}"
        DEPENDS "${po_file}"
        COMMENT "Compiling po/${lang}.po -> ${lang}.mo"
    )
    list(APPEND ABRAFLEXI_TUI_MO_FILES "${mo_file}")
    install(FILES "${mo_file}"
        DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/${lang}/LC_MESSAGES"
        RENAME abraflexi-tui.mo)
endforeach()

add_custom_target(translations ALL DEPENDS ${ABRAFLEXI_TUI_MO_FILES})

# Maintainer-only targets, only offered when the tools are actually present.
if(XGETTEXT_EXECUTABLE)
    add_custom_target(pot-update
        COMMAND "${XGETTEXT_EXECUTABLE}"
                --keyword=_ --keyword=N_ --from-code=UTF-8
                --package-name=abraflexi-tui --package-version=1.0
                --copyright-holder=Vitex\ Software --msgid-bugs-address=info@vitexsoftware.cz
                --add-comments=TRANSLATORS: --sort-output
                --files-from=${CMAKE_SOURCE_DIR}/po/POTFILES.in
                -D ${CMAKE_SOURCE_DIR}
                -o ${CMAKE_SOURCE_DIR}/po/abraflexi-tui.pot
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Regenerating po/abraflexi-tui.pot from source"
    )
endif()

if(MSGMERGE_EXECUTABLE)
    foreach(lang ${ABRAFLEXI_TUI_LANGUAGES})
        add_custom_target(update-po-${lang}
            COMMAND "${MSGMERGE_EXECUTABLE}" --update --backup=none
                    ${CMAKE_SOURCE_DIR}/po/${lang}.po ${CMAKE_SOURCE_DIR}/po/abraflexi-tui.pot
            COMMENT "Merging new strings into po/${lang}.po"
        )
    endforeach()
endif()

add_executable(abraflexi-tui
    source/main.cpp
    source/CliClient.cpp
    source/DisplayUrl.cpp
    source/ProfileStore.cpp
    source/SessionStore.cpp
    source/i18n.cpp
    source/SimpleListViewer.cpp
    source/WindowColors.cpp
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
    source/GameWindow.cpp
    source/GameView.cpp
    source/QrCodeView.cpp
    source/DocumentPreview.cpp
    source/PrinterUtil.cpp
    source/PrintDialog.cpp
    source/RelationPickerDialog.cpp
    source/FormatsCatalog.cpp
    source/RawDownload.cpp
    source/DownloadDialog.cpp
    vendor/qrcodegen/qrcodegen.cpp
)

target_include_directories(abraflexi-tui PRIVATE include vendor/qrcodegen)
target_link_libraries(abraflexi-tui PRIVATE
    tvision::tvision
    nlohmann_json::nlohmann_json
    Threads::Threads
    ${Intl_LIBRARIES}
)
target_include_directories(abraflexi-tui PRIVATE ${Intl_INCLUDE_DIRS})
target_compile_definitions(abraflexi-tui PRIVATE
    ABRAFLEXI_TUI_LOCALEDIR="${ABRAFLEXI_TUI_LOCALEDIR}"
    ABRAFLEXI_TUI_GETTEXT_DOMAIN="abraflexi-tui"
)
add_dependencies(abraflexi-tui translations)

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
