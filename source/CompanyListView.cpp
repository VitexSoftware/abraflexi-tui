#include "abraflexitui/CompanyListView.h"
#include "abraflexitui/JsonFormat.h"
#include "abraflexitui/TextFold.h"
#include "abraflexitui/WindowColors.h"
#include "abraflexitui/WindowLayout.h"

#include <cstring>
#include <vector>

namespace abraflexitui {

namespace {

class CompanyListBox : public SimpleListViewer {
public:
  CompanyListBox(const TRect &bounds, TScrollBar *bar,
                 std::function<void()> onActivate) noexcept
      : SimpleListViewer(bounds, bar), onActivate_(std::move(onActivate)) {}

  void handleEvent(TEvent &event) override {
    if ((event.what == evMouseDown &&
         (event.mouse.eventFlags & meDoubleClick)) ||
        (event.what == evKeyDown && event.keyDown.keyCode == kbEnter)) {
      onActivate_();
      clearEvent(event);
      return;
    }

    SimpleListViewer::handleEvent(event);
  }

private:
  std::function<void()> onActivate_;
  // TODO: Change default company to chosen
};

class QueryLine : public TInputLine {
public:
  QueryLine(const TRect &bounds, CompanyListView &owner) noexcept
      : TInputLine(bounds, 80), owner_(owner) {}

  void handleEvent(TEvent &event) override {
    TInputLine::handleEvent(event);

    if (data != nullptr && std::string(data) != seen_) {
      seen_ = data;
      owner_.applyQuery(seen_);
    }
  }

