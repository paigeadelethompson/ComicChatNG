#pragma once

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
  void announceAppearance(const QString &channel, const QString &avatarName);
  void announceBackdrop(const QString &channel, const QString &backdropName);

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
  void notice(const QString &nick, const QString &text);
  void appearsAs(const QString &nick, const QString &avatarName);
  void backdropAnnounce(const QString &nick, const QString &backdropName);
  void heresInfo(const QString &nick, const QString &info);

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
};
