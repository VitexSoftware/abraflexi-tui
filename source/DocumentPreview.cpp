#include "abraflexitui/TV.h"
#include "abraflexitui/DocumentPreview.h"
#include "abraflexitui/Commands.h"
#include "abraflexitui/JsonFormat.h"
#include "abraflexitui/SimpleListViewer.h"
#include "abraflexitui/WindowLayout.h"

#include <cctype>
#include <cstring>
#include <string>
#include <vector>

namespace abraflexitui {

namespace {

std::string urlEncode(const std::string &value) {
    static const char *hex = "0123456789ABCDEF";
    std::string out;

    for (unsigned char c : value) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out.push_back(static_cast<char>(c));
        } else {
            out.push_back('%');
            out.push_back(hex[c >> 4]);
            out.push_back(hex[c & 0x0F]);
        }
    }

    return out;
}

std::string shown(const nlohmann::json &record, const char *key) {
    const std::string asShown = jsonField(record, (std::string(key) + "@showAs").c_str());

    if (!asShown.empty()) {
        return asShown;
    }

    return jsonField(record, key);
}

std::string headline(const nlohmann::json &record) {
    std::string line = jsonField(record, "kod");
    const std::string name = jsonField(record, "nazev");

    if (!name.empty() && name != line) {
        if (!line.empty()) {
            line += "  ";
        }

        line += name;
    }

    const std::string issued = jsonField(record, "datVyst");
    const std::string due = jsonField(record, "datSplat");
    const std::string total = jsonField(record, "sumCelkem");
    const std::string currency = shown(record, "mena");

    if (!issued.empty()) {
        line += "  " + issued.substr(0, 10);
    }

    if (!due.empty()) {
        line += "  due " + due.substr(0, 10);
    }

    if (!total.empty()) {
        line += "  " + total;

        if (!currency.empty()) {
            line += " " + currency;
        }
    }

    return line;
}

std::string secondLine(const nlohmann::json &record) {
    const std::string partner = shown(record, "firma");

    if (!partner.empty()) {
        return partner;
    }

    std::string line;
    const char *keys[] = {"mesto", "email", "tel", "ic"};

    for (const char *key : keys) {
        const std::string value = jsonField(record, key);

        if (value.empty()) {
            continue;
        }

        if (!line.empty()) {
            line += "  ";
        }

        line += value;
    }

    return line;
}

TRect sideBySideRect() {
    TRect desk = TProgram::deskTop->getExtent();
    short count = 0;
    TProgram::deskTop->forEach(
        [](TView *view, void *arg) {
            if (dynamic_cast<DocumentPreview *>(view) != nullptr) {
                ++*static_cast<short *>(arg);
            }
        },
        &count);

    const short width = static_cast<short>(desk.b.x / 2);
    const short column = static_cast<short>(count % 2);
    const short shift = static_cast<short>((count / 2) % 4);
    short x = static_cast<short>(column * width);
    short y = shift;
    short right = static_cast<short>(x + width);
    short bottom = desk.b.y;

    if (right > desk.b.x) {
        right = desk.b.x;
    }

    if (bottom - y < 10) {
        y = 0;
    }

    return TRect(x, y, right, bottom);
}

class LinePrompt : public TDialog {
public:
    LinePrompt(const char *title, const char *prompt, const std::string &initial, std::string &out)
        : TWindowInit(&TDialog::initFrame),
          TDialog(TRect(0, 0, 58, 9), title),
          out_(out) {
        options |= ofCentered;
        input_ = new TInputLine(TRect(2, 3, 55, 4), 200);
        std::strncpy(input_->data, initial.c_str(), 200);
        input_->data[200] = '\0';
        insert(input_);
        insert(new TLabel(TRect(2, 2, 55, 3), prompt, input_));
        insert(new TButton(TRect(14, 6, 26, 8), "O~K~", cmOK, bfDefault));
        insert(new TButton(TRect(28, 6, 42, 8), "Cancel", cmCancel, bfNormal));
        selectNext(False);
    }

