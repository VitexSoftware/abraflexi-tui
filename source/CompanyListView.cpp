#include "abraflexitui/TV.h"
#include "abraflexitui/CompanyListView.h"
#include "abraflexitui/JsonFormat.h"
#include "abraflexitui/WindowLayout.h"

#include <vector>

namespace abraflexitui {

namespace {

class CompanyListBox : public SimpleListViewer {
public:
    CompanyListBox(const TRect &bounds, TScrollBar *bar, std::function<void()> onActivate) noexcept
        : SimpleListViewer(bounds, bar), onActivate_(std::move(onActivate)) {
    }

    void handleEvent(TEvent &event) override {
        if ((event.what == evMouseDown && (event.mouse.eventFlags & meDoubleClick)) ||
            (event.what == evKeyDown && event.keyDown.keyCode == kbEnter)) {
            onActivate_();
            clearEvent(event);
            return;
        }

        SimpleListViewer::handleEvent(event);
    }

private:
    std::function<void()> onActivate_;
};

} // namespace

CompanyListView::CompanyListView(CliClient &client, std::function<void(const std::string &)> onChosen)
    : TWindowInit(&TWindow::initFrame),
      TWindow(TRect(2, 1, 78, 20), "Companies", wnNoNumber),
      onChosen_(std::move(onChosen)) {
    options |= ofCentered;

    TRect inner = getExtent();
    inner.grow(-1, -1);
    TView *hint = new TStaticText(TRect(inner.a.x, inner.a.y, inner.b.x, inner.a.y + 1), "Enter chooses the company");
    growWide(hint);
    insert(hint);
    inner.a.y += 1;

    TScrollBar *vBar = standardScrollBar(sbVertical | sbHandleKeyboard);
    list_ = new CompanyListBox(inner, vBar, [this]() { chooseFocused(); });
    growFill(list_);
    insert(list_);

    CliClient::Result result = client.runJson({"list-companies"});
    std::vector<std::string> rows;

    if (!result.ok) {
        rows.push_back("Error: " + result.errorMessage);
        dbNames_.push_back(std::string());
    } else if (!result.data.is_array() || result.data.empty()) {
        rows.push_back("(no companies found)");
        dbNames_.push_back(std::string());
    } else {
        rows.push_back(fitColumn("DB Name", 20) + " " + fitColumn("Name", 30) + " " + "Status");
        dbNames_.push_back(std::string());

        for (const auto &company : result.data) {
            rows.push_back(fitColumn(jsonField(company, "dbName"), 20) + " " +
                           fitColumn(jsonField(company, "nazev"), 30) + " " + jsonField(company, "stavEnum"));
            dbNames_.push_back(jsonField(company, "dbName"));
        }
    }

    list_->setRows(std::move(rows));
}

void CompanyListView::chooseFocused() {
    const short index = list_->focused;

    if (index < 0 || static_cast<std::size_t>(index) >= dbNames_.size() || dbNames_[static_cast<std::size_t>(index)].empty() ||
        !onChosen_) {
        return;
    }

    onChosen_(dbNames_[static_cast<std::size_t>(index)]);
}

} // namespace abraflexitui
