#pragma once

// Application command ids, kept above tvision's own reserved ranges and
// hello.cpp/examples' low-numbered demo command ids (100-102).
namespace abraflexitui {

constexpr unsigned short cmShowStatus = 1000;
constexpr unsigned short cmShowCompanies = 1001;
constexpr unsigned short cmShowEvidences = 1002;
constexpr unsigned short cmShowServerConfig = 1003;
constexpr unsigned short cmShowQuery = 1004;
constexpr unsigned short cmShowSearch = 1005;
constexpr unsigned short cmShowChanges = 1006;
constexpr unsigned short cmShowAbout = 1007;

constexpr unsigned short cmOpenRecordList = 1010;   // evidence picked -> open RecordListView
constexpr unsigned short cmRecordFocused = 1011;    // record row highlight moved
constexpr unsigned short cmRecordActivated = 1012;  // Enter/double-click on a record row
constexpr unsigned short cmRecordCreateNew = 1013;  // open RecordCreateForm
constexpr unsigned short cmRecordCreateSubmit = 1014;
constexpr unsigned short cmRecordCreateDryRun = 1015;
constexpr unsigned short cmRecordRefresh = 1016;
constexpr unsigned short cmRecordEdit = 1017;
constexpr unsigned short cmRecordDelete = 1018;
constexpr unsigned short cmShowEvidenceInfo = 1019;

constexpr unsigned short cmServerAdd = 1020;
constexpr unsigned short cmServerEdit = 1021;
constexpr unsigned short cmServerDelete = 1022;
constexpr unsigned short cmServerSetActive = 1023;
constexpr unsigned short cmServerGetToken = 1024;

constexpr unsigned short cmFormatCode = 1039;

} // namespace abraflexitui
