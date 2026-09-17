#include "ircclient.h"

#include <QRegularExpression>

#include <QtMath>

namespace {
  // Byte index of an emotion value in the original Comic Chat emotion table,
  // where 0..9 go out as ASCII digits '0'..'9' and 10+ as ':'.. (rare).
  int emotionIndex(const Emotion &e) {
    if (e.emotion >= 1001.f) {
      const int g = int(e.emotion - 1001.f);
      if (g >= 0 && g <= 7)
        return 10 + g; // wave, pointother, pointself, doublepoint, shrug…
    }
    if (e.emotion == 0.f)
      return 9; // neutral
    const qreal step = 2.0 * M_PI / 8.0;
    int i = qRound(qreal(e.emotion) / step);
    i = ((i % 8) + 8) % 8;
    return i + 1; // happy..laugh
  }

  char toByte(int v) { return char('0' + qBound(0, v, 9)); }
  bool hasPrefix(const QString &s, const char *p) {
    return s.startsWith(QLatin1String(p), Qt::CaseInsensitive);
  }

  // Channel status / IRC-operator prefixes (@ + % & ~ and the * used by some
  // ircds for IRCops). They decorate names in NAMES and in talk-to lists but
  // are not part of the nick, so they must be dropped before matching members.
  QString stripNickStatus(QString n) {
    while (!n.isEmpty() && QStringLiteral("~&@%+*").contains(n.at(0)))
      n = n.mid(1);
    return n;
  }
} // namespace

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

void IrcClient::announceAppearance(const QString &target,
                                   const QString &avatarName) {
  // Comic Chat protocol: "# Appears as <name>[.<url>]"
  appearAs(target, avatarName);
}

void IrcClient::announceBackdrop(const QString &channel,
                                 const QString &backdropName) {
  if (backdropName.isEmpty())
    return;
  // New clients recognise "# BDrop2: <name>,<url>"; older ones read "# BDrop:
  // <name>". Only the bare name goes in the legacy form (no file extension).
  const int dot = backdropName.indexOf(QLatin1Char('.'));
  const QString baseName = dot > 0 ? backdropName.left(dot) : backdropName;
  sendPrivmsg(channel, QStringLiteral("# BDrop2: %1,").arg(backdropName));
  sendPrivmsg(channel, QStringLiteral("# BDrop: %1").arg(baseName));
}

void IrcClient::sendComicMessage(const QString &target, const QString &text,
                                 ComicMode mode, const QStringList &talkTos) {
  const QString header = buildAnnotations(mode, talkTos);
  if (mode == ComicMode::Action) {
    writeLine(QStringLiteral("PRIVMSG %1 :%2").arg(target, header) +
              QStringLiteral("\x01"
                             "ACTION %1\x01")
                  .arg(text));
  } else {
    writeLine(QStringLiteral("PRIVMSG %1 :%2%3").arg(target, header, text));
  }
}

void IrcClient::requestProfile(const QString &nick) {
  sendPrivmsg(nick, QStringLiteral("# GetInfo"));
}

void IrcClient::sendProfile(const QString &nick, const QString &profile) {
  sendPrivmsg(nick, QStringLiteral("# HeresInfo: %1").arg(profile));
}

void IrcClient::requestAvatarInfo(const QString &nick) {
  sendPrivmsg(nick, QStringLiteral("# GetCharInfo"));
}

void IrcClient::appearAs(const QString &target, const QString &avatarName,
                         const QString &url) {
  if (avatarName.isEmpty())
    return;
  if (!url.isEmpty())
    sendPrivmsg(target,
                QStringLiteral("# Appears as %1.%2").arg(avatarName, url));
  else
    sendPrivmsg(target, QStringLiteral("# Appears as %1").arg(avatarName));
}

void IrcClient::requestIdentity(const QString &nick) {
  writeLine(QStringLiteral("WHOIS %1 %1").arg(nick));
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
    for (QString &n : nicks)
      n = stripNickStatus(n);
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
  case 311: // RPL_WHOISUSER: <nick> <user> <host> * :<real name>
    emit identityInfo(args.value(1), trailing);
    break;
  default:
    if (!trailing.isEmpty())
      emit serverMessage(QStringLiteral("[%1] %2").arg(code).arg(trailing));
    break;
  }
}