    Boolean valid(ushort command) override {
        if (command == cmOK && input_ != nullptr) {
            out_ = input_->data;
        }

        return TDialog::valid(command);
    }

private:
    TInputLine *input_;
    std::string &out_;
};

} // namespace

bool evidenceHasItems(const std::string &evidence) {
    static const char *names[] = {
        "adresar",
        "faktura-vydana",      "faktura-prijata",    "objednavka-prijata", "objednavka-vydana",
        "nabidka-vydana",      "nabidka-prijata",    "poptavka-vydana",    "poptavka-prijata",
        "banka",               "pokladni-pohyb",     "skladovy-pohyb",     "interni-doklad",
        "pohledavka",          "zavazek",
    };

    for (const char *name : names) {
        if (evidence == name) {
            return true;
        }
    }

    return false;
}

class DocumentPreview::StatusLine : public TStaticText {
public:
    StatusLine(const TRect &bounds, const std::string &text) : TStaticText(bounds, text) {
    }

    void setCaption(const std::string &value) {
        delete[] const_cast<char *>(text);
        text = newStr(value);
        drawView();
    }
};

class DocumentPreview::ItemList : public SimpleListViewer {
public:
    ItemList(const TRect &bounds, TScrollBar *bar) noexcept : SimpleListViewer(bounds, bar) {
    }
};

DocumentPreview::DocumentPreview(CliClient &client, std::string evidence, std::string id, const nlohmann::json &record)
    : TWindowInit(&TWindow::initFrame),
      TWindow(sideBySideRect(), (evidence + " " + jsonField(record, "kod")).c_str(), wnNoNumber),
      client_(client), evidence_(std::move(evidence)), id_(std::move(id)) {
    options |= ofTileable;
    growMode = gfGrowHiX | gfGrowHiY;

    if (evidence_ == "adresar") {
        relation_ = "kontakty";
        itemsLabel_ = "Contacts";
        order_ = "prijmeni@A";
        columns_ = {"jmeno", "prijmeni", "email", "tel"};
    } else {
        relation_ = "polozkyDokladu";
        itemsLabel_ = "Items";
        order_ = "id@A";
        columns_ = {"kod", "nazev", "mnozMj", "cenaMj", "sumCelkem"};
    }

    TRect inner = getExtent();
    inner.grow(-1, -1);
    const short x = inner.a.x;
    const short right = inner.b.x;
    short y = inner.a.y;

    TView *title = new TStaticText(TRect(x, y, right, y + 1), headline(record).c_str());
    growWide(title);
    insert(title);
    ++y;

    TView *partner = new TStaticText(TRect(x, y, right, y + 1), secondLine(record).c_str());
    growWide(partner);
    insert(partner);
    ++y;

    insert(new TButton(TRect(x, y, x + 12, y + 2), "~F~ilter", cmPreviewFilter, bfNormal));
    insert(new TButton(TRect(x + 13, y, x + 23, y + 2), "~S~ort", cmPreviewSort, bfNormal));
    insert(new TButton(TRect(x + 24, y, x + 36, y + 2), "~R~efresh", cmPreviewRefresh, bfNormal));
    y = static_cast<short>(y + 2);

    status_ = new StatusLine(TRect(x, y, right, y + 1), "");
    growWide(status_);
    insert(status_);

    TScrollBar *bar = standardScrollBar(sbVertical | sbHandleKeyboard);
    items_ = new ItemList(TRect(x, static_cast<short>(y + 1), right, inner.b.y), bar);
    insert(items_);
    placeItems();
    reloadItems();
}

void DocumentPreview::changeBounds(const TRect &bounds) {
    TWindow::changeBounds(bounds);

    if (size.y >= 8) {
        placeItems();
    }
}

void DocumentPreview::placeItems() {
    if (items_ == nullptr) {
        return;
    }

    TRect inner = getExtent();
    inner.grow(-1, -1);
    const short top = static_cast<short>(inner.a.y + 5);

    if (inner.b.y - top < 3) {
        return;
    }

    TRect grid(inner.a.x, top, static_cast<short>(inner.b.x - 1), inner.b.y);
    items_->locate(grid);

    if (items_->vScrollBar != nullptr) {
        TRect bar(static_cast<short>(inner.b.x - 1), top, inner.b.x, inner.b.y);
        items_->vScrollBar->locate(bar);
    }
}

