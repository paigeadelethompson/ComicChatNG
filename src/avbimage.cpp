#include "avbimage.h"
#include "avbstream.h"

#include <QPainter>
#include <cstring>

namespace {

#pragma pack(push, 1)
  struct BmpFileHeader {
    quint16 type = 0;
    quint32 size = 0;
    quint16 reserved1 = 0;
    quint16 reserved2 = 0;
    quint32 offBits = 0;
  };

  struct BmpInfoHeader {
    quint32 size = 0;
    qint32 width = 0;
    qint32 height = 0;
    quint16 planes = 0;
    quint16 bitCount = 0;
    quint32 compression = 0;
    quint32 sizeImage = 0;
    qint32 xPelsPerMeter = 0;
    qint32 yPelsPerMeter = 0;
    quint32 clrUsed = 0;
    quint32 clrImportant = 0;
  };

  struct RgbQuad {
    quint8 b = 0, g = 0, r = 0, reserved = 0;
  };
#pragma pack(pop)

  const QVector<QRgb> kMonoPalette = {
      qRgb(255, 255, 255),
      qRgb(0, 0, 0),
  };

  const QVector<QRgb> kMaskedMonoPalette = {
      qRgb(255, 255, 255),
      qRgb(0, 0, 0),
      qRgb(128, 0, 0),
      qRgb(0, 0, 128),
  };

  int numColors(const BmpInfoHeader &h) {
    if (h.clrUsed != 0)
      return static_cast<int>(h.clrUsed);
    if (h.bitCount <= 8)
      return 1 << h.bitCount;
    return 0;
  }

