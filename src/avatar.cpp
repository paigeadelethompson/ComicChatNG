#include "avatar.h"
#include "avbstream.h"

#include <QPainter>
#include <cstdio>
#include <cstring>

Pose::Pose(const PoseLayerOffsets &layers)
    : m_layers(layers)
{
}

QImage Pose::composited() const
{
    return applyComicMask(m_drawing, m_mask);
}

bool Pose::ensureLoaded(AvbStream *stream, const AvbPalette &globalPalette)
{
    if (m_loaded)
        return true;
    if (!stream || !stream->open())
        return false;

    QImage layers[3];
    for (int i = 0; i < 3; ++i) {
        if (m_layers.offsets[i] == 0)
            continue;
        AvbImageRef ref;
        ref.streamOffset = m_layers.offsets[i];
        ref.format = m_layers.formats[i];
        ref.paletteType = m_layers.palettes[i];
        ref.globalPalette = &globalPalette;
        if (!loadAvbImage(stream, ref, &layers[i])) {
            stream->close();
            return false;
        }
    }
    stream->close();

    if (m_layers.palettes[0] == AIP_MASKEDMONO && !layers[0].isNull()) {
        if (!convertMaskedMono(layers[0], &m_drawing, &m_mask, &m_aura))
            return false;
    } else if (m_layers.palettes[1] == AIP_DUALMASK && !layers[1].isNull()) {
        m_drawing = layers[0];
        if (!convertDualMask(layers[1], &m_mask, &m_aura))
            return false;
    } else {
        m_drawing = layers[0];
        m_mask = layers[1];
        m_aura = layers[2];
    }

    m_loaded = true;
    return true;
}

Avatar::Avatar(AvatarKind kind)
    : m_kind(kind)
{
}

Avatar::~Avatar() = default;

Pose *Avatar::pose(quint16 poseId)
{
    if (poseId == 0 || poseId > m_poses.size())
        return nullptr;
    return m_poses[poseId - 1].get();
}

bool Avatar::ensurePoseLoaded(quint16 poseId)
{
    Pose *p = pose(poseId);
    if (!p || !m_stream)
        return false;
    return p->ensureLoaded(m_stream.get(), m_palette);
}

QImage Avatar::iconImage()
{
    if (!ensurePoseLoaded(m_iconPose)) {
        // Fallback: first pose
        if (!m_poses.isEmpty() && ensurePoseLoaded(1))
            return m_poses[0]->composited();
        return {};
    }
    return pose(m_iconPose)->composited();
}

quint16 Avatar::createPose(const PoseLayerOffsets &layers)
{
    m_poses.append(std::make_shared<Pose>(layers));
    return static_cast<quint16>(m_poses.size());
}

bool Avatar::handleCommonTag(AvbStream *stream, quint16 tag, quint16 size, qint32 &adj)
{
    switch (tag) {
    case AK_NAME: {
        QByteArray s;
        if (!stream->readString(&s, 60))
            return false;
        m_name = QString::fromLatin1(s);
        return true;
    }
    case AK_ORIGINAL_URL: {
        QByteArray s;
        if (!stream->readString(&s, 512))
            return false;
        m_originalUrl = QString::fromLatin1(s);
        return true;
    }
    case AK_OVERRIDE_URL: {
        QByteArray s;
        if (!stream->readString(&s, 512))
            return false;
        m_overrideUrl = QString::fromLatin1(s);
        return true;
    }
    case AK_COPYRIGHT: {
        QByteArray s;
        if (!stream->readString(&s, 256))
            return false;
        m_copyright = QString::fromLatin1(s);
        return true;
    }
    case AK_STYLE: {
        quint16 style = 0;
        if (!stream->read16(&style))
            return false;
        m_style = static_cast<quint8>(style);
        return true;
    }
    case AK_FLAGS: {
        quint16 flags = 0;
        if (!stream->read16(&flags))
            return false;
        m_flags = static_cast<quint8>(flags);
        return true;
    }
    case AK_ICON:
    case AK_ICON_NEW: {
        AvbIconData icon {};
        if (tag == AK_ICON) {
            if (!stream->read32(&icon.offset))
                return false;
            icon.format = AIF_DIB;
            icon.palette = AIP_NOPALETTE;
        } else {
            if (stream->read(&icon, sizeof(icon)) != sizeof(icon))
                return false;
        }
        adjustOffset(icon.offset, adj);
        PoseLayerOffsets layers;
        layers.offsets[0] = icon.offset;
        layers.formats[0] = icon.format;
        layers.palettes[0] = icon.palette;
        m_iconPose = createPose(layers);
        return m_iconPose != INVALID_POSE_ID;
    }
    case AK_COLORPALETTE:
        return m_palette.read(stream);
    case AK_OFFSET_ADJUSTMENT: {
        qint32 value = 0;
        if (!stream->read32(reinterpret_cast<quint32 *>(&value)))
            return false;
        adj += value;
        return true;
    }
    default:
        if (tag >= AK_ICON_NEW)
            return stream->setPosition(size, SEEK_CUR);
        return false;
    }
}

