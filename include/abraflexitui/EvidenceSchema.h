#pragma once

#include "abraflexitui/CliClient.h"

#include <string>
#include <vector>

namespace abraflexitui {

// One field from an evidence's AbraFlexi schema, as returned by
// `record <evidence> properties` (RecordCommand::handleProperties on the
// CLI side, ultimately sourced from php-abraflexi's offline/online
// getColumnsInfo()).
struct FieldSchema {
    std::string name;
    std::string title;
    std::string type;
    std::string relationEvidence;
    std::string relationType;
    bool mandatory = false;
    bool writable = false;
    bool visible = true;
    bool inSummary = false;
    bool inDetail = false;
    bool sortable = false;
    int maxLength = 0; // 0 = unspecified
};

// Per-evidence field schema, fetched via the CLI and cached for the
// process lifetime (the schema does not change mid-session). Every user
// of this class must tolerate an empty result (CLI failure, unknown
// evidence, offline schema not documented for this evidence) and fall
// back to today's schema-less behavior.
class EvidenceSchema {
public:
    static const std::vector<FieldSchema> &fetch(CliClient &client, const std::string &evidence,
                                                  const std::string &company = {});

    static const FieldSchema *fieldByName(const std::vector<FieldSchema> &fields, const std::string &name);
    static std::vector<std::string> summaryNames(const std::vector<FieldSchema> &fields);
    static std::vector<const FieldSchema *> writableFields(const std::vector<FieldSchema> &fields);
};

} // namespace abraflexitui
