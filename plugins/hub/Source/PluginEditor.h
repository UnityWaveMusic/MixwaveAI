#pragma once

#include "PluginProcessor.h"

#include <array>
#include <juce_audio_processors/juce_audio_processors.h>

class MixwaveHubEditor : public juce::AudioProcessorEditor,
                         private juce::Timer
{
public:
    explicit MixwaveHubEditor (MixwaveHubProcessor&);
    ~MixwaveHubEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    MixwaveHubProcessor& hubProcessor;

    juce::Slider mixSlider;
    juce::AudioProcessorValueTreeState::SliderAttachment mixAttachment;

    std::array<mixwave::ipc::TrackSnapshot, mixwave::ipc::kMaxTracks> tracks;
    int numTracks = 0;
    float masterMeterDb = -120.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixwaveHubEditor)
};
