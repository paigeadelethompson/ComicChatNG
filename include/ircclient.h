#pragma once

#include "emotions.h"

#include <QHash>
#include <QObject>
#include <QString>
#include <QTcpSocket>

struct ChatUser {
  QString nick;
  QString avatarName;
  QString backdropName;
  bool isSelf = false;
};

// Which of the classic Say / Think / Whisper / Action balloons a comic message
// is sent as. Encoded as the Comic Chat "M" annotation byte on the wire.
enum class ComicMode {
  Say = 1,
  Whisper = 2,
  Think = 3,
  Action = 5,
};

class IrcClient : public QObject {
  Q_OBJECT
public:
  explicit IrcClient(QObject *parent = nullptr);

  void connectToHost(const QString &host, quint16 port, const QString &nick,
                     const QString &user, const QString &realName);
  void disconnectFromHost();
  bool isConnected() const;

  void joinChannel(const QString &channel);
  void partChannel(const QString &channel, const QString &msg = {});
  void sendPrivmsg(const QString &target, const QString &text);
  void sendAction(const QString &target, const QString &text);
  void sendRaw(const QString &line);
  void setNick(const QString &nick);

  QString nick() const { return m_nick; }
  QString currentChannel() const { return m_channel; }

  // Comic Chat CTCP helpers
  void announceAppearance(const QString &target, const QString &avatarName);
  void announceBackdrop(const QString &channel, const QString &backdropName);

  // Body state used to build message annotations on the wire.
  void setSelfAvatar(const QString &name) { m_selfAvatar = name; }
  void setSelfEmotion(const Emotion &e) { m_selfEmotion = e; }
  void setSelfProfile(const QString &profile) { m_selfProfile = profile; }

  // Send a comic-view message with the full Comic Chat annotation header
  // ("(#G…E…M…) "), so other Comic Chat clients emote our avatar and see the
  // right balloon kind. `talkTos` are the addresses of who we're talking to
  // (embedded as the trailing "T<nick>,<nick>" byte list).
  void sendComicMessage(const QString &target, const QString &text,
                        ComicMode mode,
                        const QStringList &talkTos = {});

  // Comic Chat profile / avatar info exchange (plain IRC).
  void requestProfile(const QString &nick);   // "# GetInfo"
  void sendProfile(const QString &nick, const QString &profile);
  void requestAvatarInfo(const QString &nick); // "# GetCharInfo"
  void appearAs(const QString &target, const QString &avatarName,
                const QString &url = {});
  void requestIdentity(const QString &nick); // IRC WHOIS

signals:
  void connected();
  void disconnected();
  void connectionError(const QString &error);
  void serverMessage(const QString &text);
  void channelJoined(const QString &channel);
  void channelParted(const QString &channel);
  void userJoined(const QString &channel, const QString &nick);
  void userParted(const QString &channel, const QString &nick,
                  const QString &reason);
  void userQuit(const QString &nick, const QString &reason);
  void nickChanged(const QString &oldNick, const QString &newNick);
  void topicChanged(const QString &channel, const QString &topic);
  void namesList(const QString &channel, const QStringList &nicks);
  void privmsg(const QString &channel, const QString &nick,
               const QString &text);
  void action(const QString &channel, const QString &nick, const QString &text);
  void think(const QString &channel, const QString &nick, const QString &text);
  void whisper(const QString &recipient, const QString &nick,
               const QString &text);
  void notice(const QString &nick, const QString &text);
  void appearsAs(const QString &nick, const QString &avatarName);
  void backdropAnnounce(const QString &nick, const QString &backdropName);
  void heresInfo(const QString &nick, const QString &info);
  void identityInfo(const QString &nick, const QString &realName);
  // Emotion carried by the "E" annotation bytes of an incoming message.
  void messageEmotion(const QString &nick, const Emotion &emotion);
  // Who the peer addressed, carried by the "T" annotation bytes.
  void talkTo(const QString &nick, const QStringList &targets);

private slots:
  void onConnected();
  void onDisconnected();
  void onReadyRead();
  void onError(QAbstractSocket::SocketError error);

private:
  void processLine(const QString &line);
  void handleNumeric(int code, const QString &prefix, const QStringList &args,
                     const QString &trailing);
  void handlePrivmsg(const QString &nick, const QString &target,
                     const QString &text);
  static QString parseNick(const QString &prefix);
  // Strip "(#G…E…M…) " from the front of an incoming comic message. Fills
  // `faceEmotion` (from the E bytes) and `mode` (ComicMode value, 0 = none).
  static bool stripAnnotations(QString &text, Emotion *faceEmotion, int *mode,
                               QStringList *talkTos = nullptr);
  // "#G<gestIdx><gestEmo><gestInt>E<faceIdx><faceEmo><faceInt>M<mode>[T<a>,<b>]"
  // in parentheses, the header Comic Chat prepends to every comic message.
  QString buildAnnotations(ComicMode mode,
                           const QStringList &talkTos = {}) const;
  void writeLine(const QString &line);

  QTcpSocket m_socket;
  QByteArray m_buffer;
  QString m_nick;
  QString m_user;
  QString m_realName;
  QString m_channel;
  QString m_pendingChannel;
  bool m_registered = false;
  QHash<QString, QStringList> m_namesAccum;
  QString m_selfAvatar;
  QString m_selfProfile;
  Emotion m_selfEmotion;
};
