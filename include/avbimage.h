#pragma once

#include "avbformat.h"

#include <QColor>
#include <QImage>
#include <QVector>
#include <array>

class AvbStream;

class AvbPalette
{
public:
    bool read(AvbStream *stream);
    bool setFrom(const QVector<QRgb> &colors);

    int count() const { return m_colors.size(); }
    const QVector<QRgb> &colors() const { return m_colors; }

private:
    QVector<QRgb> m_colors;
};

struct AvbImageRef {
    quint32 streamOffset = 0;
    quint8 format = AIF_DIB;
    quint8 paletteType = AIP_NOPALETTE;
    const AvbPalette *globalPalette = nullptr;
};

// Decode one pose layer (drawing / mask / aura) from an open AVB stream.
bool loadAvbImage(AvbStream *stream, const AvbImageRef &ref, QImage *out);

// Apply a 1-bpp or grayscale mask so white=transparent, black=opaque (Comic Chat GDI style).
QImage applyComicMask(const QImage &drawing, const QImage &mask);

// Compute whether a pixel's luma marks it as "opaque" (black drawing) per Comic Chat masks.
inline bool maskPixelOpaque(const QRgb &c)
{
    // 0 = pure black = keep, anything else (white / transparent) = drop.
    return qRed(c) < 32 && qGreen(c) < 32 && qBlue(c) < 32;
}

// Expand AIP_MASKEDMONO (2bpp) into drawing + mask + aura monochrome images.
bool convertMaskedMono(const QImage &src2bpp, QImage *drawing, QImage *mask, QImage *aura);

// Expand AIP_DUALMASK into two monochrome masks.
bool convertDualMask(const QImage &src2bpp, QImage *mask, QImage *aura);

int dibStorageWidth(int width, int bitCount);
QImage dibToQImage(const QByteArray &bmiHeaderPlusColors, const QByteArray &bits,
                   int width, int height, int bitCount, const QVector<QRgb> &palette);
bool loadBmpFromStream(AvbStream *stream, QImage *out);
