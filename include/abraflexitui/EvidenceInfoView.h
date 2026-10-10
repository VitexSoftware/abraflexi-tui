#pragma once

#include "abraflexitui/TV.h"
#include "abraflexitui/CliClient.h"
#include "abraflexitui/SimpleListViewer.h"

#include <string>
#include <vector>

namespace abraflexitui {

class RecordListView;

// Flexplorer evidence Info tab: columns, relations and labels.
//
// When opened from a RecordListView's "Info" button (`recordList` non-null),
// each column row in the "Structure" section is prefixed with a Y/space
// indicator showing whether that field is currently listed in the owner's
// Columns input. Pressing Enter, or clicking a row, toggles that field in
// the owner's Columns input, closes this window and makes the owner
// re-request its record list with the updated columns.
class EvidenceInfoView : public TWindow {
public:
    EvidenceInfoView(CliClient &client, std::string evidence, std::string company = {},
                      RecordListView *recordList = nullptr);

    // Called by the embedded list box when the focused row is activated
    // (Enter or mouse click). No-op for rows that don't map to a toggleable
    // field (header/blank lines, Relations, Labels) or when this view was
    // not opened from a RecordListView.
    void applyQuery(const std::string &query);
    void toggleFocusedField(short item);
    void handleEvent(TEvent &event) override;
    void changeBounds(const TRect &bounds) override;
    TColorAttr mapColor(uchar index) override;

private:
    void setQueryText(const std::string &query);
    void placeList();

    std::string company_;
    RecordListView *recordList_;
    SimpleListViewer *list_ = nullptr;
    TInputLine *queryInput_ = nullptr;
    std::string query_;
    // Full structure listing. rowFields_ stays aligned with the rows currently shown.
    std::vector<std::string> allRows_;
    std::vector<std::string> allFields_;
    std::vector<std::string> rowFields_;
};

} // namespace abraflexitui
