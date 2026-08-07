#pragma once

#include "avbformat.h"
#include "avbimage.h"
#include "emotions.h"

#include <QImage>
#include <QString>
#include <QVector>
#include <memory>

class AvbStream;
class AvbFileStream;

struct PoseLayerOffsets {
    quint32 offsets[3] = {0, 0, 0};
    quint8 formats[3] = {0, 0, 0};
    quint8 palettes[3] = {0, 0, 0};
};

class Pose
{
public:
    explicit Pose(const PoseLayerOffsets &layers);
    bool ensureLoaded(AvbStream *stream, const AvbPalette &globalPalette);

    QImage drawing() const { return m_drawing; }
    QImage mask() const { return m_mask; }
    QImage aura() const { return m_aura; }
    QImage composited() const;

private:
    PoseLayerOffsets m_layers;
    QImage m_drawing;
    QImage m_mask;
    QImage m_aura;
    bool m_loaded = false;
};

struct BodyRec {
    quint16 poseId = 0;
    float emotion = 0.f;
    float intensity = 0.f;
    quint8 faceX = 0;
    quint8 faceY = 0;
};

struct FaceRec {
    quint16 poseId = 0;
    float emotion = 0.f;
    float intensity = 0.f;
    qint16 xCX = 0;
    qint16 yCX = 0;
    qint16 deltaXCX = 0;
    qint16 deltaYCX = 0;
    quint8 faceX = 0;
    quint8 faceY = 0;
};

struct TorsoRec {
    quint16 poseId = 0;
    float emotion = 0.f;
    float intensity = 0.f;
    qint16 xCX = 0;
    qint16 yCX = 0;
};

enum class AvatarKind { Simple, Complex };

struct RenderedBody {
    QImage image;
    QRect bounds;
    QPoint faceTip; // for balloon arrows, in image coords
};

class Avatar
{
public:
    virtual ~Avatar();

    static std::unique_ptr<Avatar> load(AvbFileStream *stream);

    AvatarKind kind() const { return m_kind; }
    QString name() const { return m_name; }
    void setFileName(const QString &n) { m_fileName = n; }
    QString fileName() const { return m_fileName; }
    QString copyright() const { return m_copyright; }
    QString originalUrl() const { return m_originalUrl; }
    quint16 iconPoseId() const { return m_iconPose; }
    quint8 flags() const { return m_flags; }

    Pose *pose(quint16 poseId); // 1-based
    bool ensurePoseLoaded(quint16 poseId);
    QImage iconImage();

    virtual RenderedBody renderForEmotion(const Emotion &em) = 0;
    virtual void setNeutral() = 0;

    AvbFileStream *stream() const { return m_stream.get(); }

protected:
    Avatar(AvatarKind kind);
    bool handleCommonTag(AvbStream *stream, quint16 tag, quint16 size, qint32 &adj);
    quint16 createPose(const PoseLayerOffsets &layers);
    bool loadBodyRecs(AvbStream *stream, bool oldTag, qint32 &adj);
    bool loadFaceRecs(AvbStream *stream, bool oldTag, qint32 &adj);
    bool loadTorsoRecs(AvbStream *stream, bool oldTag, qint32 &adj);

    AvatarKind m_kind;
    QString m_name;
    QString m_fileName;
    QString m_copyright;
    QString m_originalUrl;
    QString m_overrideUrl;
    quint8 m_style = 0;
    quint8 m_flags = 0;
    quint16 m_iconPose = 0;
    AvbPalette m_palette;
    QVector<std::shared_ptr<Pose>> m_poses; // 0-based; poseId = index+1
    std::unique_ptr<AvbFileStream> m_stream;
};

class AvatarSimple : public Avatar
{
public:
    AvatarSimple();
    RenderedBody renderForEmotion(const Emotion &em) override;
    void setNeutral() override;
    bool handleTag(AvbStream *stream, quint16 tag, quint16 size, qint32 &adj);

    QVector<BodyRec> bodies;
    int lastBody = -1;
};

class AvatarComplex : public Avatar
{
public:
    AvatarComplex();
    RenderedBody renderForEmotion(const Emotion &em) override;
    void setNeutral() override;
    bool handleTag(AvbStream *stream, quint16 tag, quint16 size, qint32 &adj);

    QVector<FaceRec> faces;
    QVector<TorsoRec> torsos;
    int lastFace = -1;
    int lastTorso = -1;
};
