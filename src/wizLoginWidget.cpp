#include "wizLoginWidget.h"
#include "ui_wizLoginWidget.h"
#include "utils/stylehelper.h"
#include "utils/pathresolve.h"
#include "sync/apientry.h"
#include "share/wizmisc.h"
#include "share/wizsettings.h"
#include "sync/wizkmxmlrpc.h"
#include "sync/asyncapi.h"
#include "sync/token.h"
#include <extensionsystem/pluginmanager.h>
#include <QPainter>
#include <QMouseEvent>
#include <QMenu>
#include <QFocusEvent>
#include <QBitmap>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>

using namespace WizService;

LoginLineEdit::LoginLineEdit(QWidget *parent) : QLineEdit(parent)
{
    connect(this, SIGNAL(textChanged(QString)), SLOT(on_containt_changed(QString)));
}

void LoginLineEdit::setElementStyle(const QString &strBgFile, EchoMode mode, const QString &strPlaceHoldTxt)
{
    setStyleSheet(QString("QLineEdit{ border-image:url(%1); border: 1px solid white; "
                                                     "font:16px; color:#2F2F2F; selection-background-color: #8ECAF1;}").arg(strBgFile));
    setTextMargins(40, 0, 0, 0);
    setAttribute(Qt::WA_MacShowFocusRect, false);
    setEchoMode(mode);
    setPlaceholderText(strPlaceHoldTxt);
}

void LoginLineEdit::setErrorStatus(bool bErrorStatus)
{
    if (bErrorStatus)
    {
        QString strThemeName = Utils::StyleHelper::themeName();
        QString strError = ::WizGetSkinResourceFileName(strThemeName, "loginErrorInput");
        m_extraStatus = QPixmap(strError);
    }
    else
    {
        m_extraStatus = QPixmap();
    }
}

void LoginLineEdit::on_containt_changed(const QString &strText)
{
    Q_UNUSED(strText);
    setErrorStatus(false);
    update();
}

void LoginLineEdit::paintEvent(QPaintEvent *event)
{
    QLineEdit::paintEvent(event);

    if (!m_extraStatus.isNull())
    {
        QStyleOptionFrameV3 option;
        initStyleOption(&option);

        QPainter painter(this);
        QRect rect(option.rect.right() - m_extraStatus.width() - 5, option.rect.top() + (option.rect.height() - m_extraStatus.height()) / 2,
                   m_extraStatus.width(), m_extraStatus.height());
        painter.drawPixmap(rect, m_extraStatus);
    }
}

void LoginLineEdit::mousePressEvent(QMouseEvent *event)
{
}

void LoginLineEdit::focusInEvent(QFocusEvent* event)
{
    emit editorFocusIn();
    QLineEdit::focusInEvent(event);
}

LoginButton::LoginButton(QWidget *parent) : QPushButton(parent)
{
    setFixedSize(302, 46);
}

void LoginButton::setElementStyle()
{
    QString strThemeName = Utils::StyleHelper::themeName();
    //
    QString strBtnNormal = ::WizGetSkinResourceFileName(strThemeName, "loginOKButton_normal");
    QString strBtnHover = ::WizGetSkinResourceFileName(strThemeName, "loginOKButton_hover");
    QString strBtnDown = ::WizGetSkinResourceFileName(strThemeName, "loginOKButton_down");
    setStyleSheet(QString("QPushButton{ border-image:url(%1); border: 1px solid white; font:12px}"
                        "QPushButton:hover{ border-image:url(%2); border: 1px solid white;}"
                          "QPushButton:pressed { border-image:url(%3); font:12px}")
                  .arg(strBtnNormal).arg(strBtnHover).arg(strBtnDown));
}