  void remember(const std::string &text) { seen_ = text; }

private:
  CompanyListView &owner_;
  std::string seen_;
};

} // namespace

CompanyListView::CompanyListView(
    CliClient &client, std::function<void(const std::string &)> onChosen)
    : TWindowInit(&TWindow::initFrame),
      TWindow(TRect(2, 1, 78, 20), "Companies", wnNoNumber),
      onChosen_(std::move(onChosen)) {
  options |= ofCentered | ofTileable;

  TRect inner = getExtent();
  inner.grow(-1, -1);
  TView *hint =
      new TStaticText(TRect(inner.a.x, inner.a.y, inner.b.x, inner.a.y + 1),
                      "Type to narrow. Enter chooses the company");
  growWide(hint);
  insert(hint);

  insert(new TStaticText(TRect(inner.a.x, inner.a.y + 1, inner.a.x + 6, inner.a.y + 2), "Find:"));
  queryInput_ = new QueryLine(TRect(inner.a.x + 6, inner.a.y + 1, inner.b.x, inner.a.y + 2), *this);
  growWide(queryInput_);
  insert(queryInput_);

  TScrollBar *vBar = standardScrollBar(sbVertical | sbHandleKeyboard);
  list_ = new CompanyListBox(TRect(inner.a.x, inner.a.y + 2, inner.b.x, inner.b.y), vBar,
                             [this]() { chooseFocused(); });
  insert(list_);
  placeList();

  CliClient::Result result = client.runJson({"list-companies"});

  if (!result.ok) {
    allRows_.push_back("Error: " + result.errorMessage);
    allDbNames_.push_back(std::string());
  } else if (!result.data.is_array() || result.data.empty()) {
    allRows_.push_back("(no companies found)");
    allDbNames_.push_back(std::string());
  } else {
    allRows_.push_back(fitColumn("DB Name", 20) + " " + fitColumn("Name", 30) + " " + "Status");
    allDbNames_.push_back(std::string());

    for (const auto &company : result.data) {
      std::string dbName = jsonField(company, "dbName");

      if (dbName.empty()) {
        dbName = jsonField(company, "dbNazev");
      }

      allRows_.push_back(fitColumn(dbName, 20) + " " + fitColumn(jsonField(company, "nazev"), 30) + " " +
                         jsonField(company, "stavEnum"));
      allDbNames_.push_back(dbName);
    }
  }

  applyQuery(std::string());
  queryInput_->select();
}

void CompanyListView::applyQuery(const std::string &query) {
  if (query == query_ && !dbNames_.empty()) {
    return;
  }

  query_ = query;
  std::vector<std::string> rows;
  dbNames_.clear();
  bool anyCompany = false;

  for (std::size_t i = 0; i < allRows_.size() && i < allDbNames_.size(); ++i) {
    if (!allDbNames_[i].empty() && !foldedContains(allRows_[i], query_)) {
      continue;
    }

    if (allDbNames_[i].empty() && !query_.empty() && !allRows_[i].empty() &&
        allRows_[i].compare(0, 6, "Error:") != 0 && allRows_[i].compare(0, 1, "(") != 0) {
      // Keep the column header only while the query is empty. A typed query
      // shows matching companies, or "(no match)" below.
      continue;
    }

    rows.push_back(allRows_[i]);
    dbNames_.push_back(allDbNames_[i]);

    if (!allDbNames_[i].empty()) {
      anyCompany = true;
    }
  }

  if (!anyCompany && !query_.empty()) {
    rows.push_back("(no match)");
    dbNames_.push_back(std::string());
  }

  list_->setRows(std::move(rows));
}

void CompanyListView::setQueryText(const std::string &query) {
  if (queryInput_ == nullptr) {
    return;
  }

  std::strncpy(queryInput_->data, query.c_str(), static_cast<std::size_t>(queryInput_->maxLen));
  queryInput_->data[queryInput_->maxLen] = '\0';
  const int length = static_cast<int>(std::strlen(queryInput_->data));
  queryInput_->curPos = length;
  queryInput_->selStart = length;
  queryInput_->selEnd = length;
  queryInput_->firstPos = 0;
  static_cast<QueryLine *>(queryInput_)->remember(query);
  queryInput_->drawView();
  applyQuery(query);
}

void CompanyListView::placeList() {
  if (list_ == nullptr) {
    return;
  }

  TRect inner = getExtent().grow(-1, -1);

  if (queryInput_ != nullptr) {
    TRect field(inner.a.x + 6, inner.a.y + 1, inner.b.x, inner.a.y + 2);
    queryInput_->locate(field);
  }

  if (inner.b.y - inner.a.y < 4) {
    return;
  }

  TRect grid(inner.a.x, inner.a.y + 2, static_cast<short>(inner.b.x - 1), inner.b.y);
  list_->locate(grid);

  if (list_->vScrollBar != nullptr) {
    TRect bar(static_cast<short>(inner.b.x - 1), inner.a.y + 2, inner.b.x, inner.b.y);
    list_->vScrollBar->locate(bar);
  }
}

void CompanyListView::changeBounds(const TRect &bounds) {
  TWindow::changeBounds(bounds);
  placeList();
}

void CompanyListView::handleEvent(TEvent &event) {
  TWindow::handleEvent(event);

  if (event.what == evKeyDown && current == list_ && list_ != nullptr) {
    if (event.keyDown.keyCode == kbBack) {
      std::string next = query_;
      popUtf8(next);
      setQueryText(next);
      clearEvent(event);
      return;
    }

    const TStringView typed = event.keyDown.getText();

    if (typed.size() > 0 && event.keyDown.text[0] >= 32) {
      setQueryText(query_ + std::string(typed.data(), typed.size()));
      clearEvent(event);
    }
  }

  if (event.what == evKeyDown && current == queryInput_ && list_ != nullptr) {
    if (event.keyDown.keyCode == kbEnter) {
      chooseFocused();
      clearEvent(event);
      return;
    }

    short target = list_->focused;
    bool moved = true;

    switch (event.keyDown.keyCode) {
    case kbDown:
      target = static_cast<short>(target + 1);
      break;
    case kbUp:
      target = static_cast<short>(target - 1);
      break;
    case kbPgDn:
      target = static_cast<short>(target + (list_->size.y > 0 ? list_->size.y : 1));
      break;
    case kbPgUp:
      target = static_cast<short>(target - (list_->size.y > 0 ? list_->size.y : 1));
      break;
    default:
      moved = false;
      break;
    }

    if (moved) {
      list_->focusItemNum(target);
      clearEvent(event);
    }
  }
}

void CompanyListView::chooseFocused() {
  const short index = list_->focused;

  if (index < 0 || static_cast<std::size_t>(index) >= dbNames_.size() ||
      dbNames_[static_cast<std::size_t>(index)].empty() || !onChosen_) {
    return;
  }

  onChosen_(dbNames_[static_cast<std::size_t>(index)]);
  close();
}

TColorAttr CompanyListView::mapColor(uchar index) {
    TColorAttr color;
    return windowColor(index, color) ? color : TWindow::mapColor(index);
}

} // namespace abraflexitui
