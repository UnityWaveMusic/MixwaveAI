#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>

juce::AudioProcessorValueTreeState::ParameterLayout MixwaveHubProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix_amount", 1 },
        "Mix Amount",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        100.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    return layout;
}

MixwaveHubProcessor::MixwaveHubProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MIXWAVE_HUB", createLayout()),
      heartbeat ([this] { link.heartbeatHub(); })
{
    link.claimHub();
}

MixwaveHubProcessor::~MixwaveHubProcessor() = default;   // ~link releases Hub ownership

void MixwaveHubProcessor::prepareToPlay (double, int) {}

bool MixwaveHubProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == out;
}

void MixwaveHubProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Phase 1: pure pass-through (the buffer is left untouched) plus a master peak meter.
    if (buffer.getNumSamples() == 0 || buffer.getNumChannels() == 0)
        return;

    const float db = juce::Decibels::gainToDecibels (buffer.getMagnitude (0, buffer.getNumSamples()), -120.0f);
    masterPeakDb.store (std::max (masterPeakDb.load (std::memory_order_relaxed), db), std::memory_order_relaxed);
}

juce::AudioProcessorEditor* MixwaveHubProcessor::createEditor()
{
    return new MixwaveHubEditor (*this);
}

void MixwaveHubProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void MixwaveHubProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MixwaveHubProcessor();
}
