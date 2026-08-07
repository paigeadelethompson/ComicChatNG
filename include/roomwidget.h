#pragma once

#include "appsettings.h"
#include "ircclient.h"

#include <QHash>
#include <QWidget>

class ArtManager;
class PageView;
class QListWidget;
class QPlainTextEdit;
class QLineEdit;
class QSplitter;
class QLabel;

class RoomWidget : public QWidget
{
    Q_OBJECT
public:
    RoomWidget(const QString &channel, ArtManager *art, AppSettings *settings,
               IrcClient *irc, QWidget *parent = nullptr);

    QString channel() const { return m_channel; }
    void setComicMode(bool on);

public slots:
    void onPrivmsg(const QString &channel, const QString &nick, const QString &text);
    void onAction(const QString &channel, const QString &nick, const QString &text);
    void onUserJoined(const QString &channel, const QString &nick);
    void onUserParted(const QString &channel, const QString &nick, const QString &reason);
    void onUserQuit(const QString &nick, const QString &reason);
    void onNickChanged(const QString &oldNick, const QString &newNick);
    void onNamesList(const QString &channel, const QStringList &nicks);
    void onAppearsAs(const QString &nick, const QString &avatarName);
    void onBackdropAnnounce(const QString &nick, const QString &backdropName);
    void onServerMessage(const QString &text);

private slots:
    void sendSay();

private:
    void appendText(const QString &line);
    void addComicLine(const QString &nick, const QString &text, bool isAction);
    void ensureMember(const QString &nick);
    void removeMember(const QString &nick);
    QString avatarFor(const QString &nick) const;

    QString m_channel;
    ArtManager *m_art = nullptr;
    AppSettings *m_settings = nullptr;
    IrcClient *m_irc = nullptr;

    PageView *m_pageView = nullptr;
    QPlainTextEdit *m_textView = nullptr;
    QListWidget *m_members = nullptr;
    QLineEdit *m_sayEdit = nullptr;
    QLabel *m_preview = nullptr;
    QSplitter *m_mainSplit = nullptr;
    QWidget *m_comicPane = nullptr;

    QHash<QString, QString> m_userAvatars; // nick lower -> avatar file base
    QString m_roomBackdrop;
};