void LoginButton::on_password_changed(const QString &strText)
{
    QString strThemeName = Utils::StyleHelper::themeName();
    QString strBtnHover = ::WizGetSkinResourceFileName(strThemeName, "loginOKButton_hover");
    QString strBtnDown = ::WizGetSkinResourceFileName(strThemeName, "loginOKButton_down");
    if (strText.isEmpty())
    {
        QString strBtnNormal = ::WizGetSkinResourceFileName(strThemeName, "loginOKButton_normal");
        setStyleSheet(QString("QPushButton{ border-image:url(%1); border: 1px solid white; font:12px}"
                            "QPushButton:hover{ border-image:url(%2); border: 1px solid white;}"
                              "QPushButton:pressed { border-image:url(%3); font:12px}")
                      .arg(strBtnNormal).arg(strBtnHover).arg(strBtnDown));
    }
    else
    {
        QString strBtnActive = ::WizGetSkinResourceFileName(strThemeName, "loginOKButton_active");
        setStyleSheet(QString("QPushButton{ border-image:url(%1); border: 1px solid white; font:12px}"
                            "QPushButton:hover{ border-image:url(%2); border: 1px solid white;}"
                              "QPushButton:pressed { border-image:url(%3); font:12px}")
                      .arg(strBtnActive).arg(strBtnHover).arg(strBtnDown));
    }
}

LoginTipWidget::LoginTipWidget(QWidget *parent) : QWidget(parent)
{

}

CWizLoginWidget::CWizLoginWidget(const QString &strDefaultUserId, const QString &strLocale, QWidget *parent) :
    QDialog(parent)
    , ui(new Ui::wizLoginWidget)
    , m_menu(new QMenu(this))
{
    ui->setupUi(this);

    setWindowFlags(Qt::CustomizeWindowHint);
    setAutoFillBackground(true);


    setFixedSize(352, 503);
    QPalette paletteBG(palette());
    QPixmap pix(::WizGetSkinResourceFileName(Utils::StyleHelper::themeName(), "loginBackground"));
    setMask(QBitmap(pix.mask()));
    paletteBG.setBrush(QPalette::Window, QBrush(pix));
    setPalette(paletteBG);


    setElementStyles();

    connect(m_menu, SIGNAL(triggered(QAction*)), SLOT(userListMenuClicked(QAction*)));

    connect(ui->lineEdit_newPassword, SIGNAL(textChanged(QString)), SLOT(inputDataChanged()));
    connect(ui->lineEdit_newUserName, SIGNAL(textChanged(QString)), SLOT(inputDataChanged()));
    connect(ui->lineEdit_password, SIGNAL(textChanged(QString)), SLOT(inputDataChanged()));
    connect(ui->lineEdit_repaetPassword, SIGNAL(textChanged(QString)), SLOT(inputDataChanged()));
    connect(ui->lineEdit_userName, SIGNAL(textChanged(QString)), SLOT(inputDataChanged()));
    connect(ui->lineEdit_userName, SIGNAL(showMenuRequest(QPoint)), SLOT(showUserListMenu(QPoint)));
    connect(ui->lineEdit_password, SIGNAL(textChanged(QString)), ui->btn_login, SLOT(on_password_changed(QString)));

    setUsers(strDefaultUserId);
}

CWizLoginWidget::~CWizLoginWidget()
{
    delete ui;
}

QString CWizLoginWidget::userId() const
{
    return ui->lineEdit_userName->text();
}

QString CWizLoginWidget::password() const
{
    return ui->lineEdit_password->text();
}

void CWizLoginWidget::setUsers(const QString &strDefault)
{
    CWizStdStringArray usersFolder;
    ::WizEnumFolders(Utils::PathResolve::dataStorePath(), usersFolder, 0);

    for(CWizStdStringArray::const_iterator it = usersFolder.begin();
        it != usersFolder.end(); it++)
    {
        QString strPath = *it;
        QString strUserId = ::WizFolderNameByPath(strPath);

        QRegExp mailRex("\\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\\.[A-Z]{2,4}\\b");
        mailRex.setCaseSensitivity(Qt::CaseInsensitive);

        if (!mailRex.exactMatch(strUserId))
            continue;

        if (!QFile::exists(strPath + "data/index.db"))
            continue;

        m_menu->addAction(strUserId);
    }

    // set default user as default login entry.
    setUser(strDefault);
}

