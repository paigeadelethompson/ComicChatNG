#pragma once

#include <QString>

struct AppSettings {
  QString nick = QStringLiteral("ComicUser");
  QString user = QStringLiteral("comic");
  QString realName = QStringLiteral("ComicChatNG");
  QString server = QStringLiteral("irc.libera.chat");
  quint16 port = 6667;
  QString channel = QStringLiteral("#Comic_Chat");
  QString avatarName = QStringLiteral("anna");
  QString backdropName = QStringLiteral("room");
  QString artDirectory;
  bool comicView = true;

  void load();
  void save() const;
};