bool Avatar::loadBodyRecs(AvbStream *stream, bool oldTag, qint32 &adj)
{
    auto *self = dynamic_cast<AvatarSimple *>(this);
    if (!self)
        return false;

    quint16 count = 0;
    if (!stream->read16(&count))
        return false;

    self->bodies.resize(count);
    const quint32 recSize = oldTag ? sizeof(AvbBodyDataOld) : sizeof(AvbBodyDataNew);
    quint32 prevImage = 0;

    for (int i = 0; i < count; ++i) {
        AvbBodyDataNew data {};
        if (stream->read(&data, recSize) != recSize)
            return false;

        if (data.imageOffset != prevImage) {
            adjustOffset(data.imageOffset, adj);
            adjustOffset(data.maskOffset, adj);
            adjustOffset(data.auraOffset, adj);
            PoseLayerOffsets layers;
            layers.offsets[0] = data.imageOffset;
            layers.offsets[1] = data.maskOffset;
            layers.offsets[2] = data.auraOffset;
            layers.formats[0] = data.imageFormat;
            layers.formats[1] = data.maskFormat;
            layers.formats[2] = data.auraFormat;
            layers.palettes[0] = data.imagePalette;
            layers.palettes[1] = data.maskPalette;
            layers.palettes[2] = data.auraPalette;
            self->bodies[i].poseId = createPose(layers);
            if (self->bodies[i].poseId == INVALID_POSE_ID)
                return false;
            prevImage = data.imageOffset;
        } else {
            self->bodies[i].poseId = self->bodies[i - 1].poseId;
        }

        self->bodies[i].emotion = emotionToFloat(data.emotion);
        self->bodies[i].intensity = data.intensity / 255.f;
        self->bodies[i].faceX = static_cast<quint8>(data.x);
        self->bodies[i].faceY = static_cast<quint8>(data.y);
    }
    return true;
}

bool Avatar::loadFaceRecs(AvbStream *stream, bool oldTag, qint32 &adj)
{
    auto *self = dynamic_cast<AvatarComplex *>(this);
    if (!self)
        return false;

    quint16 count = 0;
    if (!stream->read16(&count))
        return false;

    self->faces.resize(count);
    const quint32 recSize = oldTag ? sizeof(AvbFaceDataOld) : sizeof(AvbFaceDataNew);
    quint32 prevImage = 0;

    for (int i = 0; i < count; ++i) {
        AvbFaceDataNew data {};
        if (stream->read(&data, recSize) != recSize)
            return false;

        if (data.imageOffset != prevImage) {
            adjustOffset(data.imageOffset, adj);
            adjustOffset(data.maskOffset, adj);
            adjustOffset(data.auraOffset, adj);
            PoseLayerOffsets layers;
            layers.offsets[0] = data.imageOffset;
            layers.offsets[1] = data.maskOffset;
            layers.offsets[2] = data.auraOffset;
            layers.formats[0] = data.imageFormat;
            layers.formats[1] = data.maskFormat;
            layers.formats[2] = data.auraFormat;
            layers.palettes[0] = data.imagePalette;
            layers.palettes[1] = data.maskPalette;
            layers.palettes[2] = data.auraPalette;
            self->faces[i].poseId = createPose(layers);
            if (self->faces[i].poseId == INVALID_POSE_ID)
                return false;
            prevImage = data.imageOffset;
        } else {
            self->faces[i].poseId = self->faces[i - 1].poseId;
        }

        self->faces[i].emotion = emotionToFloat(data.emotion);
        self->faces[i].intensity = data.intensity / 255.f;
        self->faces[i].xCX = static_cast<qint16>(data.cx);
        self->faces[i].yCX = static_cast<qint16>(data.cy);
        self->faces[i].deltaXCX = static_cast<qint16>(data.cxDelta);
        self->faces[i].deltaYCX = static_cast<qint16>(data.cyDelta);
        self->faces[i].faceX = static_cast<quint8>(data.x);
        self->faces[i].faceY = static_cast<quint8>(data.y);
    }
    return true;
}

