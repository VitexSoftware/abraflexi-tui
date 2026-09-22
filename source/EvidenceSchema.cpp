#include "abraflexitui/EvidenceSchema.h"
#include "abraflexitui/JsonFormat.h"

#include <map>

namespace abraflexitui {

namespace {

bool jsonBool(const nlohmann::json &item, const char *key) {
    return item.is_object() && item.contains(key) && item.at(key).is_boolean() && item.at(key).get<bool>();
}

int jsonInt(const nlohmann::json &item, const char *key) {
    if (!item.is_object() || !item.contains(key) || !item.at(key).is_number_integer()) {
        return 0;
    }

    return item.at(key).get<int>();
}

std::map<std::string, std::vector<FieldSchema>> &cache() {
    static std::map<std::string, std::vector<FieldSchema>> instance;
    return instance;
}

} // namespace

const std::vector<FieldSchema> &EvidenceSchema::fetch(CliClient &client, const std::string &evidence,
                                                      const std::string &company) {
    auto &store = cache();
    std::string key = company.empty() ? evidence : company + ":" + evidence;
    auto it = store.find(key);

    if (it != store.end()) {
        return it->second;
    }

    std::vector<FieldSchema> fields;
    CliClient::Result result = company.empty()
                                   ? client.runJson({"record", evidence, "properties"})
                                   : client.runJsonForCompany({"record", evidence, "properties"}, company);

    if (result.ok && result.data.contains("columns") && result.data.at("columns").is_array()) {
        for (const auto &column : result.data.at("columns")) {
            FieldSchema field;
            field.name = jsonField(column, "name");
            field.title = jsonField(column, "title");
            field.type = jsonField(column, "type");
            field.relationEvidence = jsonField(column, "relationEvidence");
            field.relationType = jsonField(column, "relationType");
            field.mandatory = jsonBool(column, "mandatory");
            field.writable = jsonBool(column, "writable");
            field.visible = !column.is_object() || !column.contains("visible") || jsonBool(column, "visible");
            field.inSummary = jsonBool(column, "inSummary");
            field.inDetail = jsonBool(column, "inDetail");
            field.sortable = jsonBool(column, "sortable");
            field.maxLength = jsonInt(column, "maxLength");

            if (field.name.empty()) {
                continue;
            }

            fields.push_back(std::move(field));
        }
    }

    auto inserted = store.emplace(key, std::move(fields));
    return inserted.first->second;
}

const FieldSchema *EvidenceSchema::fieldByName(const std::vector<FieldSchema> &fields, const std::string &name) {
    for (const auto &field : fields) {
        if (field.name == name) {
            return &field;
        }
    }

    return nullptr;
}

std::vector<std::string> EvidenceSchema::summaryNames(const std::vector<FieldSchema> &fields) {
    std::vector<std::string> names;

    for (const auto &field : fields) {
        if (field.inSummary) {
            names.push_back(field.name);
        }
    }

    return names;
}

std::vector<const FieldSchema *> EvidenceSchema::writableFields(const std::vector<FieldSchema> &fields) {
    std::vector<const FieldSchema *> out;

    for (const auto &field : fields) {
        if (field.writable) {
            out.push_back(&field);
        }
    }

    return out;
}

} // namespace abraflexitui