void CWizLoginWidget::setUser(const QString &strUserId)
{
    CWizUserSettings userSettings(strUserId);
    QString strPassword = userSettings.password();

    ui->lineEdit_userName->setText(strUserId);
    if (strPassword.isEmpty()) {
        ui->lineEdit_password->clear();
        ui->cbx_remberPassword->setCheckState(Qt::Unchecked);
    } else {
        ui->lineEdit_password->setText(strPassword);
        ui->cbx_remberPassword->setCheckState(Qt::Checked);
    }
}

void CWizLoginWidget::doAccountVerify()
{
    CWizUserSettings userSettings(userId());

    // FIXME: should verify password if network is available to avoid attack?
    if (password() != userSettings.password()) {
        Token::setUserId(userId());
        Token::setPasswd(password());
        doOnlineVerify();
        return;
    }

    if (updateUserProfile(false) && updateGlobalProfile()) {
        QDialog::accept();
    }
    enableLoginControls(true);
}

void CWizLoginWidget::doOnlineVerify()
{
    connect(Token::instance(), SIGNAL(tokenAcquired(QString)), SLOT(onTokenAcquired(QString)), Qt::QueuedConnection);
    Token::requestToken();
}

bool CWizLoginWidget::updateGlobalProfile()
{
    QSettings* settings = ExtensionSystem::PluginManager::globalSettings();
    settings->setValue("Users/DefaultUser", userId());
    return true;
}

bool CWizLoginWidget::updateUserProfile(bool bLogined)
{
    CWizUserSettings userSettings(userId());

    if(ui->cbx_autologin->checkState() == Qt::Checked) {
        userSettings.setAutoLogin(true);
    } else {
        userSettings.setAutoLogin(false);
    }

    if(ui->cbx_remberPassword->checkState() != Qt::Checked) {
        userSettings.setPassword();
    }

    if (bLogined) {
        if (ui->cbx_remberPassword->checkState() == Qt::Checked)
            userSettings.setPassword(::WizEncryptPassword(password()));

        CWizDatabase db;
        if (!db.Open(userId())) {
            //QMessageBox::critical(0, tr("Update user profile"), QObject::tr("Can not open database while update user profile"));
            ui->label_passwordError->setText(tr("Can not open database while update user profile"));
            return false;
        }

        db.SetUserInfo(Token::info());
        db.Close();
    }

    return true;
}

void CWizLoginWidget::enableLoginControls(bool bEnable)
{
    ui->lineEdit_userName->setEnabled(bEnable);
    ui->lineEdit_password->setEnabled(bEnable);
    ui->cbx_autologin->setEnabled(bEnable);
    ui->cbx_remberPassword->setEnabled(bEnable);
    ui->btn_login->setEnabled(bEnable);
    ui->btn_changeToSignin->setEnabled(bEnable);
}

void CWizLoginWidget::enableSignInControls(bool bEnable)
{
    ui->lineEdit_newUserName->setEnabled(bEnable);
    ui->lineEdit_newPassword->setEnabled(bEnable);
    ui->lineEdit_repaetPassword->setEnabled(bEnable);
    ui->btn_singin->setEnabled(bEnable);
    ui->btn_changeToLogin->setEnabled(bEnable);
}

void CWizLoginWidget::mousePressEvent(QMouseEvent *event)
{
    m_mousePoint = event->globalPos();
}

void CWizLoginWidget::mouseMoveEvent(QMouseEvent *event)
{
    if(m_mousePoint != QPoint(0, 0))
    {
        move(geometry().x() + event->globalPos().x() - m_mousePoint.x(), geometry().y() + event->globalPos().y() - m_mousePoint.y());
        m_mousePoint = event->globalPos();
    }
}

void CWizLoginWidget::mouseReleaseEvent(QMouseEvent */*event*/)
{
    m_mousePoint = QPoint(0, 0);
}



