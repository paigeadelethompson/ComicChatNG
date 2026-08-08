#include "backdrop.h"
#include "avbformat.h"
#include "avbimage.h"
#include "avbstream.h"

#include <cstdio>
#include <memory>

std::unique_ptr<Backdrop> Backdrop::loadFile(const QString &path)
{
    auto stream = std::make_unique<AvbFileStream>(path);
    auto bd = loadFromStream(stream.get());
    if (bd) {
        stream.release(); // loadFromStream may keep stream only for avatars; backdrop is fully decoded
        const int slash = path.lastIndexOf(QLatin1Char('/'));
        const int bslash = path.lastIndexOf(QLatin1Char('\\'));
        const int idx = qMax(slash, bslash);
        QString base = idx >= 0 ? path.mid(idx + 1) : path;
        if (base.endsWith(QLatin1String(".bgb"), Qt::CaseInsensitive)
            || base.endsWith(QLatin1String(".bmp"), Qt::CaseInsensitive))
            base.chop(4);
        bd->setFileName(base);
    }
    return bd;
}

std::unique_ptr<Backdrop> Backdrop::loadFromStream(AvbFileStream *stream)
{
    if (!stream || !stream->open())
        return nullptr;

    quint16 magic = 0;
    if (!stream->read16(&magic)) {
        stream->close();
        return nullptr;
    }
    if (!stream->setPosition(-2, SEEK_CUR)) {
        stream->close();
        return nullptr;
    }

    auto bd = std::make_unique<Backdrop>();
    bool ok = false;
    if (magic == 0x4D42)
        ok = bd->loadBmp(stream);
    else if (magic == AF_MAGICNUM_NEW)
        ok = bd->loadAvb(stream);
    stream->close();
    if (!ok)
        return nullptr;
    return bd;
}

bool Backdrop::loadBmp(AvbStream *stream)
{
    return loadBmpFromStream(stream, &m_image);
}

bool Backdrop::loadAvb(AvbStream *stream)
{
    AvbHeader hdr {};
    if (stream->read(&hdr, sizeof(hdr)) != sizeof(hdr))
        return false;
    if (hdr.type != AT_BACKDROP)
        return false;
    if ((hdr.version >> 8) != 0)
        return false;

    qint32 adj = 0;
    quint32 backdropOffset = 0;
    quint8 format = AIF_DIB;
    quint8 paletteType = AIP_NOPALETTE;
    AvbPalette palette;
    bool found = false;

    while (true) {
        quint16 tag = 0;
        if (!stream->read16(&tag))
            return false;
        quint16 size = 0;
        if (tag >= AK_ICON_NEW && !stream->read16(&size))
            return false;

        if (tag == AK_STARTDATA) {
            if (!found)
                return false;
            break;
        }

        switch (tag) {
        case AK_NAME: {
            QByteArray s;
            if (!stream->readString(&s, 60))
                return false;
            m_name = QString::fromLatin1(s);
            break;
        }
        case AK_COPYRIGHT: {
            QByteArray s;
            if (!stream->readString(&s, 256))
                return false;
            m_copyright = QString::fromLatin1(s);
            break;
        }
        case AK_COLORPALETTE:
            if (!palette.read(stream))
                return false;
            break;
        case AK_OFFSET_ADJUSTMENT: {
            qint32 v = 0;
            if (!stream->read32(reinterpret_cast<quint32 *>(&v)))
                return false;
            adj += v;
            break;
        }
        case AK_BACKDROP: {
            quint32 off = 0;
            quint8 fmt = 0, pal = 0;
            if (!stream->read32(&off) || !stream->read8(&fmt) || !stream->read8(&pal))
                return false;
            if (pal != AIP_LOCALPALETTE && pal != AIP_NOPALETTE)
                return false;
            adjustOffset(off, adj);
            backdropOffset = off;
            format = fmt;
            paletteType = pal;
            found = true;
            break;
        }
        default:
            if (tag >= AK_ICON_NEW) {
                if (!stream->setPosition(size, SEEK_CUR))
                    return false;
            } else {
                return false;
            }
            break;
        }
    }

    AvbImageRef ref;
    ref.streamOffset = backdropOffset;
    ref.format = format;
    ref.paletteType = paletteType;
    ref.globalPalette = &palette;
    return loadAvbImage(stream, ref, &m_image);
}
