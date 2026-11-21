#pragma once

#include "MixLinkTimer.h"
#include "ipc/SharedMixConnection.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

class MixwaveHubProcessor : public juce::AudioProcessor
{
public:
    MixwaveHubProcessor();
    ~MixwaveHubProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    mixwave::ipc::SharedMixConnection& getLink() noexcept { return link; }

    // "Mix Amount" is exposed now so automation/state can be tested; it controls
    // nothing yet (it will scale the Auto Mix correction in Phase 3).
    juce::AudioProcessorValueTreeState apvts;

    std::atomic<float> masterPeakDb { -120.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // The heartbeat timer must be destroyed BEFORE the link.
    mixwave::ipc::SharedMixConnection link;
    MixLinkTimer heartbeat;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixwaveHubProcessor)
};
