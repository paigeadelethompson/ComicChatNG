#include "mainwindow.h"
#include "connectdialog.h"
#include "icons.h"
#include "roomwidget.h"
#include "setupdialog.h"

#include <QAction>
#include <QActionGroup>
#include <QClipboard>
#include <QCloseEvent>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("Chat - ComicChatNG"));
    setWindowIcon(icons::window());
    resize(1024, 720);

    m_settings.load();
    m_art.setArtDirectory(m_settings.artDirectory);
    if (!m_art.scan()) {
        statusBar()->showMessage(tr("Warning: no comic art found in %1").arg(m_settings.artDirectory));
    }

    m_tabs = new QTabWidget;
    m_tabs->setTabsClosable(true);
    connect(m_tabs, &QTabWidget::tabCloseRequested, this, [this](int index) {
        QWidget *w = m_tabs->widget(index);
        if (auto *room = qobject_cast<RoomWidget *>(w)) {
            m_rooms.remove(room->channel().toLower());
            if (m_irc.isConnected())
                m_irc.partChannel(room->channel());
        }
        m_tabs->removeTab(index);
        w->deleteLater();
    });
    setCentralWidget(m_tabs);

    m_status = new QLabel(tr("Not connected"));
    statusBar()->addWidget(m_status, 1);

    menuBar()->addMenu(buildFileMenu());
    menuBar()->addMenu(buildEditMenu());
    menuBar()->addMenu(buildViewMenu());
    menuBar()->addMenu(buildFormatMenu());
    menuBar()->addMenu(buildRoomMenu());
    menuBar()->addMenu(buildMemberMenu());
    menuBar()->addMenu(buildFavoritesMenu());
    menuBar()->addMenu(buildWindowMenu());
    menuBar()->addMenu(buildHelpMenu());

    QToolBar *mainBar = buildMainToolbar();
    QToolBar *textBar = buildTextToolbar();
    QToolBar *userBar = buildUserToolbar();
    addToolBar(Qt::TopToolBarArea, mainBar);
    addToolBar(Qt::TopToolBarArea, textBar);
    addToolBar(Qt::TopToolBarArea, userBar);
    mainBar->setObjectName(QStringLiteral("MainToolbar"));
    textBar->setObjectName(QStringLiteral("TextToolbar"));
    userBar->setObjectName(QStringLiteral("MemberToolbar"));

    connect(&m_irc, &IrcClient::connected, this, &MainWindow::onConnected);
    connect(&m_irc, &IrcClient::disconnected, this, &MainWindow::onDisconnected);
    connect(&m_irc, &IrcClient::connectionError, this, &MainWindow::onConnectionError);
    connect(&m_irc, &IrcClient::channelJoined, this, &MainWindow::onChannelJoined);
    connect(&m_irc, &IrcClient::channelParted, this, &MainWindow::onChannelParted);
    connect(&m_irc, &IrcClient::privmsg, this, &MainWindow::routePrivmsg);
    connect(&m_irc, &IrcClient::action, this, &MainWindow::routeAction);
    connect(&m_irc, &IrcClient::userJoined, this, &MainWindow::routeUserJoined);
    connect(&m_irc, &IrcClient::userParted, this, &MainWindow::routeUserParted);
    connect(&m_irc, &IrcClient::userQuit, this, &MainWindow::routeUserQuit);
    connect(&m_irc, &IrcClient::nickChanged, this, &MainWindow::routeNickChanged);
    connect(&m_irc, &IrcClient::namesList, this, &MainWindow::routeNames);
    connect(&m_irc, &IrcClient::appearsAs, this, &MainWindow::routeAppearsAs);
    connect(&m_irc, &IrcClient::backdropAnnounce, this, &MainWindow::routeBackdrop);
    connect(&m_irc, &IrcClient::serverMessage, this, &MainWindow::routeServerMessage);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_settings.save();
    m_irc.disconnectFromHost();
    QMainWindow::closeEvent(event);
}

QMenu *MainWindow::buildFileMenu()
{
    QMenu *menu = new QMenu(tr("&File"), this);
    menu->addAction(tr("&New Connection...\tCtrl+N"), this, &MainWindow::sessionConnect);
    menu->addAction(tr("&Open...\tCtrl+O"), this, &MainWindow::notImplemented);
    menu->addAction(tr("&Close"), this, &MainWindow::leaveRoom);
    menu->addSeparator();
    menu->addAction(tr("&Save\tCtrl+S"), this, &MainWindow::notImplemented);
    menu->addAction(tr("Save &As..."), this, &MainWindow::notImplemented);
    menu->addSeparator();
    menu->addAction(tr("E&xit"), this, &QWidget::close);
    return menu;
}

