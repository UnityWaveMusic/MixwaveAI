#pragma once

#include "MixLinkTimer.h"
#include "ipc/SharedMixConnection.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

class MixwaveTrackProcessor : public juce::AudioProcessor
{
public:
    MixwaveTrackProcessor();
    ~MixwaveTrackProcessor() override;

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

    // Called by hosts that report the mixer-track name/colour (FL Studio may or may not).
    void updateTrackProperties (const TrackProperties& properties) override;

    mixwave::ipc::SharedMixConnection& getLink() noexcept { return link; }

    juce::AudioProcessorValueTreeState apvts;

    // Peak meters for the editor (audio thread writes, editor reads and resets).
    std::atomic<float> inPeakDb  { -120.0f };
    std::atomic<float> outPeakDb { -120.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    std::atomic<float>* gainDb = nullptr;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> gain;

    // Declaration order matters: the heartbeat timer must be destroyed BEFORE the link.
    mixwave::ipc::SharedMixConnection link;
    MixLinkTimer heartbeat;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixwaveTrackProcessor)
};
