#include "emotions.h"

#include <QString>

namespace {

const float kEmFloats[] = {
    0.f,
    float(0 * 2 * M_PI / 8), // happy
    float(1 * 2 * M_PI / 8),
    float(2 * 2 * M_PI / 8),
    float(3 * 2 * M_PI / 8),
    float(4 * 2 * M_PI / 8),
    float(5 * 2 * M_PI / 8),
    float(6 * 2 * M_PI / 8),
    float(7 * 2 * M_PI / 8),
    0.f, // neutral
    EM_WAVE,
    EM_POINTOTHER,
    EM_POINTSELF,
    EM_DOUBLEPOINT,
    EM_SHRUG,
    EM_3QRWALK,
    EM_SIDEWALK,
    EM_3QFWALK,
};

} // namespace

float emotionToFloat(int index)
{
    const int n = int(sizeof(kEmFloats) / sizeof(kEmFloats[0]));
    if (index < 0 || index >= n)
        return 0.f;
    return kEmFloats[index];
}

float subtractAngles(float a, float b)
{
    float d = a - b;
    while (d > float(M_PI))
        d -= float(2 * M_PI);
    while (d < float(-M_PI))
        d += float(2 * M_PI);
    return d;
}

Emotion emotionFromId(EmotionId id, float intensity)
{
    switch (id) {
    case EmotionId::Happy:  return {intensity, emHappy()};
    case EmotionId::Coy:    return {intensity, emCoy()};
    case EmotionId::Bored:  return {intensity, emBored()};
    case EmotionId::Scared: return {intensity, emScared()};
    case EmotionId::Sad:    return {intensity, emSad()};
    case EmotionId::Angry:  return {intensity, emAngry()};
    case EmotionId::Shout:  return {intensity, emShout()};
    case EmotionId::Laugh:  return {intensity, emLaugh()};
    case EmotionId::Neutral:
    default:
        return {0.f, emNeutral()};
    }
}

QString emotionName(EmotionId id)
{
    switch (id) {
    case EmotionId::Happy: return QStringLiteral("Happy");
    case EmotionId::Coy: return QStringLiteral("Coy");
    case EmotionId::Bored: return QStringLiteral("Bored");
    case EmotionId::Scared: return QStringLiteral("Scared");
    case EmotionId::Sad: return QStringLiteral("Sad");
    case EmotionId::Angry: return QStringLiteral("Angry");
    case EmotionId::Shout: return QStringLiteral("Shout");
    case EmotionId::Laugh: return QStringLiteral("Laugh");
    case EmotionId::Neutral:
    default:
        return QStringLiteral("Neutral");
    }
}
