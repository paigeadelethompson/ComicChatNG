#pragma once

#include "appsettings.h"
#include "ircclient.h"
#include "balloon.h"
#include "emotions.h"

#include <QHash>
#include <QStringList>
#include <QWidget>

class ArtManager;
class PageView;
class EmotionPicker;
class QListWidget;
class QPlainTextEdit;
class QLineEdit;
class QScrollArea;
class QSplitter;
class QLabel;
class QToolButton;

class RoomWidget : public QWidget
{
    Q_OBJECT
public:
    RoomWidget(const QString &channel, ArtManager *art, AppSettings *settings,
               IrcClient *irc, QWidget *parent = nullptr);

    QString channel() const { return m_channel; }
    void setComicMode(bool on);
    void applySettings();

signals:
    void clearHistoryRequested();

public slots:
    void onPrivmsg(const QString &channel, const QString &nick, const QString &text);
    void onAction(const QString &channel, const QString &nick, const QString &text);
    void onThink(const QString &channel, const QString &nick, const QString &text);
    void onWhisper(const QString &channel, const QString &nick, const QString &text);
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
    void onEmotionChanged(const Emotion &e);
    void showRoomMenu(const QPoint &gpos);
    void showMemberMenu(const QPoint &gpos);
    void showRoomProperties();
    void notImplemented();

private:
    void appendText(const QString &line);
    void addComicLine(const QString &nick, const QString &text, bool isAction);
    void addComicBalloon(const QString &nick, const QString &text, BalloonKind kind);
    void ensureMember(const QString &nick);
    void removeMember(const QString &nick);
    void refreshSelfPreview();
    void setSelfAvatar(const QString &name);
    QString avatarFor(const QString &nick) const;
    void applyMemberIcon(int row);
    QStringList panelForActors(const QString &speaker);

    QString m_channel;
    ArtManager *m_art = nullptr;
    AppSettings *m_settings = nullptr;
    IrcClient *m_irc = nullptr;

    PageView *m_pageView = nullptr;
    QPlainTextEdit *m_textView = nullptr;
    QListWidget *m_members = nullptr;
    QLineEdit *m_sayEdit = nullptr;
    QLabel *m_preview = nullptr;
    EmotionPicker *m_emotionPicker = nullptr;
    QScrollArea *m_comicScroll = nullptr;
    QSplitter *m_mainSplit = nullptr;

    QHash<QString, QString> m_userAvatars; // nick lower -> avatar file base
    QString m_roomBackdrop;
    QString m_previousSpeaker;
    Emotion m_selfEmotion;
    int m_sayMode = 0; // 0=Say, 1=Think, 2=Whisper, 3=Action
    QString m_whisperTarget;
    QString m_roomTopic;
};