QMenu *MainWindow::buildEditMenu()
{
    QMenu *menu = new QMenu(tr("&Edit"), this);
    menu->addAction(tr("&Undo\tCtrl+Z"), this, &MainWindow::notImplemented);
    menu->addSeparator();
    menu->addAction(tr("Cu&t\tCtrl+X"), this, &MainWindow::notImplemented);
    menu->addAction(tr("&Copy\tCtrl+C"), this, &MainWindow::notImplemented);
    menu->addAction(tr("&Paste\tCtrl+V"), this, &MainWindow::notImplemented);
    menu->addAction(tr("C&lear\tDel"), this, &MainWindow::notImplemented);
    menu->addSeparator();
    menu->addAction(tr("Select &All\tCtrl+A"), this, &MainWindow::notImplemented);
    menu->addAction(tr("Clear &History"), this, &MainWindow::clearHistory);
    return menu;
}

QMenu *MainWindow::buildViewMenu()
{
    QMenu *menu = new QMenu(tr("&View"), this);

    menu->addAction(tr("&Toolbar\tMain"), this, &MainWindow::notImplemented);

    menu->addSeparator();

    m_actComics = menu->addAction(tr("Comic Stri&p"));
    m_actComics->setCheckable(true);
    m_actComics->setChecked(m_settings.comicView);
    connect(m_actComics, &QAction::toggled, this, &MainWindow::toggleComicView);

    m_actText = menu->addAction(tr("Plain Te&xt"));
    m_actText->setCheckable(true);
    m_actText->setChecked(!m_settings.comicView);
    connect(m_actText, &QAction::toggled, this, &MainWindow::toggleTextView);

    menu->addSeparator();
    menu->addAction(tr("&Status Bar"), this, &MainWindow::notImplemented);
    menu->addAction(tr("Message of the &Day"), this, &MainWindow::notImplemented);
    menu->addSeparator();
    menu->addAction(tr("&Options...\tCtrl+Q"), this, &MainWindow::openSettings);
    return menu;
}

QMenu *MainWindow::buildFormatMenu()
{
    QMenu *menu = new QMenu(tr("F&ormat"), this);
    menu->addAction(tr("&Color...\tCtrl+K"), this, &MainWindow::notImplemented);
    menu->addAction(tr("&Bold\tCtrl+B"), this, &MainWindow::notImplemented);
    menu->addAction(tr("&Italic\tCtrl+I"), this, &MainWindow::notImplemented);
    menu->addAction(tr("&Underline\tCtrl+U"), this, &MainWindow::notImplemented);
    menu->addAction(tr("&Fixed Pitch Font\tCtrl+F"), this, &MainWindow::notImplemented);
    menu->addAction(tr("&Symbol\tCtrl+D"), this, &MainWindow::notImplemented);
    return menu;
}

QMenu *MainWindow::buildRoomMenu()
{
    QMenu *menu = new QMenu(tr("&Room"), this);
    m_actEnterRoom = menu->addAction(tr("&Enter Room..."));
    m_actEnterRoom->setIcon(icons::main(2));
    connect(m_actEnterRoom, &QAction::triggered, this, &MainWindow::enterRoom);

    m_actLeaveRoom = menu->addAction(tr("&Leave Room"));
    m_actLeaveRoom->setIcon(icons::main(3));
    connect(m_actLeaveRoom, &QAction::triggered, this, &MainWindow::leaveRoom);

    menu->addAction(tr("Crea&te Room..."), this, &MainWindow::createRoom);
    menu->addSeparator();
    QAction *list = menu->addAction(tr("&Room List..."));
    list->setIcon(icons::main(7));
    connect(list, &QAction::triggered, this, &MainWindow::showRoomList);
    menu->addSeparator();
    menu->addAction(tr("&Connect..."), this, &MainWindow::sessionConnect);
    menu->addAction(tr("&Disconnect"), this, &MainWindow::sessionDisconnect);
    return menu;
}

