#include "artmanager.h"
#include "avbstream.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QRandomGenerator>

ArtManager::ArtManager(QObject *parent) : QObject(parent) {}

void ArtManager::setArtDirectory(const QString &dir) {
  m_artDir = dir;
  m_avatarNames.clear();
  m_backdropNames.clear();
  m_avatars.clear();
  m_backdrops.clear();
  m_nextAvatarIndex = 0;
}

bool ArtManager::scan() {
  m_avatarNames.clear();
  m_backdropNames.clear();

  QDir dir(m_artDir);
  if (!dir.exists())
    return false;

  const QFileInfoList avbs =
      dir.entryInfoList({QStringLiteral("*.avb")}, QDir::Files, QDir::Name);
  for (const QFileInfo &fi : avbs)
    m_avatarNames.append(fi.completeBaseName());

  const QFileInfoList bgbs =
      dir.entryInfoList({QStringLiteral("*.bgb"), QStringLiteral("*.bmp")},
                        QDir::Files, QDir::Name);
  for (const QFileInfo &fi : bgbs)
    m_backdropNames.append(fi.completeBaseName());

  return hasArt();
}

Avatar *ArtManager::avatar(const QString &name) {
  if (name.isEmpty())
    return nullptr;

  const QString key = name.toLower();
  if (m_avatars.contains(key))
    return m_avatars.value(key).get();

  // Resolve actual casing from scanned list
  QString fileBase = name;
  for (const QString &n : m_avatarNames) {
    if (n.compare(name, Qt::CaseInsensitive) == 0) {
      fileBase = n;
      break;
    }
  }

  const QString path =
      QDir(m_artDir).filePath(fileBase + QStringLiteral(".avb"));
  if (!QFileInfo::exists(path))
    return nullptr;

  auto *stream = new AvbFileStream(path);
  std::unique_ptr<Avatar> av = Avatar::load(stream);
  if (!av) {
    delete stream;
    return nullptr;
  }
  av->setFileName(fileBase);
  if (av->name().isEmpty())
    av->setFileName(fileBase);

  Avatar *raw = av.get();
  m_avatars.insert(key, std::shared_ptr<Avatar>(av.release()));
  return raw;
}

Backdrop *ArtManager::backdrop(const QString &name) {
  if (name.isEmpty())
    return nullptr;

  const QString key = name.toLower();
  if (m_backdrops.contains(key))
    return m_backdrops.value(key).get();

  QString fileBase = name;
  for (const QString &n : m_backdropNames) {
    if (n.compare(name, Qt::CaseInsensitive) == 0) {
      fileBase = n;
      break;
    }
  }

  QDir dir(m_artDir);
  QString path = dir.filePath(fileBase + QStringLiteral(".bgb"));
  if (!QFileInfo::exists(path))
    path = dir.filePath(fileBase + QStringLiteral(".bmp"));
  if (!QFileInfo::exists(path))
    return nullptr;

  auto bd = Backdrop::loadFile(path);
  if (!bd)
    return nullptr;

  Backdrop *raw = bd.get();
  m_backdrops.insert(key, std::shared_ptr<Backdrop>(bd.release()));
  return raw;
}

Avatar *ArtManager::avatarOrRandom(const QString &preferred) {
  if (Avatar *a = avatar(preferred))
    return a;
  if (m_avatarNames.isEmpty())
    return nullptr;
  const int idx = QRandomGenerator::global()->bounded(m_avatarNames.size());
  return avatar(m_avatarNames.at(idx));
}

Backdrop *ArtManager::defaultBackdrop() {
  const QStringList prefer = {
      QStringLiteral("room"),
      QStringLiteral("buckroom"),
      QStringLiteral("clouds"),
  };
  for (const QString &n : prefer) {
    if (Backdrop *b = backdrop(n))
      return b;
  }
  if (!m_backdropNames.isEmpty())
    return backdrop(m_backdropNames.first());
  return nullptr;
}

QString ArtManager::nextAvatarName() {
  if (m_avatarNames.isEmpty())
    return {};
  const QString n = m_avatarNames.at(m_nextAvatarIndex % m_avatarNames.size());
  ++m_nextAvatarIndex;
  return n;
}

QString ArtManager::resolveDefaultArtDir() {
#ifdef CCNG_DEFAULT_ART_DIR
  const QString baked = QString::fromUtf8(CCNG_DEFAULT_ART_DIR);
  if (QDir(baked).exists())
    return baked;
#endif

  const QStringList candidates = {
      QDir::current().filePath(QStringLiteral("v2.5-beta-1-modern/comicart")),
      QDir::current().filePath(QStringLiteral("comicart")),
      QDir(QCoreApplication::applicationDirPath())
          .filePath(QStringLiteral("comicart")),
      QDir(QCoreApplication::applicationDirPath())
          .filePath(QStringLiteral("../v2.5-beta-1-modern/comicart")),
      QDir(QCoreApplication::applicationDirPath())
          .filePath(QStringLiteral("../../v2.5-beta-1-modern/comicart")),
  };
  for (const QString &c : candidates) {
    if (QDir(c).exists())
      return QFileInfo(c).absoluteFilePath();
  }
  return candidates.first();
}
