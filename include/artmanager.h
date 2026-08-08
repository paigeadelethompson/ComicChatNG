#pragma once

#include "avatar.h"
#include "backdrop.h"

#include <QHash>
#include <QObject>
#include <QStringList>
#include <memory>

class ArtManager : public QObject
{
    Q_OBJECT
public:
    explicit ArtManager(QObject *parent = nullptr);

    void setArtDirectory(const QString &dir);
    QString artDirectory() const { return m_artDir; }

    bool scan();
    bool hasArt() const { return !m_avatarNames.isEmpty() && !m_backdropNames.isEmpty(); }

    QStringList avatarNames() const { return m_avatarNames; }
    QStringList backdropNames() const { return m_backdropNames; }

    Avatar *avatar(const QString &name);
    Backdrop *backdrop(const QString &name);

    Avatar *avatarOrRandom(const QString &preferred);
    Backdrop *defaultBackdrop();

    QString nextAvatarName();

    static QString resolveDefaultArtDir();

private:
    QString m_artDir;
    QStringList m_avatarNames;
    QStringList m_backdropNames;
    QHash<QString, std::shared_ptr<Avatar>> m_avatars;
    QHash<QString, std::shared_ptr<Backdrop>> m_backdrops;
    int m_nextAvatarIndex = 0;
};