QMenu *MainWindow::buildMemberMenu()
{
    QMenu *menu = new QMenu(tr("&Member"), this);
    QAction *ul = menu->addAction(tr("&User List..."));
    ul->setIcon(icons::main(8));
    connect(ul, &QAction::triggered, this, &MainWindow::notImplemented);
    menu->addAction(tr("&Invite..."), this, &MainWindow::notImplemented);
    menu->addAction(tr("&Away from Keyboard..."), this, &MainWindow::notImplemented);
    menu->addSeparator();
    menu->addAction(tr("&Get Profile"), this, &MainWindow::notImplemented);
    menu->addAction(tr("Get &Identity"), this, &MainWindow::notImplemented);
    menu->addAction(tr("&Whisper Box..."), this, &MainWindow::notImplemented);
    menu->addAction(tr("Add to Notifi&cations..."), this, &MainWindow::notImplemented);
    menu->addAction(tr("&Ignore"), this, &MainWindow::notImplemented);
    menu->addSeparator();
    menu->addAction(tr("Send &E-mail"), this, &MainWindow::notImplemented);
    menu->addAction(tr("Send &File..."), this, &MainWindow::notImplemented);
    menu->addAction(tr("Visit H&ome Page"), this, &MainWindow::notImplemented);
    menu->addAction(tr("Net&Meeting"), this, &MainWindow::notImplemented);
    menu->addSeparator();
    menu->addAction(tr("&Version"), this, &MainWindow::notImplemented);
    menu->addAction(tr("Lag &Time"), this, &MainWindow::notImplemented);
    menu->addAction(tr("&Local Time"), this, &MainWindow::notImplemented);
    return menu;
}

QMenu *MainWindow::buildFavoritesMenu()
{
    QMenu *menu = new QMenu(tr("F&avorites"), this);
    menu->addAction(tr("&Add to Favorites"), this, &MainWindow::notImplemented);
    QAction *open = menu->addAction(tr("&Open Favorites..."));
    open->setIcon(icons::main(9));
    connect(open, &QAction::triggered, this, &MainWindow::notImplemented);
    return menu;
}

QMenu *MainWindow::buildWindowMenu()
{
    QMenu *menu = new QMenu(tr("&Window"), this);
    menu->addAction(tr("&Cascade"), this, &MainWindow::notImplemented);
    menu->addAction(tr("&Tile Horizontally"), this, &MainWindow::notImplemented);
    menu->addAction(tr("Tile &Vertically"), this, &MainWindow::notImplemented);
    menu->addAction(tr("&Arrange Icons"), this, &MainWindow::notImplemented);
    return menu;
}

QMenu *MainWindow::buildHelpMenu()
{
    QMenu *menu = new QMenu(tr("&Help"), this);
    menu->addAction(tr("&Help Topics"), this, &MainWindow::notImplemented);
    menu->addSeparator();
    menu->addAction(tr("&About Chat"), this, &MainWindow::about);
    return menu;
}

QToolBar *MainWindow::buildMainToolbar()
{
    QToolBar *bar = new QToolBar(tr("Main"), this);
    bar->setMovable(false);
    bar->setIconSize(QSize(16, 16));

    bar->addAction(icons::main(0), tr("Connect"), this, &MainWindow::sessionConnect)->setToolTip(tr("Connect"));
    bar->addAction(icons::main(1), tr("Disconnect"), this, &MainWindow::sessionDisconnect);
    bar->addAction(icons::main(2), tr("Enter Room"), this, &MainWindow::enterRoom);
    bar->addAction(icons::main(3), tr("Leave Room"), this, &MainWindow::leaveRoom);
    bar->addAction(icons::main(4), tr("Create Room"), this, &MainWindow::createRoom);
    bar->addSeparator();

    m_actComics = bar->addAction(icons::main(5), tr("Comic Strip"));
    m_actComics->setCheckable(true);
    m_actComics->setChecked(m_settings.comicView);
    connect(m_actComics, &QAction::toggled, this, &MainWindow::toggleComicView);

    m_actText = bar->addAction(icons::main(6), tr("Plain Text"));
    m_actText->setCheckable(true);
    m_actText->setChecked(!m_settings.comicView);
    connect(m_actText, &QAction::toggled, this, &MainWindow::toggleTextView);

    bar->addSeparator();
    bar->addAction(icons::main(7), tr("Room List"), this, &MainWindow::showRoomList);
    bar->addAction(icons::main(8), tr("User List"), this, &MainWindow::notUserList);
    bar->addSeparator();
    bar->addAction(icons::main(9), tr("Open Favorites..."), this, &MainWindow::notImplemented);
    return bar;
}

