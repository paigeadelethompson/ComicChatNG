#include "ircclient.h"

#include <QRegularExpression>

IrcClient::IrcClient(QObject *parent) : QObject(parent) {
  connect(&m_socket, &QTcpSocket::connected, this, &IrcClient::onConnected);
  connect(&m_socket, &QTcpSocket::disconnected, this,
          &IrcClient::onDisconnected);
  connect(&m_socket, &QTcpSocket::readyRead, this, &IrcClient::onReadyRead);
  connect(&m_socket, &QAbstractSocket::errorOccurred, this,
          &IrcClient::onError);
}

void IrcClient::connectToHost(const QString &host, quint16 port,
                              const QString &nick, const QString &user,
                              const QString &realName) {
  m_nick = nick;
  m_user = user.isEmpty() ? nick : user;
  m_realName = realName.isEmpty() ? nick : realName;
  m_registered = false;
  m_channel.clear();
  m_buffer.clear();
  m_socket.connectToHost(host, port);
}

void IrcClient::disconnectFromHost() {
  if (m_socket.state() != QAbstractSocket::UnconnectedState) {
    writeLine(QStringLiteral("QUIT :ComicChatNG"));
    m_socket.disconnectFromHost();
  }
}

bool IrcClient::isConnected() const {
  return m_socket.state() == QAbstractSocket::ConnectedState && m_registered;
}

void IrcClient::joinChannel(const QString &channel) {
  QString ch = channel.trimmed();
  if (ch.isEmpty())
    return;
  if (!ch.startsWith(QLatin1Char('#')) && !ch.startsWith(QLatin1Char('&')))
    ch.prepend(QLatin1Char('#'));
  m_pendingChannel = ch;
  writeLine(QStringLiteral("JOIN %1").arg(ch));
}

void IrcClient::partChannel(const QString &channel, const QString &msg) {
  if (msg.isEmpty())
    writeLine(QStringLiteral("PART %1").arg(channel));
  else
    writeLine(QStringLiteral("PART %1 :%2").arg(channel, msg));
}

void IrcClient::sendPrivmsg(const QString &target, const QString &text) {
  writeLine(QStringLiteral("PRIVMSG %1 :%2").arg(target, text));
}

void IrcClient::sendAction(const QString &target, const QString &text) {
  writeLine(QStringLiteral("PRIVMSG %1 :\x01"
                           "ACTION %2\x01")
                .arg(target, text));
}

void IrcClient::sendRaw(const QString &line) { writeLine(line); }

void IrcClient::setNick(const QString &nick) {
  m_nick = nick;
  if (m_socket.state() == QAbstractSocket::ConnectedState)
    writeLine(QStringLiteral("NICK %1").arg(nick));
}

void IrcClient::announceAppearance(const QString &channel,
                                   const QString &avatarName) {
  // Comic Chat protocol: "# Appears as <name>"
  sendPrivmsg(channel, QStringLiteral("# Appears as %1").arg(avatarName));
}

void IrcClient::announceBackdrop(const QString &channel,
                                 const QString &backdropName) {
  if (backdropName.isEmpty())
    return;
  // New clients recognise "# BDrop2: <name>,<url>"; older ones read "# BDrop:
  // <name>".
  sendPrivmsg(channel, QStringLiteral("# BDrop2: %1,").arg(backdropName));
  sendPrivmsg(channel, QStringLiteral("# BDrop: %1").arg(backdropName));
}

void IrcClient::onConnected() {
  writeLine(QStringLiteral("NICK %1").arg(m_nick));
  writeLine(QStringLiteral("USER %1 0 * :%2").arg(m_user, m_realName));
  emit connected();
}

void IrcClient::onDisconnected() {
  m_registered = false;
  m_channel.clear();
  emit disconnected();
}

void IrcClient::onError(QAbstractSocket::SocketError) {
  emit connectionError(m_socket.errorString());
}

