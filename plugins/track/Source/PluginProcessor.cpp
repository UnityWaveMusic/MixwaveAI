#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

juce::AudioProcessorValueTreeState::ParameterLayout MixwaveTrackProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "gain", 1 },
        "Gain",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    return layout;
}

MixwaveTrackProcessor::MixwaveTrackProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MIXWAVE_TRACK", createLayout()),
      heartbeat ([this] { link.heartbeatTrack(); })
{
    gainDb = apvts.getRawParameterValue ("gain");

    link.claimTrackSlot();
    link.setChannels (2);
}

MixwaveTrackProcessor::~MixwaveTrackProcessor() = default;   // ~link releases the slot

void MixwaveTrackProcessor::prepareToPlay (double sampleRate, int)
{
    gain.reset (sampleRate, 0.02);
    gain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (gainDb->load()));

    link.setChannels (getTotalNumInputChannels());
}

bool MixwaveTrackProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == out;
}

void MixwaveTrackProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    if (numChannels == 0 || numSamples == 0)
        return;

    // ---- analysis of the incoming signal (pre-gain) --------------------------
    float peak = 0.0f;
    float sumSquares = 0.0f;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* d = buffer.getReadPointer (ch);
        for (int i = 0; i < numSamples; ++i)
        {
            const float v = d[i];
            peak = std::max (peak, std::abs (v));
            sumSquares += v * v;
        }
    }

    const float rmsDb  = juce::Decibels::gainToDecibels (std::sqrt (sumSquares / (float) (numSamples * numChannels)), -120.0f);
    const float peakDb = juce::Decibels::gainToDecibels (peak, -120.0f);

    link.publishLevels (rmsDb, peakDb);   // wait-free, audio-thread safe

    // ---- processing: smoothed gain (the only DSP in Phase 1) -----------------
    gain.setTargetValue (juce::Decibels::decibelsToGain (gainDb->load()));

    if (gain.isSmoothing())
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float g = gain.getNextValue();
            for (int ch = 0; ch < numChannels; ++ch)
                buffer.getWritePointer (ch)[i] *= g;
        }
    }
    else
    {
        buffer.applyGain (gain.getTargetValue());
    }

    // ---- meters for the editor ----------------------------------------------
    const float outDb = juce::Decibels::gainToDecibels (buffer.getMagnitude (0, numSamples), -120.0f);
    inPeakDb.store  (std::max (inPeakDb.load  (std::memory_order_relaxed), peakDb), std::memory_order_relaxed);
    outPeakDb.store (std::max (outPeakDb.load (std::memory_order_relaxed), outDb),  std::memory_order_relaxed);
}

juce::AudioProcessorEditor* MixwaveTrackProcessor::createEditor()
{
    return new MixwaveTrackEditor (*this);
}

void MixwaveTrackProcessor::updateTrackProperties (const TrackProperties& properties)
{
    if (properties.name.has_value())
        link.setName (properties.name->toStdString());
}

void MixwaveTrackProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void MixwaveTrackProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MixwaveTrackProcessor();
}