bool Avatar::loadTorsoRecs(AvbStream *stream, bool oldTag, qint32 &adj)
{
    auto *self = dynamic_cast<AvatarComplex *>(this);
    if (!self)
        return false;

    quint16 count = 0;
    if (!stream->read16(&count))
        return false;

    self->torsos.resize(count);
    const quint32 recSize = oldTag ? sizeof(AvbTorsoDataOld) : sizeof(AvbTorsoDataNew);
    quint32 prevImage = 0;

    for (int i = 0; i < count; ++i) {
        AvbTorsoDataNew data {};
        if (stream->read(&data, recSize) != recSize)
            return false;

        if (data.imageOffset != prevImage) {
            adjustOffset(data.imageOffset, adj);
            adjustOffset(data.maskOffset, adj);
            adjustOffset(data.auraOffset, adj);
            PoseLayerOffsets layers;
            layers.offsets[0] = data.imageOffset;
            layers.offsets[1] = data.maskOffset;
            layers.offsets[2] = data.auraOffset;
            layers.formats[0] = data.imageFormat;
            layers.formats[1] = data.maskFormat;
            layers.formats[2] = data.auraFormat;
            layers.palettes[0] = data.imagePalette;
            layers.palettes[1] = data.maskPalette;
            layers.palettes[2] = data.auraPalette;
            self->torsos[i].poseId = createPose(layers);
            if (self->torsos[i].poseId == INVALID_POSE_ID)
                return false;
            prevImage = data.imageOffset;
        } else {
            self->torsos[i].poseId = self->torsos[i - 1].poseId;
        }

        self->torsos[i].emotion = emotionToFloat(data.emotion);
        self->torsos[i].intensity = data.intensity / 255.f;
        self->torsos[i].xCX = static_cast<qint16>(data.cx);
        self->torsos[i].yCX = static_cast<qint16>(data.cy);
    }
    return true;
}

std::unique_ptr<Avatar> Avatar::load(AvbFileStream *ownedStream)
{
    if (!ownedStream || !ownedStream->open())
        return nullptr;

    AvbHeader hdr {};
    if (ownedStream->read(&hdr, sizeof(hdr)) != sizeof(hdr)) {
        ownedStream->close();
        return nullptr;
    }

    if (hdr.magic != AF_MAGICNUM && hdr.magic != AF_MAGICNUM_NEW) {
        ownedStream->close();
        return nullptr;
    }

    // Major version in HIWORD of 16-bit version field must be 0.
    if ((hdr.version >> 8) != 0) {
        ownedStream->close();
        return nullptr;
    }

    std::unique_ptr<Avatar> avatar;
    if (hdr.type == AT_COMPLEX)
        avatar.reset(new AvatarComplex);
    else if (hdr.type == AT_SIMPLE)
        avatar.reset(new AvatarSimple);
    else {
        ownedStream->close();
        return nullptr;
    }

    qint32 adj = 0;
    while (true) {
        quint16 tag = 0;
        if (!ownedStream->read16(&tag)) {
            ownedStream->close();
            return nullptr;
        }
        quint16 size = 0;
        if (tag >= AK_ICON_NEW && !ownedStream->read16(&size)) {
            ownedStream->close();
            return nullptr;
        }
        if (tag == AK_STARTDATA)
            break;

        bool ok = false;
        if (auto *c = dynamic_cast<AvatarComplex *>(avatar.get()))
            ok = c->handleTag(ownedStream, tag, size, adj);
        else if (auto *s = dynamic_cast<AvatarSimple *>(avatar.get()))
            ok = s->handleTag(ownedStream, tag, size, adj);

        if (!ok) {
            ownedStream->close();
            return nullptr;
        }
    }

    ownedStream->close();
    avatar->m_stream.reset(ownedStream);
    return avatar;
}

AvatarSimple::AvatarSimple()
    : Avatar(AvatarKind::Simple)
{
}

bool AvatarSimple::handleTag(AvbStream *stream, quint16 tag, quint16 size, qint32 &adj)
{
    if (tag == AK_NBODIES || tag == AK_NBODIES2)
        return loadBodyRecs(stream, tag == AK_NBODIES, adj);
    return handleCommonTag(stream, tag, size, adj);
}

void AvatarSimple::setNeutral()
{
    lastBody = 0;
    for (int i = 0; i < bodies.size(); ++i) {
        if (bodies[i].emotion == emNeutral() && bodies[i].intensity == 0.f) {
            lastBody = i;
            return;
        }
    }
}

