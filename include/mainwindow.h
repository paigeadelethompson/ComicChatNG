#pragma once

#include "appsettings.h"
#include "artmanager.h"
#include "ircclient.h"

#include <QMainWindow>
#include <QHash>

class QTabWidget;
class RoomWidget;
class QLabel;
class QAction;
class QToolBar;
class QMenu;
class QLineEdit;
class QIcon;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void sessionConnect();
    void sessionDisconnect();
    void enterRoom();
    void leaveRoom();
    void createRoom();
    void showRoomList();
    void toggleComicView(bool on);
    void toggleTextView(bool on);
    void openSettings();
    void about();
    void clearHistory();

    void onConnected();
    void onDisconnected();
    void onConnectionError(const QString &error);
    void onChannelJoined(const QString &channel);
    void onChannelParted(const QString &channel);
    void routePrivmsg(const QString &channel, const QString &nick, const QString &text);
    void routeAction(const QString &channel, const QString &nick, const QString &text);
    void routeUserJoined(const QString &channel, const QString &nick);
    void routeUserParted(const QString &channel, const QString &nick, const QString &reason);
    void routeUserQuit(const QString &nick, const QString &reason);
    void routeNickChanged(const QString &oldNick, const QString &newNick);
    void routeNames(const QString &channel, const QStringList &nicks);
    void routeAppearsAs(const QString &nick, const QString &avatar);
    void routeBackdrop(const QString &nick, const QString &backdrop);
    void routeServerMessage(const QString &text);

private:
    RoomWidget *roomForChannel(const QString &channel, bool create);
    RoomWidget *currentRoom() const;
    QMenu *buildFileMenu();
    QMenu *buildEditMenu();
    QMenu *buildViewMenu();
    QMenu *buildFormatMenu();
    QMenu *buildRoomMenu();
    QMenu *buildMemberMenu();
    QMenu *buildFavoritesMenu();
    QMenu *buildWindowMenu();
    QMenu *buildHelpMenu();
    QToolBar *buildMainToolbar();
    QToolBar *buildTextToolbar();
    QToolBar *buildUserToolbar();
    void notImplemented();
    void notUserList();
    void addToggleTextAction(QToolBar *bar, const QIcon &icon, const QString &text);

    QAction *m_actComics = nullptr;
    QAction *m_actText = nullptr;
    QAction *m_actEnterRoom = nullptr;
    QAction *m_actLeaveRoom = nullptr;

    AppSettings m_settings;
    ArtManager m_art;
    IrcClient m_irc;
    QTabWidget *m_tabs = nullptr;
    QLabel *m_status = nullptr;
    QHash<QString, RoomWidget *> m_rooms;
};