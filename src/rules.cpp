#include "rules.h"

namespace EmotionRules {

  EmotionId analyzeId(const QString &text) {
    const QString t = text.toLower();

    if (t.contains(QLatin1String("lol")) || t.contains(QLatin1String("haha")) ||
        t.contains(QLatin1String("heh")) || t.contains(QStringLiteral("😂")) ||
        t.contains(QLatin1String("rofl")))
      return EmotionId::Laugh;

    if (t.contains(QLatin1Char('!')))
      return EmotionId::Shout;

    if (t.contains(QLatin1String("angry")) ||
        t.contains(QLatin1String("hate")) || t.contains(QLatin1String("grr")) ||
        t.contains(QLatin1String("damn")))
      return EmotionId::Angry;

    if (t.contains(QLatin1String("sad")) || t.contains(QLatin1String("cry")) ||
        t.contains(QLatin1String(":( ")) || t.contains(QLatin1String(":-(")) ||
        t.contains(QLatin1String("miss you")))
      return EmotionId::Sad;

    if (t.contains(QLatin1String("scare")) ||
        t.contains(QLatin1String("afraid")) ||
        t.contains(QLatin1String("eek")) || t.contains(QLatin1String("help")))
      return EmotionId::Scared;

    if (t.contains(QLatin1String("bored")) ||
        t.contains(QLatin1String("meh")) ||
        t.contains(QLatin1String("whatever")))
      return EmotionId::Bored;

    if (t.contains(QLatin1String("love")) ||
        t.contains(QLatin1String("cute")) || t.contains(QLatin1String(":)")) ||
        t.contains(QLatin1String(":-)")) ||
        t.contains(QLatin1String("happy")) || t.contains(QLatin1String("yay")))
      return EmotionId::Happy;

    if (t.contains(QLatin1String("hmm")) || t.contains(QLatin1String("coy")) ||
        t.contains(QLatin1String(";)")))
      return EmotionId::Coy;

    // Intensity from punctuation / caps
    return EmotionId::Neutral;
  }

  Emotion analyze(const QString &text) {
    const EmotionId id = analyzeId(text);
    float intensity = 0.35f;
    if (text.contains(QLatin1Char('!')))
      intensity = 0.75f;
    if (text == text.toUpper() && text.size() > 3)
      intensity = 0.9f;
    if (id == EmotionId::Neutral)
      intensity = 0.f;
    return emotionFromId(id, intensity);
  }

} // namespace EmotionRules
