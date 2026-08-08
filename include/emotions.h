#pragma once

#include <QString>
#include <QtMath>

struct Emotion {
  float intensity = 0.f;
  float emotion = 0.f;

  Emotion() = default;
  Emotion(float inten, float emo) : intensity(inten), emotion(emo) {}
};

constexpr int NEMOTIONS = 8;

inline float emHappy() { return float(0 * 2 * M_PI / 8); }
inline float emCoy() { return float(1 * 2 * M_PI / 8); }
inline float emBored() { return float(2 * 2 * M_PI / 8); }
inline float emScared() { return float(3 * 2 * M_PI / 8); }
inline float emSad() { return float(4 * 2 * M_PI / 8); }
inline float emAngry() { return float(5 * 2 * M_PI / 8); }
inline float emShout() { return float(6 * 2 * M_PI / 8); }
inline float emLaugh() { return float(7 * 2 * M_PI / 8); }
inline float emNeutral() { return 0.f; }

constexpr float EM_WAVE = 1001.f;
constexpr float EM_POINTOTHER = 1002.f;
constexpr float EM_POINTSELF = 1003.f;
constexpr float EM_DOUBLEPOINT = 1004.f;
constexpr float EM_SHRUG = 1005.f;
constexpr float EM_3QRWALK = 1006.f;
constexpr float EM_SIDEWALK = 1007.f;
constexpr float EM_3QFWALK = 1008.f;

float emotionToFloat(int index);
float subtractAngles(float a, float b);

enum class EmotionId {
  Neutral = 0,
  Happy,
  Coy,
  Bored,
  Scared,
  Sad,
  Angry,
  Shout,
  Laugh,
};

Emotion emotionFromId(EmotionId id, float intensity = 0.5f);
QString emotionName(EmotionId id);
