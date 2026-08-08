#include "appsettings.h"
#include "artmanager.h"

#include <QDir>
#include <QSettings>

void AppSettings::load() {
  QSettings s(QStringLiteral("ComicChatNG"), QStringLiteral("ComicChatNG"));
  nick = s.value(QStringLiteral("nick"), nick).toString();
  user = s.value(QStringLiteral("user"), user).toString();
  realName = s.value(QStringLiteral("realName"), realName).toString();
  server = s.value(QStringLiteral("server"), server).toString();
  port = static_cast<quint16>(s.value(QStringLiteral("port"), port).toUInt());
  channel = s.value(QStringLiteral("channel"), channel).toString();
  avatarName = s.value(QStringLiteral("avatarName"), avatarName).toString();
  backdropName =
      s.value(QStringLiteral("backdropName"), backdropName).toString();
  artDirectory = s.value(QStringLiteral("artDirectory"), QString()).toString();
  comicView = s.value(QStringLiteral("comicView"), comicView).toBool();

  if (artDirectory.isEmpty() || !QDir(artDirectory).exists())
    artDirectory = ArtManager::resolveDefaultArtDir();
}

void AppSettings::save() const {
  QSettings s(QStringLiteral("ComicChatNG"), QStringLiteral("ComicChatNG"));
  s.setValue(QStringLiteral("nick"), nick);
  s.setValue(QStringLiteral("user"), user);
  s.setValue(QStringLiteral("realName"), realName);
  s.setValue(QStringLiteral("server"), server);
  s.setValue(QStringLiteral("port"), port);
  s.setValue(QStringLiteral("channel"), channel);
  s.setValue(QStringLiteral("avatarName"), avatarName);
  s.setValue(QStringLiteral("backdropName"), backdropName);
  s.setValue(QStringLiteral("artDirectory"), artDirectory);
  s.setValue(QStringLiteral("comicView"), comicView);
}
