#include "abraflexitui/TV.h"
#include "abraflexitui/EvidenceInfoView.h"
#include "abraflexitui/JsonFormat.h"
#include "abraflexitui/SimpleListViewer.h"
#include "abraflexitui/WindowLayout.h"

#include <vector>

namespace abraflexitui {

namespace {

std::string flag(const nlohmann::json &item, const char *key) {
    if (!item.is_object() || !item.contains(key) || !item.at(key).is_boolean()) {
        return " ";
    }

    return item.at(key).get<bool>() ? "Y" : " ";
}

} // namespace

EvidenceInfoView::EvidenceInfoView(CliClient &client, std::string evidence, std::string company)
    : TWindowInit(&TWindow::initFrame),
      TWindow(TRect(2, 1, 78, 23),
              ("Structure: " + evidence + " [" + (company.empty() ? client.company() : company) + "]").c_str(),
              wnNoNumber),
      company_(company.empty() ? client.company() : std::move(company)) {
    options |= ofCentered | ofTileable;

    CliClient::Result result = client.runJsonForCompany({"record", evidence, "properties"}, company_);
    std::vector<std::string> rows;

    if (!result.ok) {
        rows.push_back("Error: " + result.errorMessage);
    } else {
        rows.push_back(fitColumn("Column", 22) + " " + fitColumn("Type", 12) + " M W  Title");

        if (result.data.contains("columns") && result.data.at("columns").is_array()) {
            for (const auto &column : result.data.at("columns")) {
                rows.push_back(fitColumn(jsonField(column, "name"), 22) + " " + fitColumn(jsonField(column, "type"), 12) +
                               " " + flag(column, "mandatory") + " " + flag(column, "writable") + "  " +
                               jsonField(column, "title"));
            }
        }

        rows.push_back(std::string());
        rows.push_back("Relations");

        if (result.data.contains("relations") && result.data.at("relations").is_array()) {
            for (const auto &relation : result.data.at("relations")) {
                rows.push_back(fitColumn(jsonField(relation, "name"), 24) + " " +
                               fitColumn(jsonField(relation, "evidenceType"), 16) + " " + jsonField(relation, "url"));
            }
        }

        rows.push_back(std::string());
        rows.push_back("Labels");

        if (result.data.contains("labels") && result.data.at("labels").is_array()) {
            for (const auto &label : result.data.at("labels")) {
                rows.push_back(fitColumn(jsonField(label, "kod"), 16) + " " + jsonField(label, "nazev"));
            }
        }
    }

    TScrollBar *vBar = standardScrollBar(sbVertical | sbHandleKeyboard);
    SimpleListViewer *list = new SimpleListViewer(getExtent().grow(-1, -1), vBar);
    growFill(list);
    list->setRows(std::move(rows));
    insert(list);
}

} // namespace abraflexitui
