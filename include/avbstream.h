#pragma once

#include <QByteArray>
#include <QFile>
#include <QString>
#include <cstdint>

class AvbStream {
public:
  virtual ~AvbStream() = default;

  virtual bool open() = 0;
  virtual bool close() = 0;
  virtual quint32 read(void *data, quint32 size) = 0;
  virtual qint64 position() const = 0;
  virtual bool setPosition(qint64 pos, int from) = 0;

  bool read8(quint8 *v);
  bool read16(quint16 *v);
  bool read32(quint32 *v);
  bool readString(QByteArray *out, int maxLen);
  bool allocAndReadCompressed(QByteArray *out);
};

class AvbFileStream : public AvbStream {
public:
  explicit AvbFileStream(const QString &path);
  ~AvbFileStream() override;

  bool open() override;
  bool close() override;
  quint32 read(void *data, quint32 size) override;
  qint64 position() const override;
  bool setPosition(qint64 pos, int from) override;

  QString path() const { return m_path; }

private:
  QString m_path;
  QFile m_file;
  int m_openCount = 0;
};
