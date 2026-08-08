#include "mainwindow.h"
#include "connectdialog.h"
#include "roomwidget.h"
#include "setupdialog.h"

#include <QAction>
#include <QCloseEvent>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("ComicChatNG"));
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

    auto *fileMenu = menuBar()->addMenu(tr("&Session"));
    fileMenu->addAction(tr("&Connect…"), this, &MainWindow::sessionConnect);
    fileMenu->addAction(tr("&Disconnect"), this, &MainWindow::sessionDisconnect);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("E&xit"), this, &QWidget::close);

    auto *viewMenu = menuBar()->addMenu(tr("&View"));
    auto *comicAct = viewMenu->addAction(tr("&Comic View"));
    comicAct->setCheckable(true);
    comicAct->setChecked(m_settings.comicView);
    connect(comicAct, &QAction::toggled, this, &MainWindow::toggleComicView);

    auto *toolsMenu = menuBar()->addMenu(tr("&Tools"));
    toolsMenu->addAction(tr("&Settings…"), this, &MainWindow::openSettings);

    auto *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&About ComicChatNG"), this, &MainWindow::about);

    auto *tb = addToolBar(tr("Main"));
    tb->addAction(tr("Connect"), this, &MainWindow::sessionConnect);
    tb->addAction(tr("Disconnect"), this, &MainWindow::sessionDisconnect);
    tb->addAction(tr("Settings"), this, &MainWindow::openSettings);

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
        for (auto it = m_rooms.begin(); it != m_rooms.end(); ++it)
            it.value()->setComicMode(m_settings.comicView);
    }
}

void MainWindow::toggleComicView(bool on)
{
    m_settings.comicView = on;
    m_settings.save();
    for (auto it = m_rooms.begin(); it != m_rooms.end(); ++it)
        it.value()->setComicMode(on);
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About ComicChatNG"),
                       tr("<h3>ComicChatNG</h3>"
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
    m_tabs->addTab(room, channel);
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