QToolBar *MainWindow::buildTextToolbar()
{
    QToolBar *bar = new QToolBar(tr("Text"), this);
    bar->setMovable(false);
    bar->setIconSize(QSize(16, 16));
    bar->addAction(icons::text(0), tr("Set Font..."), this, &MainWindow::notImplemented);
    bar->addAction(icons::text(1), tr("Set Color..."), this, &MainWindow::notImplemented);

    addToggleTextAction(bar, icons::text(2), tr("Bold"));
    addToggleTextAction(bar, icons::text(3), tr("Italic"));
    addToggleTextAction(bar, icons::text(4), tr("Underline"));
    addToggleTextAction(bar, icons::text(5), tr("Fixed Pitch"));
    addToggleTextAction(bar, icons::text(6), tr("Symbol"));
    return bar;
}

void MainWindow::addToggleTextAction(QToolBar *bar, const QIcon &icon, const QString &text)
{
    QAction *a = bar->addAction(icon, text);
    a->setCheckable(true);
    connect(a, &QAction::toggled, this, &MainWindow::notImplemented);
}

QToolBar *MainWindow::buildUserToolbar()
{
    QToolBar *bar = new QToolBar(tr("Member"), this);
    bar->setMovable(false);
    bar->setIconSize(QSize(16, 16));
    QAction *away = bar->addAction(icons::user(0), tr("Away from Keyboard"));
    away->setCheckable(true);
    connect(away, &QAction::toggled, this, &MainWindow::notImplemented);
    bar->addAction(icons::user(1), tr("Get Identity"), this, &MainWindow::notImplemented);
    bar->addAction(icons::user(2), tr("Ignore"), this, &MainWindow::notImplemented);
    bar->addAction(icons::user(3), tr("Whisper Box"), this, &MainWindow::notImplemented);
    bar->addSeparator();
    bar->addAction(icons::user(4), tr("Send E-mail"), this, &MainWindow::notImplemented);
    bar->addAction(icons::user(5), tr("Visit Home Page"), this, &MainWindow::notImplemented);
    bar->addAction(icons::user(6), tr("NetMeeting"), this, &MainWindow::notImplemented);
    return bar;
}

void MainWindow::notImplemented()
{
    statusBar()->showMessage(tr("This feature is not wired up yet"), 4000);
}

void MainWindow::notUserList()
{
    statusBar()->showMessage(tr("User List"), 4000);
}

void MainWindow::clearHistory()
{
    if (RoomWidget *r = currentRoom())
        emit r->clearHistoryRequested();
}