void CWizLoginWidget::on_btn_close_clicked()
{
    qApp->quit();
}

void CWizLoginWidget::setElementStyles()
{
    ui->stackedWidget->setCurrentIndex(0);

    QString strThemeName = Utils::StyleHelper::themeName();
    QString strlogo = ::WizGetSkinResourceFileName(strThemeName, "loginLogoCn");
    ui->label_logo->setStyleSheet(QString("QLabel {border: none;background-image: url(%1);"
                                        "background-position: center; background-repeat: no-repeat}").arg(strlogo));
    //
    QString strBtnCloseNormal = ::WizGetSkinResourceFileName(strThemeName, "loginCloseButton_normal");
    QString strBtnCloseHot = ::WizGetSkinResourceFileName(strThemeName, "loginCloseButton_hot");
    ui->btn_close->setStyleSheet(QString("QToolButton{ border-image:url(%1); height: 13px; width: 13px;}"
                                         "QToolButton:hover{ border-image:url(%2);}"
                                           "QToolButton:pressed { border-image:url(%3);}")
                                 .arg(strBtnCloseNormal).arg(strBtnCloseHot).arg(strBtnCloseHot));
    QString strGrayButton = ::WizGetSkinResourceFileName(strThemeName, "loginGrayButton");
    ui->btn_min->setStyleSheet(QString("QToolButton{ border-image:url(%1); height: 13px; width: 13px;}").arg(strGrayButton));
    ui->btn_max->setStyleSheet(QString("QToolButton{ border-image:url(%1); height: 13px; width: 13px;}").arg(strGrayButton));
    //
    QString strLineEditName = ::WizGetSkinResourceFileName(strThemeName, "loginTopLineEditor");
    QString strLineEditMidPassword = ::WizGetSkinResourceFileName(strThemeName, "loginMidLineEditor");
    QString strLineEditBottomPassword = ::WizGetSkinResourceFileName(strThemeName, "loginBottomLineEditor");
    ui->lineEdit_userName->setElementStyle(strLineEditName, QLineEdit::Normal, "example@mail.com");
    ui->lineEdit_password->setElementStyle(strLineEditBottomPassword, QLineEdit::Password, tr("password"));
    ui->lineEdit_newUserName->setElementStyle(strLineEditName, QLineEdit::Normal, "Please enter email as your account");
    ui->lineEdit_newPassword->setElementStyle(strLineEditMidPassword, QLineEdit::Password, tr("Please enter password"));
    ui->lineEdit_repaetPassword->setElementStyle(strLineEditBottomPassword, QLineEdit::Password, tr("Please enter password again"));
    //
    ui->btn_singin->setElementStyle();
    ui->btn_login->setElementStyle();
    //
    QString strSeparator = ::WizGetSkinResourceFileName(strThemeName, "loginSeparator");
    ui->label_separator2->setStyleSheet(QString("QLabel {border: none;background-image: url(%1);"
                                               "background-position: center; background-repeat: no-repeat}").arg(strSeparator));
   //
    ui->label_noaccount->setStyleSheet(QString("QLabel {border: none; font: 15px; color: #5f5f5f;}"));
    ui->btn_changeToSignin->setStyleSheet(QString("QPushButton { border: 1px; background: none; "
                                                 "color: #43a6e8; font: 15px; padding-left: 10px; padding-bottom: 3px}"));
    ui->btn_thridpart->setStyleSheet(QString("QPushButton { border: none; background: none; "
                                                 "color: #b1b1b1; font: 13px; padding-right: 15px; padding-bottom: 5px}"));
    ui->btn_fogetpass->setStyleSheet(QString("QPushButton { border: none; background: none; "
                                                 "color: #b1b1b1; font: 13px; padding-left: 15px; padding-bottom: 5px}"));

    QString strLineSeparator = ::WizGetSkinResourceFileName(strThemeName, "loginLineSeparator");
    ui->label_separator3->setStyleSheet(QString("QLabel {border: none;background-image: url(%1);"
                                                "background-position: center; background-repeat: no-repeat}").arg(strLineSeparator));

    ui->btn_changeToLogin->setStyleSheet(QString("QPushButton { border: 1px; background: none; "
                                                 "color: #43a6e8; font: 15px; padding-left: 10px; padding-bottom: 3px}"));
    ui->btn_changeToLogin->setVisible(false);
    ui->label_passwordError->setStyleSheet(QString("QLabel {border: none; padding-left: 25px; color: red;}"));

    m_menu->setFixedWidth(302);
    m_menu->setStyleSheet("QMenu {background-color: white; border-style: solid; border-color: #43A6E8; border-width: 1px; font: 16px; color: #5F5F5F; menu-scrollable: 1;}"
                          "QMenu::item {padding: 10px 0px 10px 40px; }"
                          "QMenu::item:selected {background-color: #E7F5FF; }");
}