void IrcClient::handlePrivmsg(const QString &nick, const QString &target,
                              const QString &rawText) {
  QString text = rawText;
  Emotion anno;
  int annoMode = 0;
  QStringList annoTalkTos;
  if (stripAnnotations(text, &anno, &annoMode, &annoTalkTos) &&
      anno.intensity > 0.01f)
    emit messageEmotion(nick, anno);
  if (!annoTalkTos.isEmpty())
    emit talkTo(nick, annoTalkTos);

  // Comic Chat control messages are plain PRIVMSG text beginning with '#'.
  if (text.startsWith(QLatin1Char('#'))) {
    const QString body = text.mid(1);
    // Prefixes keep the leading space, e.g. " Appears as anna"
    if (hasPrefix(body, " Appears as ")) {
      QString n = body.mid(12).trimmed();
      const int dot = n.indexOf(QLatin1Char('.'));
      if (dot > 0)
        n = n.left(dot);
      emit appearsAs(nick, n);
      return;
    }
    if (hasPrefix(body, " BDrop: ")) {
      emit backdropAnnounce(nick, body.mid(8).trimmed());
      return;
    }
    if (hasPrefix(body, " BDrop2: ")) {
      const QString rest = body.mid(9).trimmed();
      const int comma = rest.indexOf(QLatin1Char(','));
      emit backdropAnnounce(nick, comma >= 0 ? rest.left(comma) : rest);
      return;
    }
    if (hasPrefix(body, " HeresInfo: ")) {
      emit heresInfo(nick, body.mid(12).trimmed());
      return;
    }
    if (hasPrefix(body, " GetInfo")) {
      // Someone asked for our profile — answer privately.
      sendProfile(nick, m_selfProfile.isEmpty() ? m_realName : m_selfProfile);
      return;
    }
    if (hasPrefix(body, " GetCharInfo")) {
      // Someone asked for our avatar — tell them what we look like.
      appearAs(nick, m_selfAvatar);
      return;
    }
    // Other #-comment control messages never reach the room.
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
    QString n = text.mid(11).trimmed();
    const int dot = n.indexOf(QLatin1Char('.'));
    if (dot > 0)
      n = n.left(dot);
    emit appearsAs(nick, n);
    return;
  }
  if (text.startsWith(QLatin1String("BDrop:"), Qt::CaseInsensitive)) {
    emit backdropAnnounce(nick, text.mid(6).trimmed());
    return;
  }

  // The "M" annotation byte tells us the balloon kind even when the peer did
  // not use a separate CTCP envelope.
  if (annoMode == static_cast<int>(ComicMode::Action)) {
    emit action(target, nick, text);
    return;
  }
  if (annoMode == static_cast<int>(ComicMode::Think)) {
    emit think(target, nick, text);
    return;
  }
  if (annoMode == static_cast<int>(ComicMode::Whisper)) {
    emit whisper(target, nick, text);
    return;
  }

  emit privmsg(target, nick, text);
}

QString IrcClient::buildAnnotations(ComicMode mode,
                                    const QStringList &talkTos) const {
  // "#G<torsoIndex><torsoEmotion><torsoIntensity>E<faceIndex><faceEmotion>
  // <faceIntensity>M<mode>[T<addr>,<addr>]" — same layout bInsertAnnotations()
  // produced in the original client. Torso/face indices are 0 (single-part
  // avatars); the talk-tos are clipped at 5 like GetAddressees() did.
  const int em = emotionIndex(m_selfEmotion);
  const int in = int(m_selfEmotion.intensity * 10.0f + 0.5f);
  const int modeByte = static_cast<int>(mode);
  QString s = QStringLiteral("(#G0");
  s += QLatin1Char(toByte(em));
  s += QLatin1Char(toByte(in));
  s += QLatin1String("E0");
  s += QLatin1Char(toByte(em));
  s += QLatin1Char(toByte(in));
  s += QLatin1Char('M');
  s += QLatin1Char(toByte(modeByte));
  if (!talkTos.isEmpty()) {
    s += QLatin1Char('T');
    const int upto = qMin(talkTos.size(), 5);
    for (int i = 0; i < upto; ++i) {
      if (i > 0)
        s += QLatin1Char(',');
      s += talkTos.at(i);
    }
  }
  s += QLatin1String(") ");
  return s;
}

bool IrcClient::stripAnnotations(QString &text, Emotion *faceEmotion, int *mode,
                                 QStringList *talkTos) {
  if (faceEmotion)
    *faceEmotion = Emotion();
  if (mode)
    *mode = 0;
  if (talkTos)
    talkTos->clear();
  if (!text.startsWith(QLatin1String("(#")))
    return false;
  const int close = text.indexOf(QLatin1String(") "));
  if (close < 2)
    return false;

  const QByteArray body = text.mid(2, close - 2).toLatin1();
  Emotion face;
  int parsedMode = 0;
  const int n = body.size();
  int i = 0;
  while (i < n) {
    const char c = body.at(i);
    if (c == 'G' || c == 'g') {
      i += 4; // <torso index><emotion><intensity>
    } else if (c == 'E' || c == 'e') {
      if (i + 3 <= n) {
        const int emInt = body.at(i + 1) - '0';
        const int inInt = body.at(i + 2) - '0';
        const qreal step = 2.0 * M_PI / 8.0;
        if (emInt >= 1 && emInt <= 8)
          face.emotion = float((emInt - 1) * step);
        else
          face.emotion = 0.f;
        face.intensity = qBound(0.f, float(inInt) / 10.f, 1.f);
      }
      i += 4;
    } else if (c == 'R' || c == 'r') {
      i += 1; // "requested" flag
    } else if (c == 'M' || c == 'm') {
      if (i + 1 < n) {
        const int m = body.at(i + 1) - '0';
        if (m >= 1 && m <= 5)
          parsedMode = m;
      }
      i += 2;
    } else if (c == 'T' || c == 't') {
      // talk-tos: comma-separated nicks, Go until the closing ')'. GetTalkTos
      // in the original stopped at ')' or end-of-string, same here.
      int j = i + 1;
      while (j < n) {
        int k = j;
        while (k < n && body.at(k) != ',')
          ++k;
        const QString name =
            stripNickStatus(QString::fromLatin1(body.mid(j, k - j)).trimmed());
        if (!name.isEmpty() && talkTos)
          talkTos->append(name);
        j = k + 1;
      }
      i = n;
    } else {
      break;
    }
  }

  text = text.mid(close + 2);
  if (faceEmotion)
    *faceEmotion = face;
  if (mode)
    *mode = parsedMode;
  return true;
}
