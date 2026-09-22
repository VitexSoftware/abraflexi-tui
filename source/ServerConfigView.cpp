#include "abraflexitui/TV.h"
#include "abraflexitui/AppButton.h"
#include "abraflexitui/ServerConfigView.h"
#include "abraflexitui/Commands.h"
#include "abraflexitui/JsonFormat.h"
#include "abraflexitui/StatusView.h"
#include "abraflexitui/WindowLayout.h"

#include <cstring>
#include <map>

namespace abraflexitui {

namespace {

constexpr int kNameLimit = 64;
constexpr int kUrlLimit = 512;
constexpr int kLoginLimit = 128;
constexpr int kPasswordLimit = 128;
constexpr int kCompanyLimit = 64;
constexpr int kTokenLimit = 2048;

std::string trim(const std::string &s) {
    const auto start = s.find_first_not_of(" \t\r\n");

    if (start == std::string::npos) {
        return std::string();
    }

    const auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

void setLine(TInputLine *line, const std::string &text) {
    std::strncpy(line->data, text.c_str(), static_cast<std::size_t>(line->maxLen));
    line->data[line->maxLen] = '\0';
}

std::string lineText(const TInputLine *line) {
    return line->data == nullptr ? std::string() : std::string(line->data);
}

class ProfileListBox : public SimpleListViewer {
public:
    ProfileListBox(const TRect &bounds, TScrollBar *bar, std::function<void()> onChoose) noexcept
        : SimpleListViewer(bounds, bar), onChoose_(std::move(onChoose)) {
    }

    void handleEvent(TEvent &event) override {
        if ((event.what == evMouseDown && (event.mouse.eventFlags & meDoubleClick)) ||
            (event.what == evKeyDown && event.keyDown.keyCode == kbEnter)) {
            onChoose_();
            clearEvent(event);
            return;
        }

        SimpleListViewer::handleEvent(event);
    }

private:
    std::function<void()> onChoose_;
};

class MaskedInputLine : public TInputLine {
public:
    MaskedInputLine(const TRect &bounds, int limit) noexcept : TInputLine(bounds, limit) {
    }

    void draw() override {
        const std::size_t n = std::strlen(data);
        const std::string original(data, n);

        for (std::size_t i = 0; i < n; ++i) {
            if (static_cast<unsigned char>(data[i]) >= 0x20) {
                data[i] = '*';
            }
        }

        TInputLine::draw();

        if (n > 0) {
            std::memcpy(data, original.data(), n);
        }

        data[n] = '\0';
    }
};

class ServerProfileForm : public TDialog {
public:
    using NameTaken = std::function<bool(const std::string &)>;

    ServerProfileForm(CliClient &client, ServerProfile *target, NameTaken nameTaken)
        : TWindowInit(&TDialog::initFrame),
          TDialog(TRect(6, 1, 74, 20), "Server profile"),
          client_(client), target_(target), nameTaken_(std::move(nameTaken)) {
        options |= ofCentered;
        makeMaximizable(*this);

        const short x = 2;
        const short right = static_cast<short>(size.x - 2);
        const short label = 16;

        insert(new TLabel(TRect(x, 2, x + label, 3), "~N~ame:", name_ = new TInputLine(TRect(x + label, 2, right, 3), kNameLimit)));
        insert(name_);
        growWide(name_);
        insert(new TLabel(TRect(x, 3, x + label, 4), "~U~RL:", url_ = new TInputLine(TRect(x + label, 3, right, 4), kUrlLimit)));
        insert(url_);
        growWide(url_);
        insert(new TLabel(TRect(x, 4, x + label, 5), "~L~ogin:", login_ = new TInputLine(TRect(x + label, 4, right, 5), kLoginLimit)));
        insert(login_);
        growWide(login_);
        insert(new TLabel(TRect(x, 5, x + label, 6), "Password:",
                          password_ = new MaskedInputLine(TRect(x + label, 5, right, 6), kPasswordLimit)));
        insert(password_);
        growWide(password_);
        insert(new TLabel(TRect(x, 6, x + label, 7), "~C~ompany:",
                          company_ = new TInputLine(TRect(x + label, 6, right, 7), kCompanyLimit)));
        insert(company_);
        growWide(company_);

        insert(new TStaticText(TRect(x, 8, x + label, 9), "Auth:"));
        auth_ = new TRadioButtons(TRect(x + label, 8, x + label + 22, 10),
                                  new TSItem("Password", new TSItem("Token", nullptr)));
        insert(auth_);

        insert(new TLabel(TRect(x, 10, x + label, 11), "Session token:",
                          token_ = new TInputLine(TRect(x + label, 10, right, 11), kTokenLimit)));
        insert(token_);
        growWide(token_);

        TView *tokenButton = new AppButton(TRect(x, 13, x + 16, 15), "Get ~T~oken...", cmServerGetToken, bfNormal);
        stickBottom(tokenButton);
        insert(tokenButton);
        TView *ok = new AppButton(TRect(right - 24, 16, right - 12, 18), "~O~K", cmOK, bfDefault);
        stickCorner(ok);
        insert(ok);
        TView *cancel = new AppButton(TRect(right - 11, 16, right, 18), "Cancel", cmCancel, bfNormal);
        stickCorner(cancel);
        insert(cancel);

        if (target_ != nullptr) {
            setLine(name_, target_->name);
            setLine(url_, target_->url);
            setLine(login_, target_->login);
            setLine(password_, target_->password);
            setLine(company_, target_->company);
            setLine(token_, target_->authSessionId);
            ushort method = target_->authMethod == AuthMethod::Token ? 1 : 0;
            auth_->setData(&method);
        }

        selectNext(False);
    }

    void handleEvent(TEvent &event) override {
        TDialog::handleEvent(event);

        if (event.what == evCommand && event.message.command == cmServerGetToken) {
            requestToken();
            clearEvent(event);
        }
    }

    Boolean valid(ushort command) override {
        if (command == cmOK) {
            ServerProfile collected;

            if (!collect(collected)) {
                return False;
            }

            if (nameTaken_ && nameTaken_(collected.name)) {
                messageBox("A profile with that name already exists.", mfError | mfOKButton);
                return False;
            }

            if (target_ != nullptr) {
                *target_ = std::move(collected);
            }
        }

        return TDialog::valid(command);
    }

private:
    bool collect(ServerProfile &out) {
        out.name = trim(lineText(name_));
        out.url = trim(lineText(url_));
        out.login = lineText(login_);
        out.password = lineText(password_);
        out.company = trim(lineText(company_));
        out.authSessionId = trim(lineText(token_));

        ushort method = 0;
        auth_->getData(&method);
        out.authMethod = method == 1 ? AuthMethod::Token : AuthMethod::Password;

        if (out.name.empty()) {
            messageBox("Profile name is required.", mfError | mfOKButton);
            return false;
        }

        if (out.url.empty()) {
            messageBox("Server URL is required.", mfError | mfOKButton);
            return false;
        }

        return true;
    }

    void requestToken() {
        const std::string url = trim(lineText(url_));
        const std::string login = lineText(login_);
        const std::string password = lineText(password_);

        if (url.empty() || login.empty() || password.empty()) {
            messageBox("URL, login and password are required to request a session token.", mfError | mfOKButton);
            return;
        }

        std::map<std::string, std::string> env = {
            {"ABRAFLEXI_URL", url},
            {"ABRAFLEXI_LOGIN", login},
            {"ABRAFLEXI_PASSWORD", password},
            {"ABRAFLEXI_COMPANY", trim(lineText(company_))},
            {"ABRAFLEXI_USER", ""},
            {"ABRAFLEXI_AUTHSESSID", ""},
        };

        CliClient::Result result = client_.runJsonWithEnv({"login"}, env);

        if (!result.ok) {
            messageBox(result.errorMessage.empty() ? std::string("Login failed") : result.errorMessage, mfError | mfOKButton);
            return;
        }

        std::string token;

        if (result.data.is_object() && result.data.contains("authSessionId") && result.data.at("authSessionId").is_string()) {
            token = result.data.at("authSessionId").get<std::string>();
        }

        if (token.empty()) {
            messageBox("Login succeeded but no authSessionId was returned.", mfError | mfOKButton);
            return;
        }

        setLine(token_, token);
        token_->drawView();

        if (messageBox("Session token received. Switch this profile to token authentication and forget the password?",
                       mfConfirmation | mfYesButton | mfNoButton) == cmYes) {
            password_->data[0] = '\0';
            password_->drawView();
            ushort method = 1;
            auth_->setData(&method);
        }
    }

    CliClient &client_;
    ServerProfile *target_;
    NameTaken nameTaken_;
    TInputLine *name_ = nullptr;
    TInputLine *url_ = nullptr;
    TInputLine *login_ = nullptr;
    TInputLine *password_ = nullptr;
    TInputLine *company_ = nullptr;
    TInputLine *token_ = nullptr;
    TRadioButtons *auth_ = nullptr;
};

} // namespace

ServerConfigView::ServerConfigView(ProfileStore &store, CliClient &client, std::function<void()> onChanged)
    : TWindowInit(&TDialog::initFrame),
      TDialog(TRect(2, 1, 78, 22), "Servers"),
      store_(store), client_(client), onChanged_(std::move(onChanged)) {
    options |= ofCentered;
    makeMaximizable(*this);

    const short x = 2;
    const short right = static_cast<short>(size.x - 2);
    const short row1 = static_cast<short>(size.y - 5);
    const short row2 = static_cast<short>(size.y - 3);

    TView *heading = new TStaticText(TRect(x, 2, right, 3), "Enter chooses a server and shows its status.");
    growWide(heading);
    insert(heading);

    TScrollBar *vBar = standardScrollBar(sbVertical | sbHandleKeyboard);
    list_ = new ProfileListBox(TRect(x, 3, right, row1), vBar, [this]() { activateSelected(); });
    growFill(list_);
    insert(list_);

    TView *add = new AppButton(TRect(x, row1, x + 12, row1 + 2), "~A~dd", cmServerAdd, bfNormal);
    stickBottom(add);
    insert(add);
    TView *edit = new AppButton(TRect(x + 13, row1, x + 25, row1 + 2), "~E~dit", cmServerEdit, bfNormal);
    stickBottom(edit);
    insert(edit);
    TView *remove = new AppButton(TRect(x + 26, row1, x + 40, row1 + 2), "~D~elete", cmServerDelete, bfNormal);
    stickBottom(remove);
    insert(remove);
    TView *activate = new AppButton(TRect(x, row2, x + 16, row2 + 2), "~S~et Active", cmServerSetActive, bfNormal);
    stickBottom(activate);
    insert(activate);
    TView *close = new AppButton(TRect(right - 12, row2, right, row2 + 2), "Close", cmCancel, bfNormal);
    stickCorner(close);
    insert(close);

    refreshList();
    selectNext(False);
}

void ServerConfigView::refreshList() {
    std::vector<std::string> rows;
    const short keep = list_->focused;

    for (const auto &profile : store_.profiles()) {
        const std::string mark = profile.name == store_.activeName() ? "*" : " ";
        const std::string method = profile.authMethod == AuthMethod::Token ? "token" : "password";
        rows.push_back(mark + " " + fitColumn(profile.name, 18) + " " + fitColumn(method, 10) + " " + profile.url);
    }

    list_->setRows(std::move(rows));

    if (keep >= 0 && static_cast<std::size_t>(keep) < list_->rowCount()) {
        list_->focusItem(static_cast<short>(keep));
    }
}

const ServerProfile *ServerConfigView::selected() const {
    if (list_ == nullptr) {
        return nullptr;
    }

    const short index = list_->focused;

    if (index < 0 || static_cast<std::size_t>(index) >= store_.profiles().size()) {
        return nullptr;
    }

    return &store_.profiles()[static_cast<std::size_t>(index)];
}

bool ServerConfigView::persist() {
    std::string error;

    if (!store_.save(error)) {
        messageBox(error.empty() ? std::string("Could not save servers.json") : error, mfError | mfOKButton);
    }

    if (onChanged_) {
        onChanged_();
    }

    refreshList();
    return error.empty();
}

void ServerConfigView::addProfile() {
    ServerProfile created;
    auto *form = new ServerProfileForm(client_, &created, [this](const std::string &name) {
        return store_.find(name) != nullptr;
    });
    const ushort code = TProgram::deskTop->execView(form);
    TObject::destroy(form);

    if (code != cmOK) {
        return;
    }

    store_.add(std::move(created));
    persist();
}

void ServerConfigView::editProfile() {
    const ServerProfile *current = selected();

    if (current == nullptr) {
        messageBox("Select a profile first.", mfError | mfOKButton);
        return;
    }

    ServerProfile edited = *current;
    const std::string original = edited.name;
    auto *form = new ServerProfileForm(client_, &edited, [this, original](const std::string &name) {
        return name != original && store_.find(name) != nullptr;
    });
    const ushort code = TProgram::deskTop->execView(form);
    TObject::destroy(form);

    if (code != cmOK) {
        return;
    }

    store_.update(original, std::move(edited));
    persist();
}

void ServerConfigView::deleteProfile() {
    const ServerProfile *current = selected();

    if (current == nullptr) {
        messageBox("Select a profile first.", mfError | mfOKButton);
        return;
    }

    const std::string name = current->name;

    if (messageBox(std::string("Delete server profile \"") + name + "\"?", mfConfirmation | mfYesButton | mfNoButton) !=
        cmYes) {
        return;
    }

    store_.remove(name);
    persist();
}

void ServerConfigView::activateSelected() {
    const ServerProfile *current = selected();

    if (current == nullptr) {
        messageBox("Select a profile first.", mfError | mfOKButton);
        return;
    }

    store_.setActive(current->name);

    if (!persist()) {
        return;
    }

    TProgram::application->executeDialog(new StatusView(client_));
}

void ServerConfigView::handleEvent(TEvent &event) {
    TDialog::handleEvent(event);

    if (event.what != evCommand) {
        return;
    }

    switch (event.message.command) {
    case cmServerAdd:
        addProfile();
        clearEvent(event);
        break;

    case cmServerEdit:
        editProfile();
        clearEvent(event);
        break;

    case cmServerDelete:
        deleteProfile();
        clearEvent(event);
        break;

    case cmServerSetActive:
        activateSelected();
        clearEvent(event);
        break;

    default:
        break;
    }
}

} // namespace abraflexitui