void IrcClient::onReadyRead() {
  m_buffer.append(m_socket.readAll());
  while (true) {
    int idx = m_buffer.indexOf("\r\n");
    int skip = 2;
    if (idx < 0) {
      idx = m_buffer.indexOf('\n');
      skip = 1;
    }
    if (idx < 0)
      break;
    QByteArray raw = m_buffer.left(idx);
    m_buffer.remove(0, idx + skip);
    processLine(QString::fromUtf8(raw));
  }
}

void IrcClient::writeLine(const QString &line) {
  if (m_socket.state() != QAbstractSocket::ConnectedState)
    return;
  QByteArray data = line.toUtf8();
  data.append("\r\n");
  m_socket.write(data);
}

QString IrcClient::parseNick(const QString &prefix) {
  const int bang = prefix.indexOf(QLatin1Char('!'));
  if (bang > 0)
    return prefix.left(bang);
  return prefix;
}

void IrcClient::processLine(const QString &line) {
  if (line.isEmpty())
    return;

  QString prefix;
  QString command;
  QStringList args;
  QString trailing;
  QString rest = line;

  if (rest.startsWith(QLatin1Char(':'))) {
    const int sp = rest.indexOf(QLatin1Char(' '));
    if (sp < 0)
      return;
    prefix = rest.mid(1, sp - 1);
    rest = rest.mid(sp + 1);
  }

  const int trail = rest.indexOf(QLatin1String(" :"));
  if (trail >= 0) {
    trailing = rest.mid(trail + 2);
    rest = rest.left(trail);
  }

  const QStringList parts = rest.split(QLatin1Char(' '), Qt::SkipEmptyParts);
  if (parts.isEmpty())
    return;
  command = parts.first().toUpper();
  for (int i = 1; i < parts.size(); ++i)
    args.append(parts.at(i));

  if (command == QLatin1String("PING")) {
    writeLine(QStringLiteral("PONG :%1")
                  .arg(trailing.isEmpty() ? (args.value(0)) : trailing));
    return;
  }

  bool okNum = false;
  const int code = command.toInt(&okNum);
  if (okNum) {
    handleNumeric(code, prefix, args, trailing);
    return;
  }

  const QString nick = parseNick(prefix);

  if (command == QLatin1String("PRIVMSG")) {
    const QString target = args.value(0);
    handlePrivmsg(nick, target, trailing);
  } else if (command == QLatin1String("NOTICE")) {
    emit notice(nick, trailing);
    emit serverMessage(QStringLiteral("-%1- %2").arg(nick, trailing));
  } else if (command == QLatin1String("JOIN")) {
    QString ch = trailing.isEmpty() ? args.value(0) : trailing;
    if (ch.startsWith(QLatin1Char(':')))
      ch = ch.mid(1);
    if (nick.compare(m_nick, Qt::CaseInsensitive) == 0) {
      m_channel = ch;
      emit channelJoined(ch);
    }
    emit userJoined(ch, nick);
  } else if (command == QLatin1String("PART")) {
    const QString ch = args.value(0);
    if (nick.compare(m_nick, Qt::CaseInsensitive) == 0) {
      m_channel.clear();
      emit channelParted(ch);
    }
    emit userParted(ch, nick, trailing);
  } else if (command == QLatin1String("QUIT")) {
    emit userQuit(nick, trailing);
  } else if (command == QLatin1String("NICK")) {
    const QString neu = trailing.isEmpty() ? args.value(0) : trailing;
    if (nick.compare(m_nick, Qt::CaseInsensitive) == 0)
      m_nick = neu;
    emit nickChanged(nick, neu);
  } else if (command == QLatin1String("TOPIC")) {
    emit topicChanged(args.value(0), trailing);
  } else {
    emit serverMessage(line);
  }
}

