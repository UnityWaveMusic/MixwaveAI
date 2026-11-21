#pragma once

#include "PluginProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>

class MixwaveTrackEditor : public juce::AudioProcessorEditor,
                           private juce::Timer
{
public:
    explicit MixwaveTrackEditor (MixwaveTrackProcessor&);
    ~MixwaveTrackEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    MixwaveTrackProcessor& trackProcessor;

    juce::Slider gainSlider;
    juce::AudioProcessorValueTreeState::SliderAttachment gainAttachment;

    float inMeterDb  = -120.0f;
    float outMeterDb = -120.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixwaveTrackEditor)
};