void MainWindow::enterRoom()
{
    ConnectDialog dlg(m_settings, m_art.avatarNames(), this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    const AppSettings r = dlg.resultSettings();
    m_settings.port = r.port;
    m_settings.nick = r.nick;
    m_settings.avatarName = r.avatarName;
    m_settings.save();

    m_status->setText(tr("Connecting to %1…").arg(m_settings.server));
    m_irc.connectToHost(m_settings.server, m_settings.port,
                        m_settings.nick, m_settings.user, m_settings.realName);
    m_irc.joinChannel(r.channel);
}

void MainWindow::leaveRoom()
{
    if (RoomWidget *r = currentRoom()) {
        m_rooms.remove(r->channel().toLower());
        if (m_irc.isConnected())
            m_irc.partChannel(r->channel());
        int idx = m_tabs->indexOf(r);
        m_tabs->removeTab(idx);
        delete r;
    }
}

void MainWindow::createRoom()
{
    enterRoom();
}

void MainWindow::showRoomList()
{
    QMessageBox::information(this, tr("Room List"),
                             tr("The list of open chat rooms.\n"
                                "Use “Enter Room…” to join one."));
}

void MainWindow::sessionConnect()
{
    ConnectDialog dlg(m_settings, m_art.avatarNames(), this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    const AppSettings r = dlg.resultSettings();
    m_settings.server = r.server;
    m_settings.port = r.port;
    m_settings.channel = r.channel;
    m_settings.nick = r.nick;
    m_settings.avatarName = r.avatarName;
    m_settings.save();

    m_status->setText(tr("Connecting to %1…").arg(m_settings.server));
    m_irc.connectToHost(m_settings.server, m_settings.port,
                        m_settings.nick, m_settings.user, m_settings.realName);
    m_irc.joinChannel(m_settings.channel);
}

void MainWindow::sessionDisconnect()
{
    m_irc.disconnectFromHost();
}

void MainWindow::openSettings()
{
    SetupDialog dlg(&m_settings, &m_art, this);
    if (dlg.exec() == QDialog::Accepted) {
        for (auto it = m_rooms.begin(); it != m_rooms.end(); ++it) {
            it.value()->applySettings();
            it.value()->setComicMode(m_settings.comicView);
        }
    }
}

void MainWindow::toggleComicView(bool on)
{
    m_settings.comicView = on;
    m_settings.save();
    if (m_actText)
        m_actText->setChecked(!on);
    for (auto it = m_rooms.begin(); it != m_rooms.end(); ++it)
        it.value()->setComicMode(on);
}

void MainWindow::toggleTextView(bool on)
{
    toggleComicView(!on);
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Chat"),
                       tr("<h3>Chat: Microsoft Chat 2.5</h3>"
                          "<p>Qt6 reimplementation of Microsoft Comic Chat 2.5.</p>"
                          "<p>Loads the original <code>.avb</code> / <code>.bgb</code> art.</p>"));
}

void MainWindow::onConnected()
{
    m_status->setText(tr("Connected — registering…"));
}

void MainWindow::onDisconnected()
{
    m_status->setText(tr("Disconnected"));
}

void MainWindow::onConnectionError(const QString &error)
{
    m_status->setText(error);
    statusBar()->showMessage(error, 8000);
}

void MainWindow::onChannelJoined(const QString &channel)
{
    RoomWidget *room = roomForChannel(channel, true);
    m_tabs->setCurrentWidget(room);
    m_status->setText(tr("Joined %1").arg(channel));
    m_irc.announceAppearance(channel, m_settings.avatarName);
    m_irc.announceBackdrop(channel, m_settings.backdropName);
}

void MainWindow::onChannelParted(const QString &channel)
{
    m_status->setText(tr("Left %1").arg(channel));
}

RoomWidget *MainWindow::roomForChannel(const QString &channel, bool create)
{
    const QString key = channel.toLower();
    if (m_rooms.contains(key))
        return m_rooms.value(key);
    if (!create)
        return nullptr;
    auto *room = new RoomWidget(channel, &m_art, &m_settings, &m_irc);
    m_rooms.insert(key, room);
    m_tabs->addTab(room, icons::room(), channel);
    return room;
}

RoomWidget *MainWindow::currentRoom() const
{
    return qobject_cast<RoomWidget *>(m_tabs->currentWidget());
}

void MainWindow::routePrivmsg(const QString &channel, const QString &nick, const QString &text)
{
    if (RoomWidget *r = roomForChannel(channel, true))
        r->onPrivmsg(channel, nick, text);
}

void MainWindow::routeAction(const QString &channel, const QString &nick, const QString &text)
{
    if (RoomWidget *r = roomForChannel(channel, true))
        r->onAction(channel, nick, text);
}

void MainWindow::routeUserJoined(const QString &channel, const QString &nick)
{
    if (RoomWidget *r = roomForChannel(channel, true))
        r->onUserJoined(channel, nick);
}

void MainWindow::routeUserParted(const QString &channel, const QString &nick, const QString &reason)
{
    if (RoomWidget *r = roomForChannel(channel, false))
        r->onUserParted(channel, nick, reason);
}

void MainWindow::routeUserQuit(const QString &nick, const QString &reason)
{
    for (RoomWidget *r : m_rooms)
        r->onUserQuit(nick, reason);
}

void MainWindow::routeNickChanged(const QString &oldNick, const QString &newNick)
{
    for (RoomWidget *r : m_rooms)
        r->onNickChanged(oldNick, newNick);
}

void MainWindow::routeNames(const QString &channel, const QStringList &nicks)
{
    if (RoomWidget *r = roomForChannel(channel, true))
        r->onNamesList(channel, nicks);
}

void MainWindow::routeAppearsAs(const QString &nick, const QString &avatar)
{
    for (RoomWidget *r : m_rooms)
        r->onAppearsAs(nick, avatar);
}

void MainWindow::routeBackdrop(const QString &nick, const QString &backdrop)
{
    for (RoomWidget *r : m_rooms)
        r->onBackdropAnnounce(nick, backdrop);
}

void MainWindow::routeServerMessage(const QString &text)
{
    if (RoomWidget *r = currentRoom())
        r->onServerMessage(text);
}