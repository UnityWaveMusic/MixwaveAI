#pragma once

#include <juce_graphics/juce_graphics.h>

// Minimal Phase-1 look (dark charcoal). The full interface comes in Phase 8.
namespace mixwave::theme
{
    inline const juce::Colour background { 0xff1b1d21 };
    inline const juce::Colour panel      { 0xff25282d };
    inline const juce::Colour text       { 0xffe6e8eb };
    inline const juce::Colour textDim    { 0xff8d939b };
    inline const juce::Colour accent     { 0xff3fb6a8 };
    inline const juce::Colour warn       { 0xffe0a93b };
    inline const juce::Colour clip       { 0xffd9544d };

    inline juce::Colour levelColour (float db)
    {
        if (db > -1.0f)  return clip;
        if (db > -9.0f)  return warn;
        return accent;
    }

    inline float dbToFraction (float db)
    {
        return juce::jmap (juce::jlimit (-60.0f, 0.0f, db), -60.0f, 0.0f, 0.0f, 1.0f);
    }

    inline void drawVerticalMeter (juce::Graphics& g, juce::Rectangle<int> r, float db)
    {
        g.setColour (panel);
        g.fillRoundedRectangle (r.toFloat(), 3.0f);

        const auto h = (int) std::round (dbToFraction (db) * (float) r.getHeight());
        g.setColour (levelColour (db));
        g.fillRoundedRectangle (r.withTop (r.getBottom() - h).toFloat(), 3.0f);
    }
}