RenderedBody AvatarSimple::renderForEmotion(const Emotion &em)
{
    RenderedBody out;
    int nearest = -1;
    double bestIntensity = 2.0;
    for (int i = 0; i < bodies.size(); ++i) {
        const int index = (lastBody + 1 + i) % bodies.size();
        if (bodies[index].emotion > 7.f && bodies[index].emotion < 100.f)
            continue;
        const double angle = qAbs(subtractAngles(bodies[index].emotion, em.emotion));
        const bool firstNeutral = bodies[index].emotion == emNeutral()
            && bodies[index].intensity == 0.f && nearest < 0;
        if (angle < M_PI / NEMOTIONS || firstNeutral) {
            double di = firstNeutral && em.intensity > 0.f
                ? 1.5
                : qAbs(em.intensity - bodies[index].intensity);
            if (di < bestIntensity) {
                bestIntensity = di;
                nearest = index;
            }
        }
    }
    if (nearest < 0)
        nearest = bodies.isEmpty() ? -1 : 0;
    if (nearest < 0)
        return out;

    lastBody = nearest;
    const BodyRec &rec = bodies[nearest];
    if (!ensurePoseLoaded(rec.poseId))
        return out;
    Pose *p = pose(rec.poseId);
    out.image = p->composited();
    out.bounds = out.image.rect();
    out.faceTip = QPoint(rec.faceX, rec.faceY);
    return out;
}

AvatarComplex::AvatarComplex()
    : Avatar(AvatarKind::Complex)
{
}

bool AvatarComplex::handleTag(AvbStream *stream, quint16 tag, quint16 size, qint32 &adj)
{
    if (tag == AK_NFACES || tag == AK_NFACES2)
        return loadFaceRecs(stream, tag == AK_NFACES, adj);
    if (tag == AK_NTORSOS || tag == AK_NTORSOS2)
        return loadTorsoRecs(stream, tag == AK_NTORSOS, adj);
    return handleCommonTag(stream, tag, size, adj);
}

void AvatarComplex::setNeutral()
{
    lastFace = 0;
    lastTorso = 0;
    for (int i = 0; i < faces.size(); ++i) {
        if (faces[i].emotion == emNeutral() && faces[i].intensity == 0.f) {
            lastFace = i;
            break;
        }
    }
    for (int i = 0; i < torsos.size(); ++i) {
        if (torsos[i].emotion == emNeutral() && torsos[i].intensity == 0.f) {
            lastTorso = i;
            break;
        }
    }
}

RenderedBody AvatarComplex::renderForEmotion(const Emotion &em)
{
    RenderedBody out;
    if (faces.isEmpty() || torsos.isEmpty())
        return out;

    int faceIdx = 0;
    double nearestAngle = 3 * M_PI;
    double bestI = 2.0;
    for (int i = 0; i < faces.size(); ++i) {
        const double angle = qAbs(subtractAngles(faces[i].emotion, em.emotion));
        const double di = qAbs(em.intensity - faces[i].intensity);
        if (angle < nearestAngle || (angle == nearestAngle && di < bestI)) {
            nearestAngle = angle;
            bestI = di;
            faceIdx = i;
        }
    }

    int torsoIdx = 0;
    bestI = 2.0;
    for (int i = 0; i < torsos.size(); ++i) {
        const int index = (lastTorso + 1 + i) % torsos.size();
        if (torsos[index].emotion > 7.f && torsos[index].emotion < 100.f)
            continue;
        const double angle = qAbs(subtractAngles(torsos[index].emotion, em.emotion));
        if (angle < M_PI / NEMOTIONS
            || (torsos[index].emotion == emNeutral() && torsos[index].intensity == 0.f)) {
            const double di = qAbs(em.intensity - torsos[index].intensity);
            if (di < bestI) {
                bestI = di;
                torsoIdx = index;
            }
        }
    }

    lastFace = faceIdx;
    lastTorso = torsoIdx;

    const FaceRec &face = faces[faceIdx];
    const TorsoRec &torso = torsos[torsoIdx];
    if (!ensurePoseLoaded(face.poseId) || !ensurePoseLoaded(torso.poseId))
        return out;

    QImage head = pose(face.poseId)->composited();
    QImage body = pose(torso.poseId)->composited();
    if (head.isNull() || body.isNull())
        return out;

    // Align head onto torso using registration points (xCX/yCX).
    const QPoint torsoAnchor(torso.xCX, torso.yCX);
    const QPoint headAnchor(face.xCX, face.yCX);

    const QPoint headTopLeft = torsoAnchor - headAnchor;
    QRect unionRect = body.rect().united(QRect(headTopLeft, head.size()));
    QImage canvas(unionRect.size(), QImage::Format_ARGB32);
    canvas.fill(Qt::transparent);

    QPainter painter(&canvas);
    const QPoint bodyPos = -unionRect.topLeft();
    const QPoint headPos = headTopLeft - unionRect.topLeft();

    const bool torsoFirst = (m_flags & 4) != 0; // TORSOFIRST
    if (torsoFirst) {
        painter.drawImage(bodyPos, body);
        painter.drawImage(headPos, head);
    } else {
        painter.drawImage(headPos, head);
        painter.drawImage(bodyPos, body);
    }
    painter.end();

    out.image = canvas;
    out.bounds = canvas.rect();
    out.faceTip = headPos + QPoint(face.faceX, face.faceY);
    return out;
}
