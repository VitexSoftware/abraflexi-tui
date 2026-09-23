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
constexpr unsigned short cmShowWebQr = 1008;

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
constexpr unsigned short cmFieldFormPrevPage = 1044;
constexpr unsigned short cmFieldFormNextPage = 1045;
constexpr unsigned short cmFieldFormToggleRaw = 1046;
constexpr unsigned short cmOpenRecordWindow = 1040;
constexpr unsigned short cmPreviewFilter = 1041;
constexpr unsigned short cmPreviewSort = 1042;
constexpr unsigned short cmPreviewRefresh = 1043;
constexpr unsigned short cmRecordPrint = 1047;

// Standard Turbo Vision command ids (0-255) so the Window menu can disable them.
constexpr unsigned short cmMinimizeAll = 200;
constexpr unsigned short cmRestoreWindows = 201;

// Module-menu quick-open shortcuts: each opens a RecordListView pinned to a
// specific evidence and default column list (see AppShell.cpp's quickOpens
// table). Numbered from 1050 to stay clear of the ranges above.
constexpr unsigned short cmOpenAdresy = 1050;
constexpr unsigned short cmOpenKontakty = 1051;
constexpr unsigned short cmOpenFakturaVydana = 1052;
constexpr unsigned short cmOpenObjednavkaPrijata = 1053;
constexpr unsigned short cmOpenPohledavka = 1054;
constexpr unsigned short cmOpenFakturaPrijata = 1055;
constexpr unsigned short cmOpenObjednavkaVydana = 1056;
constexpr unsigned short cmOpenZavazek = 1057;
constexpr unsigned short cmOpenCenik = 1058;
constexpr unsigned short cmOpenSkladovaKarta = 1059;
constexpr unsigned short cmOpenSklad = 1060;
constexpr unsigned short cmOpenSkladovyPohyb = 1061;
constexpr unsigned short cmOpenBanka = 1062;
constexpr unsigned short cmOpenBankovniUcet = 1063;
constexpr unsigned short cmOpenPokladna = 1064;
constexpr unsigned short cmOpenPokladniPohyb = 1065;
constexpr unsigned short cmOpenUcetniDenik = 1066;
constexpr unsigned short cmOpenUcet = 1067;
constexpr unsigned short cmOpenStredisko = 1068;
constexpr unsigned short cmOpenZakazka = 1069;
constexpr unsigned short cmOpenSaldo = 1070;

} // namespace abraflexitui