void DocumentPreview::showStatus() {
    if (status_ == nullptr) {
        return;
    }

    std::string line = itemsLabel_ + "  Filter: ";
    line += filter_.empty() ? "(none)" : filter_;
    line += "   Sort: ";
    line += order_.empty() ? "(none)" : order_;
    status_->setCaption(line);
}

void DocumentPreview::reloadItems() {
    showStatus();

    std::string path = evidence_ + "/" + id_ + "/" + relation_ + ".json?limit=200&detail=custom:" +
                       [&]() {
                           std::string joined;

                           for (std::size_t i = 0; i < columns_.size(); ++i) {
                               if (i != 0) {
                                   joined += ',';
                               }

                               joined += columns_[i];
                           }

                           return joined;
                       }();

    if (!order_.empty()) {
        path += "&order=" + urlEncode(order_);
    }

    if (!filter_.empty()) {
        path += "&filter=" + urlEncode(filter_);
    }

    CliClient::Result result = client_.runJson({"query", path, "--method=GET"});
    std::vector<std::string> rows;
    std::string header;

    for (std::size_t i = 0; i < columns_.size(); ++i) {
        const std::size_t width = (columns_[i] == "nazev" || columns_[i] == "email" || columns_[i] == "prijmeni") ? 22 : 12;
        header += fitColumn(columns_[i], width);

        if (i + 1 < columns_.size()) {
            header += " ";
        }
    }

    rows.push_back(header);

    if (!result.ok) {
        rows.push_back(result.errorMessage.empty() ? "Could not load items" : result.errorMessage);
        items_->setRows(std::move(rows));
        return;
    }

    const nlohmann::json *body = &result.data;

    if (body->is_object() && body->contains("body")) {
        body = &body->at("body");
    }

    if (body->is_object() && body->contains("winstrom") && body->at("winstrom").is_object()) {
        body = &body->at("winstrom");
    }

    const nlohmann::json *records = nullptr;

    if (body->is_object()) {
        for (auto it = body->begin(); it != body->end(); ++it) {
            if (it.value().is_array()) {
                records = &it.value();
                break;
            }
        }
    }

    if (records == nullptr || records->empty()) {
        rows.push_back("(no " + itemsLabel_ + ")");
    } else {
        for (const auto &rec : *records) {
            std::string line;

            for (std::size_t i = 0; i < columns_.size(); ++i) {
                const std::size_t width = (columns_[i] == "nazev" || columns_[i] == "email" || columns_[i] == "prijmeni") ? 22 : 12;
                line += fitColumn(jsonField(rec, columns_[i].c_str()), width);

                if (i + 1 < columns_.size()) {
                    line += " ";
                }
            }

            rows.push_back(std::move(line));
        }
    }

    items_->setRows(std::move(rows));
}

void DocumentPreview::askFilter() {
    std::string value = filter_;

    if (TProgram::application->executeDialog(new LinePrompt("Filter", "Filter, for example nazev BEGINS 'A'", filter_, value)) ==
        cmOK) {
        filter_ = value;
        reloadItems();
    }
}

void DocumentPreview::askSort() {
    std::string value = order_;

    if (TProgram::application->executeDialog(
            new LinePrompt("Sort", "Order, for example nazev@A or sumCelkem@D", order_, value)) == cmOK) {
        order_ = value;
        reloadItems();
    }
}

void DocumentPreview::handleEvent(TEvent &event) {
    TWindow::handleEvent(event);

    if (event.what == evCommand) {
        switch (event.message.command) {
        case cmPreviewFilter:
            askFilter();
            clearEvent(event);
            break;

        case cmPreviewSort:
            askSort();
            clearEvent(event);
            break;

        case cmPreviewRefresh:
            reloadItems();
            clearEvent(event);
            break;

        default:
            break;
        }
    } else if (event.what == evKeyDown && event.keyDown.keyCode == kbF5) {
        reloadItems();
        clearEvent(event);
    }
}

} // namespace abraflexitui