void IrcClient::handleNumeric(int code, const QString &,
                              const QStringList &args,
                              const QString &trailing) {
  switch (code) {
  case 1: // RPL_WELCOME
    m_registered = true;
    emit serverMessage(trailing);
    if (!m_pendingChannel.isEmpty())
      joinChannel(m_pendingChannel);
    break;
  case 331:
  case 332:
    emit topicChanged(args.value(1), trailing);
    break;
  case 353: { // RPL_NAMREPLY
    const QString ch = args.value(2);
    QStringList nicks = trailing.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    for (QString &n : nicks) {
      while (!n.isEmpty() && QStringLiteral("@%+&~").contains(n[0]))
        n = n.mid(1);
    }
    m_namesAccum[ch].append(nicks);
    break;
  }
  case 366: { // RPL_ENDOFNAMES
    const QString ch = args.value(1);
    emit namesList(ch, m_namesAccum.take(ch));
    break;
  }
  case 372:
  case 375:
  case 376:
  case 422:
    emit serverMessage(trailing);
    break;
  case 433:
    emit connectionError(tr("Nickname already in use"));
    emit serverMessage(trailing);
    break;
  default:
    if (!trailing.isEmpty())
      emit serverMessage(QStringLiteral("[%1] %2").arg(code).arg(trailing));
    break;
  }
}

void IrcClient::handlePrivmsg(const QString &nick, const QString &target,
                              const QString &text) {
  // Comic Chat control messages are plain PRIVMSG text beginning with '#'.
  if (text.startsWith(QLatin1Char('#'))) {
    const QString body = text.mid(1);
    // Prefixes keep the leading space, e.g. " Appears as anna"
    if (body.startsWith(QLatin1String(" Appears as "), Qt::CaseInsensitive)) {
      emit appearsAs(nick, body.mid(12).trimmed());
      return;
    }
    if (body.startsWith(QLatin1String(" BDrop: "), Qt::CaseInsensitive)) {
      emit backdropAnnounce(nick, body.mid(8).trimmed());
      return;
    }
    if (body.startsWith(QLatin1String(" BDrop2: "), Qt::CaseInsensitive)) {
      const QString rest = body.mid(9).trimmed();
      const int comma = rest.indexOf(QLatin1Char(','));
      emit backdropAnnounce(nick, comma >= 0 ? rest.left(comma) : rest);
      return;
    }
    if (body.startsWith(QLatin1String(" HeresInfo: "), Qt::CaseInsensitive)) {
      emit heresInfo(nick, body.mid(12).trimmed());
      return;
    }
    // Other #-comment control messages (GetInfo, GetCharInfo, …) are not
    // conversation text — swallow them so they never reach the room.
    return;
  }

  if (text.startsWith(QChar(0x01)) && text.endsWith(QChar(0x01))) {
    const QString ctcp = text.mid(1, text.size() - 2);
    if (ctcp.startsWith(QLatin1String("ACTION "), Qt::CaseInsensitive)) {
      emit action(target, nick, ctcp.mid(7));
      return;
    }
    if (ctcp.startsWith(QLatin1String("Appears as "), Qt::CaseInsensitive)) {
      emit appearsAs(nick, ctcp.mid(11).trimmed());
      return;
    }
    if (ctcp.startsWith(QLatin1String("BDrop:"), Qt::CaseInsensitive)) {
      emit backdropAnnounce(nick, ctcp.mid(6).trimmed());
      return;
    }
    if (ctcp.startsWith(QLatin1String("HeresInfo:"), Qt::CaseInsensitive) ||
        ctcp.startsWith(QLatin1String("HeresInfo"), Qt::CaseInsensitive)) {
      emit heresInfo(nick, ctcp);
      return;
    }
    // Ignore other CTCP
    return;
  }

  // Also accept plain-text Comic Chat markers used by some clients
  if (text.startsWith(QLatin1String("Appears as "), Qt::CaseInsensitive)) {
    emit appearsAs(nick, text.mid(11).trimmed());
    return;
  }
  if (text.startsWith(QLatin1String("BDrop:"), Qt::CaseInsensitive)) {
    emit backdropAnnounce(nick, text.mid(6).trimmed());
    return;
  }

  emit privmsg(target, nick, text);
}
