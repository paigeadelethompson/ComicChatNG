#pragma once

#include "emotions.h"

#include <QString>

namespace EmotionRules {

// Map message text to an emotion using Comic Chat-style keyword heuristics.
Emotion analyze(const QString &text);

EmotionId analyzeId(const QString &text);

} // namespace EmotionRules