bool CWizLoginWidget::checkSingMessage()
{
    QRegExp mailRex("\\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\\.[A-Z]{2,4}\\b");
    mailRex.setCaseSensitivity(Qt::CaseInsensitive);
    if (!mailRex.exactMatch(ui->lineEdit_newUserName->text()))
    {
        ui->lineEdit_newUserName->setErrorStatus(true);
        ui->lineEdit_newUserName->update();
        ui->label_passwordError->setText(tr("Invalid email address."));
        return false;
    }

    if (ui->lineEdit_newPassword->text().isEmpty())
    {
        ui->lineEdit_newPassword->setErrorStatus(true);
        ui->lineEdit_newPassword->update();
        ui->label_passwordError->setText(tr("Password is Empty"));
        return false;
    }

    if (ui->lineEdit_newPassword->text() != ui->lineEdit_repaetPassword->text())
    {
        ui->lineEdit_repaetPassword->setErrorStatus(true);
        ui->lineEdit_repaetPassword->update();
        ui->label_passwordError->setText(tr("Passwords don't match"));
        return false;
    }

    return true;
}




void CWizLoginWidget::on_btn_changeToSignin_clicked()
{
    ui->btn_changeToSignin->setVisible(false);
    ui->btn_changeToLogin->setVisible(true);
    ui->label_noaccount->setText(tr("Already got account,"));
    ui->label_passwordError->clear();
    ui->stackedWidget->setCurrentIndex(1);
}

void CWizLoginWidget::on_btn_changeToLogin_clicked()
{
    ui->btn_changeToLogin->setVisible(false);
    ui->btn_changeToSignin->setVisible(true);
    ui->label_noaccount->setText(tr("No account,"));
    ui->label_passwordError->clear();
    ui->stackedWidget->setCurrentIndex(0);
}

void CWizLoginWidget::on_btn_thridpart_clicked()
{
    QString strUrl = WizService::ApiEntry::standardCommandUrl("snspage");
    QDesktopServices::openUrl(QUrl(strUrl));
}

void CWizLoginWidget::on_btn_fogetpass_clicked()
{
    QString strUrl = WizService::ApiEntry::standardCommandUrl("forgot_password");
    QDesktopServices::openUrl(QUrl(strUrl));
}


LoginMenuLineEdit::LoginMenuLineEdit(QWidget *parent) : LoginLineEdit(parent)
{

}


void LoginMenuLineEdit::paintEvent(QPaintEvent *event)
{
    QLineEdit::paintEvent(event);

    QStyleOptionFrameV3 option;
    initStyleOption(&option);

    QPainter painter(this);
    QPixmap downArrow(::WizGetSkinResourceFileName(Utils::StyleHelper::themeName(), "loginLineEditorDownArrow"));
    QRect rect(option.rect.right() - downArrow.width() - 5, option.rect.top() + (option.rect.height() - downArrow.height()) / 2,
               downArrow.width(), downArrow.height());
    painter.drawPixmap(rect, downArrow);
}

