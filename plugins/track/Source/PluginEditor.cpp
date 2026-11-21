#include "PluginEditor.h"
#include "MixwaveTheme.h"

namespace
{
    const juce::Rectangle<int> inMeterBounds  { 24, 104, 18, 150 };
    const juce::Rectangle<int> outMeterBounds { 298, 104, 18, 150 };
}

MixwaveTrackEditor::MixwaveTrackEditor (MixwaveTrackProcessor& p)
    : juce::AudioProcessorEditor (&p),
      trackProcessor (p),
      gainAttachment (p.apvts, "gain", gainSlider)
{
    gainSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    gainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 22);
    gainSlider.setTextValueSuffix (" dB");
    gainSlider.setDoubleClickReturnValue (true, 0.0);
    addAndMakeVisible (gainSlider);

    setSize (340, 290);
    startTimerHz (30);
}

MixwaveTrackEditor::~MixwaveTrackEditor()
{
    stopTimer();
}

void MixwaveTrackEditor::resized()
{
    gainSlider.setBounds (78, 100, 184, 160);
}

void MixwaveTrackEditor::timerCallback()
{
    // Read-and-reset the peak, then apply a simple falloff (~20 dB/s).
    const float inPeak  = trackProcessor.inPeakDb.exchange  (-120.0f, std::memory_order_relaxed);
    const float outPeak = trackProcessor.outPeakDb.exchange (-120.0f, std::memory_order_relaxed);

    inMeterDb  = std::max (inPeak,  inMeterDb  - 0.7f);
    outMeterDb = std::max (outPeak, outMeterDb - 0.7f);

    repaint();
}

void MixwaveTrackEditor::paint (juce::Graphics& g)
{
    using namespace mixwave::theme;

    g.fillAll (background);

    g.setColour (text);
    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawText ("MIXWAVE TRACK", 20, 14, 300, 26, juce::Justification::centredLeft);

    const auto& link = trackProcessor.getLink();

    juce::String linkLine;
    if (! link.isConnected())
        linkLine = "Link: shared memory unavailable";
    else if (link.trackSlot() < 0)
        linkLine = "Link: no free track slot (64 max)";
    else
        linkLine = "Link: track slot " + juce::String (link.trackSlot() + 1);

    const auto hubLine = link.isHubPresent() ? juce::String ("Hub: connected") : juce::String ("Hub: not found");

    const auto name = juce::String::fromUTF8 (link.getCachedName().c_str());
    const auto nameLine = name.isNotEmpty() ? ("Track name: " + name)
                                            : juce::String ("Track name: (not provided by host)");

    g.setFont (juce::FontOptions (13.0f));
    g.setColour (link.isHubPresent() ? accent : textDim);
    g.drawText (hubLine, 20, 44, 300, 18, juce::Justification::centredLeft);
    g.setColour (textDim);
    g.drawText (linkLine, 20, 62, 300, 18, juce::Justification::centredLeft);
    g.drawText (nameLine, 20, 80, 300, 18, juce::Justification::centredLeft);

    drawVerticalMeter (g, inMeterBounds,  inMeterDb);
    drawVerticalMeter (g, outMeterBounds, outMeterDb);

    g.setColour (textDim);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("IN",  inMeterBounds.withY  (inMeterBounds.getBottom()  + 4).withHeight (14), juce::Justification::centred);
    g.drawText ("OUT", outMeterBounds.withY (outMeterBounds.getBottom() + 4).withHeight (14), juce::Justification::centred);
}
