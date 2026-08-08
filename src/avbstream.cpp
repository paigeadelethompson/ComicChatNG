#include "avbstream.h"
#include "avbformat.h"

#include <zlib.h>

#include <cstdio>

bool AvbStream::read8(quint8 *v) { return read(v, 1) == 1; }

bool AvbStream::read16(quint16 *v) { return read(v, 2) == 2; }

bool AvbStream::read32(quint32 *v) { return read(v, 4) == 4; }

bool AvbStream::readString(QByteArray *out, int maxLen) {
  if (!out || maxLen <= 0)
    return false;
  out->clear();
  for (int i = 0; i < maxLen - 1; ++i) {
    char ch = 0;
    if (read(&ch, 1) != 1)
      return false;
    if (ch == '\0')
      return true;
    out->append(ch);
  }
  char nul = 0;
  read(&nul, 1);
  return true;
}

bool AvbStream::allocAndReadCompressed(QByteArray *out) {
  if (!out)
    return false;

  struct {
    quint32 uncompressed = 0;
    quint32 compressed = 0;
  } sizes;

  if (read(&sizes, sizeof(sizes)) != sizeof(sizes))
    return false;

  if (sizes.uncompressed == 0) {
    out->clear();
    return true;
  }

  if (sizes.uncompressed > static_cast<quint32>(MAX_COMPRESS_BUFFER_SIZE) ||
      sizes.compressed > static_cast<quint32>(MAX_COMPRESS_BUFFER_SIZE)) {
    return false;
  }

  QByteArray compressed;
  compressed.resize(static_cast<int>(sizes.compressed));
  if (read(compressed.data(), sizes.compressed) != sizes.compressed)
    return false;

  QByteArray uncompressed;
  uncompressed.resize(static_cast<int>(sizes.uncompressed));
  uLongf destLen = sizes.uncompressed;
  if (uncompress(reinterpret_cast<Bytef *>(uncompressed.data()), &destLen,
                 reinterpret_cast<const Bytef *>(compressed.constData()),
                 sizes.compressed) != Z_OK) {
    return false;
  }
  uncompressed.resize(static_cast<int>(destLen));
  *out = uncompressed;
  return true;
}

AvbFileStream::AvbFileStream(const QString &path) : m_path(path) {}

AvbFileStream::~AvbFileStream() {
  if (m_openCount > 0) {
    m_openCount = 1;
    close();
  }
}

bool AvbFileStream::open() {
  if (m_openCount > 0) {
    ++m_openCount;
    return true;
  }
  m_file.setFileName(m_path);
  if (!m_file.open(QIODevice::ReadOnly))
    return false;
  m_openCount = 1;
  return true;
}

bool AvbFileStream::close() {
  if (m_openCount <= 0)
    return false;
  if (m_openCount > 1) {
    --m_openCount;
    return true;
  }
  m_file.close();
  m_openCount = 0;
  return true;
}

quint32 AvbFileStream::read(void *data, quint32 size) {
  if (!m_file.isOpen())
    return 0;
  const qint64 n = m_file.read(reinterpret_cast<char *>(data), size);
  return n < 0 ? 0 : static_cast<quint32>(n);
}

qint64 AvbFileStream::position() const {
  return m_file.isOpen() ? m_file.pos() : -1;
}

bool AvbFileStream::setPosition(qint64 pos, int from) {
  if (!m_file.isOpen())
    return false;
  qint64 target = pos;
  if (from == SEEK_CUR)
    target = m_file.pos() + pos;
  else if (from == SEEK_END)
    target = m_file.size() + pos;
  return m_file.seek(target);
}