void LoginMenuLineEdit::mousePressEvent(QMouseEvent *event)
{
    QStyleOptionFrameV3 option;
    initStyleOption(&option);
    QPixmap downArrow(::WizGetSkinResourceFileName(Utils::StyleHelper::themeName(), "loginLineEditorDownArrow"));
    QRect rect(option.rect.right() - downArrow.width() - 5, option.rect.top() + (option.rect.height() - downArrow.height()) / 2,
               downArrow.width(), downArrow.height());

    if (rect.contains(event->pos()))
    {
        emit showMenuRequest(mapToGlobal(QPoint(0, option.rect.height())));
    }
}

void CWizLoginWidget::on_btn_login_clicked()
{
    if (userId().isEmpty()) {
        ui->label_passwordError->setText(tr("Please enter user id"));
        return;
    }

    if (password().isEmpty()) {
        ui->label_passwordError->setText(tr("Please enter user password"));
        return;
    }

    enableLoginControls(false);
    doAccountVerify();
}

void CWizLoginWidget::on_btn_singin_clicked()
{
    if (checkSingMessage())
    {

    #if defined Q_OS_MAC
        QString strCode = "129ce11c";
    #elif defined Q_OS_LINUX
        QString strCode = "7abd8f4a";
    #else
        QString strCode = "8480c6d7";
    #endif

        AsyncApi* api = new AsyncApi(this);
        connect(api, SIGNAL(registerAccountFinished(bool)), SLOT(onRegisterAccountFinished(bool)));
        api->registerAccount(ui->lineEdit_newUserName->text(), ui->lineEdit_newPassword->text(), strCode);
        enableLoginControls(false);
    }
}

void CWizLoginWidget::onTokenAcquired(const QString &strToken)
{
    Token::instance()->disconnect(this);

    enableLoginControls(true);
    if (strToken.isEmpty())
    {
        int nErrorCode = Token::lastErrorCode();
        // network unavailable
        if (QNetworkReply::ProtocolUnknownError == nErrorCode)
        {
            CWizUserSettings userSettings(userId());
            if (password() != userSettings.password())
            {
                ui->label_passwordError->setText(tr("Connection is not available, please check your network connection."));
                return;
            }
            else
            {
                // login use local data
                QDialog::accept();
                return;
            }
        }
        else
        {
            //QMessageBox::critical(0, tr("Verify account failed"), );
            ui->label_passwordError->setText(Token::lastErrorMessage());
            return;
        }
    }

    if (updateUserProfile(true) && updateGlobalProfile())
        QDialog::accept();
}

void CWizLoginWidget::inputDataChanged()
{
    ui->label_passwordError->clear();
}

void CWizLoginWidget::userListMenuClicked(QAction *action)
{
    if (action)
    {
        setUser(action->text());
    }
}

void CWizLoginWidget::showUserListMenu(QPoint point)
{
    m_menu->popup(point);
}

void CWizLoginWidget::onRegisterAccountFinished(bool bFinish)
{
    AsyncApi* api = dynamic_cast<AsyncApi*>(sender());
    enableSignInControls(true);
    if (bFinish) {
        ui->stackedWidget->setCurrentIndex(0);
        enableLoginControls(false);
        ui->lineEdit_userName->setText(ui->lineEdit_newUserName->text());
        ui->lineEdit_password->setText(ui->lineEdit_newPassword->text());
        doAccountVerify();
    } else {
        ui->label_passwordError->setText(api->lastErrorMessage());
    }

    api->deleteLater();
}

void CWizLoginWidget::on_cbx_remberPassword_toggled(bool checked)
{
    if (!checked)
        ui->cbx_autologin->setChecked(false);
}

void CWizLoginWidget::on_cbx_autologin_toggled(bool checked)
{
    if (checked)
        ui->cbx_remberPassword->setChecked(true);
}

void CWizLoginWidget::on_lineEdit_userName_textEdited(const QString &arg1)
{
    Q_UNUSED(arg1);
    ui->lineEdit_password->setText("");
}