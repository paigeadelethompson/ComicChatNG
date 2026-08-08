#pragma once

#include <QImage>
#include <QString>
#include <memory>

class AvbStream;
class AvbFileStream;

class Backdrop
{
public:
    static std::unique_ptr<Backdrop> loadFile(const QString &path);

    QString name() const { return m_name; }
    QString fileName() const { return m_fileName; }
    void setFileName(const QString &n) { m_fileName = n; }
    QString copyright() const { return m_copyright; }
    QImage image() const { return m_image; }

private:
    static std::unique_ptr<Backdrop> loadFromStream(AvbFileStream *stream);
    bool loadAvb(AvbStream *stream);
    bool loadBmp(AvbStream *stream);

    QString m_name;
    QString m_fileName;
    QString m_copyright;
    QImage m_image;
};