  QImage imageFromIndexedBits(int width, int height, int bitCount,
                              const QVector<QRgb> &palette,
                              const QByteArray &bits) {
    if (width <= 0 || height <= 0 || bits.isEmpty())
      return {};

    const bool topDown = height < 0;
    const int absH = qAbs(height);
    const int stride = dibStorageWidth(width, bitCount);
    if (bits.size() < stride * absH)
      return {};

    QImage img(width, absH, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    auto sample = [&](int x, int yRow) -> int {
      const uchar *row =
          reinterpret_cast<const uchar *>(bits.constData()) + yRow * stride;
      if (bitCount == 1) {
        const int byte = row[x / 8];
        return (byte >> (7 - (x % 8))) & 1;
      }
      if (bitCount == 2) {
        // 2bpp: packed 4 pixels per byte, high bits first (Comic Chat style).
        const int byte = row[x / 4];
        const int shift = (3 - (x % 4)) * 2;
        return (byte >> shift) & 0x3;
      }
      if (bitCount == 4) {
        const int byte = row[x / 2];
        return (x & 1) ? (byte & 0xF) : ((byte >> 4) & 0xF);
      }
      if (bitCount == 8)
        return row[x];
      return 0;
    };

    for (int y = 0; y < absH; ++y) {
      const int srcY = topDown ? y : (absH - 1 - y);
      QRgb *dst = reinterpret_cast<QRgb *>(img.scanLine(y));
      for (int x = 0; x < width; ++x) {
        const int idx = sample(x, srcY);
        if (idx >= 0 && idx < palette.size())
          dst[x] = palette[idx];
        else
          dst[x] = qRgb(0, 0, 0);
      }
    }
    return img;
  }

  bool getProperPalette(AvbStream *stream, const AvbImageRef &ref,
                        AvbPalette *pal) {
    switch (ref.paletteType) {
    case AIP_NOPALETTE:
      return true;
    case AIP_GLOBALPALETTE:
      if (!ref.globalPalette)
        return false;
      return pal->setFrom(ref.globalPalette->colors());
    case AIP_LOCALPALETTE: {
      quint16 tag = 0, size = 0;
      if (stream->read(&tag, 2) != 2 || stream->read(&size, 2) != 2)
        return false;
      if (tag != AK_COLORPALETTE)
        return false;
      return pal->read(stream);
    }
    case AIP_MONOCHROME:
      return pal->setFrom(kMonoPalette);
    case AIP_MASKEDMONO:
    case AIP_DUALMASK:
      return pal->setFrom(kMaskedMonoPalette);
    default:
      return false;
    }
  }

  bool readZlibImage(AvbStream *stream, const AvbImageRef &ref, QImage *out) {
    if (!stream->setPosition(ref.streamOffset, SEEK_SET))
      return false;

    AvbPalette pal;
    if (!getProperPalette(stream, ref, &pal))
      return false;

    quint32 headerSize = 0;
    if (!stream->read32(&headerSize))
      return false;
    if (headerSize < sizeof(BmpInfoHeader) ||
        headerSize > sizeof(BmpInfoHeader) * 6)
      return false;

    BmpInfoHeader bih{};
    bih.size = headerSize;
    const quint32 rest = headerSize - sizeof(quint32);
    if (stream->read(reinterpret_cast<char *>(&bih) + sizeof(quint32), rest) !=
        rest)
      return false;

    if (bih.bitCount == 0)
      return false;

    QByteArray bits;
    if (!stream->allocAndReadCompressed(&bits))
      return false;

    const int expected =
        dibStorageWidth(bih.width, bih.bitCount) * qAbs(bih.height);
    if (bits.size() != expected)
      return false;

    *out = imageFromIndexedBits(bih.width, bih.height, bih.bitCount,
                                pal.colors(), bits);
    return !out->isNull();
  }

  bool readDibImage(AvbStream *stream, const AvbImageRef &ref, QImage *out) {
    if (!stream->setPosition(ref.streamOffset, SEEK_SET))
      return false;
    return loadBmpFromStream(stream, out);
  }

} // namespace

int dibStorageWidth(int width, int bitCount) {
  return ((width * bitCount + 31) / 32) * 4;
}

bool AvbPalette::read(AvbStream *stream) {
  m_colors.clear();
  quint16 nEntries = 0;
  if (!stream->read16(&nEntries))
    return false;
  if (nEntries > MAX_PALETTE_SIZE)
    return false;
  m_colors.reserve(nEntries);
  for (int i = 0; i < nEntries; ++i) {
    quint8 rgb[3] = {};
    if (stream->read(rgb, 3) != 3)
      return false;
    // Stored as COLORREF bytes (R,G,B) in file per Comic Chat reader.
    m_colors.append(qRgb(rgb[0], rgb[1], rgb[2]));
  }
  return true;
}

bool AvbPalette::setFrom(const QVector<QRgb> &colors) {
  m_colors = colors;
  return true;
}

bool loadBmpFromStream(AvbStream *stream, QImage *out) {
  const qint64 start = stream->position();
  if (start < 0)
    return false;

  BmpFileHeader fh{};
  if (stream->read(&fh, sizeof(fh)) != sizeof(fh))
    return false;
  if (fh.type != 0x4D42)
    return false;

  BmpInfoHeader bih{};
  if (stream->read(&bih, sizeof(bih)) != sizeof(bih))
    return false;
  if (bih.size != sizeof(BmpInfoHeader))
    return false;
  if (bih.bitCount == 0)
    return false;

  const int colors = numColors(bih);
  QVector<QRgb> palette;
  palette.reserve(colors);
  for (int i = 0; i < colors; ++i) {
    RgbQuad q{};
    if (stream->read(&q, sizeof(q)) != sizeof(q))
      return false;
    palette.append(qRgb(q.r, q.g, q.b));
  }

  if (!stream->setPosition(start + fh.offBits, SEEK_SET))
    return false;

  const int bitsSize = static_cast<int>(fh.size - fh.offBits);
  if (bitsSize <= 0)
    return false;
  QByteArray bits;
  bits.resize(bitsSize);
  if (stream->read(bits.data(), static_cast<quint32>(bitsSize)) !=
      static_cast<quint32>(bitsSize))
    return false;

  if (bih.bitCount == 24 || bih.bitCount == 32) {
    // Rebuild a minimal BMP for QImage.
    QByteArray bmp;
    bmp.append(reinterpret_cast<const char *>(&fh), sizeof(fh));
    // Fix header sizes relative to our slice — easier path: decode manually for
    // 24bpp.
    const int absH = qAbs(bih.height);
    const int stride = dibStorageWidth(bih.width, bih.bitCount);
    QImage img(bih.width, absH, QImage::Format_ARGB32);
    for (int y = 0; y < absH; ++y) {
      const int srcY = (bih.height < 0) ? y : (absH - 1 - y);
      const uchar *row =
          reinterpret_cast<const uchar *>(bits.constData()) + srcY * stride;
      QRgb *dst = reinterpret_cast<QRgb *>(img.scanLine(y));
      for (int x = 0; x < bih.width; ++x) {
        if (bih.bitCount == 24) {
          const uchar *p = row + x * 3;
          dst[x] = qRgb(p[2], p[1], p[0]);
        } else {
          const uchar *p = row + x * 4;
          dst[x] = qRgba(p[2], p[1], p[0], p[3]);
        }
      }
    }
    *out = img;
    return true;
  }

  *out =
      imageFromIndexedBits(bih.width, bih.height, bih.bitCount, palette, bits);
  return !out->isNull();
}

bool loadAvbImage(AvbStream *stream, const AvbImageRef &ref, QImage *out) {
  if (!out || !stream || ref.streamOffset == 0)
    return false;
  switch (ref.format) {
  case AIF_DIB:
    return readDibImage(stream, ref, out);
  case AIF_LZDEFLATE:
    return readZlibImage(stream, ref, out);
  default:
    return false;
  }
}

QImage applyComicMask(const QImage &drawing, const QImage &mask) {
  if (drawing.isNull())
    return {};
  QImage src = drawing.convertToFormat(QImage::Format_ARGB32);
  if (mask.isNull())
    return src;

  QImage m =
      mask.convertToFormat(QImage::Format_ARGB32)
          .scaled(src.size(), Qt::IgnoreAspectRatio, Qt::FastTransformation);

  for (int y = 0; y < src.height(); ++y) {
    QRgb *d = reinterpret_cast<QRgb *>(src.scanLine(y));
    const QRgb *ms = reinterpret_cast<const QRgb *>(m.constScanLine(y));
    for (int x = 0; x < src.width(); ++x) {
      // Comic Chat: black in mask = keep, white = transparent.
      const int lum = qGray(ms[x]);
      if (lum > 128)
        d[x] = qRgba(0, 0, 0, 0);
      else
        d[x] = qRgba(qRed(d[x]), qGreen(d[x]), qBlue(d[x]), 255);
    }
  }
  return src;
}

bool convertMaskedMono(const QImage &src2bpp, QImage *drawing, QImage *mask,
                       QImage *aura) {
  // src pixels use MaskedMonoPalette indices: 00 blank, 01 aura, 10 white, 11
  // black. The conversion mirrors the original GDI lookup tables:
  //   value 00 -> blank             (no pixels anywhere)
  //   value 01 -> aura "nimbus"     (soft halo; mask transparent)
  //   value 10 -> white fill        (mask opaque, drawing white)
  //   value 11 -> black ink         (mask opaque, drawing black)
  if (src2bpp.isNull() || !drawing || !mask || !aura)
    return false;

  const int w = src2bpp.width();
  const int h = src2bpp.height();
  *drawing = QImage(w, h, QImage::Format_ARGB32);
  *mask = QImage(w, h, QImage::Format_ARGB32);
  *aura = QImage(w, h, QImage::Format_ARGB32);
  drawing->fill(Qt::transparent);
  mask->fill(qRgb(255, 255, 255));
  aura->fill(qRgb(255, 255, 255));

  QImage s = src2bpp.convertToFormat(QImage::Format_ARGB32);
  for (int y = 0; y < h; ++y) {
    const QRgb *sp = reinterpret_cast<const QRgb *>(s.constScanLine(y));
    QRgb *dp = reinterpret_cast<QRgb *>(drawing->scanLine(y));
    QRgb *mp = reinterpret_cast<QRgb *>(mask->scanLine(y));
    QRgb *ap = reinterpret_cast<QRgb *>(aura->scanLine(y));
    for (int x = 0; x < w; ++x) {
      const QRgb c = sp[x];
      const int r = qRed(c), g = qGreen(c), b = qBlue(c);
      int code = 0;
      if (r == 255 && g == 255 && b == 255)
        code = 0; // 00 blank
      else if (r == 0 && g == 0 && b == 0)
        code = 1; // 01 aura
      else if (r == 128 && g == 0 && b == 0)
        code = 2; // 10 white fill
      else if (r == 0 && g == 0 && b == 128)
        code = 3; // 11 black ink
      else if (qGray(c) < 64)
        code = 3;
      else
        code = 0;

      switch (code) {
      case 0: // blank
        break;
      case 1: // aura
        ap[x] = qRgb(0, 0, 0);
        break;
      case 2: // white fill: opaque, drawing white
        dp[x] = qRgb(255, 255, 255);
        mp[x] = qRgb(0, 0, 0);
        break;
      case 3: // black ink: opaque, drawing black
        dp[x] = qRgb(0, 0, 0);
        mp[x] = qRgb(0, 0, 0);
        break;
      }
    }
  }
  return true;
}

bool convertDualMask(const QImage &src2bpp, QImage *mask, QImage *aura) {
  // AIP_DUALMASK: two monochrome masks packed 2bpp. Bit 0 (value&1) = mask,
  // bit 1 (value&2) = aura. Value 0 = transparent, value 3 = both.
  if (src2bpp.isNull() || !mask || !aura)
    return false;
  const int w = src2bpp.width();
  const int h = src2bpp.height();
  *mask = QImage(w, h, QImage::Format_ARGB32);
  *aura = QImage(w, h, QImage::Format_ARGB32);
  mask->fill(qRgb(255, 255, 255));
  aura->fill(qRgb(255, 255, 255));

  QImage s = src2bpp.convertToFormat(QImage::Format_ARGB32);
  for (int y = 0; y < h; ++y) {
    const QRgb *sp = reinterpret_cast<const QRgb *>(s.constScanLine(y));
    QRgb *mp = reinterpret_cast<QRgb *>(mask->scanLine(y));
    QRgb *ap = reinterpret_cast<QRgb *>(aura->scanLine(y));
    for (int x = 0; x < w; ++x) {
      const QRgb c = sp[x];
      const int r = qRed(c), g = qGreen(c), b = qBlue(c);
      int v = 0;
      if (r == 255 && g == 255 && b == 255)
        v = 0;
      else if (r == 0 && g == 0 && b == 0)
        v = 1;
      else if (r == 128 && g == 0 && b == 0)
        v = 2;
      else if (r == 0 && g == 0 && b == 128)
        v = 3;
      else if (qGray(c) < 64)
        v = 3;
      else
        v = 0;

      if (v & 1)
        mp[x] = qRgb(0, 0, 0);
      if (v & 2)
        ap[x] = qRgb(0, 0, 0);
    }
  }
  return true;
}
