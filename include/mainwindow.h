#pragma once

#include "appsettings.h"
#include "artmanager.h"
#include "ircclient.h"

#include <QMainWindow>
#include <QHash>

class QTabWidget;
class RoomWidget;
class QLabel;

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
    void openSettings();
    void toggleComicView(bool on);
    void about();

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
    void updateStatus();

    AppSettings m_settings;
    ArtManager m_art;
    IrcClient m_irc;
    QTabWidget *m_tabs = nullptr;
    QLabel *m_status = nullptr;
    QHash<QString, RoomWidget *> m_rooms;
};
