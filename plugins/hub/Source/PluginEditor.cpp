#include "PluginEditor.h"
#include "MixwaveTheme.h"

namespace
{
    constexpr int kWidth = 560;
    constexpr int kRowHeight = 22;
    constexpr int kMaxRows = 14;
    constexpr int kListTop = 128;
}

MixwaveHubEditor::MixwaveHubEditor (MixwaveHubProcessor& p)
    : juce::AudioProcessorEditor (&p),
      hubProcessor (p),
      mixAttachment (p.apvts, "mix_amount", mixSlider)
{
    mixSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    mixSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 70, 20);
    mixSlider.setTextValueSuffix (" %");
    mixSlider.setDoubleClickReturnValue (true, 100.0);
    addAndMakeVisible (mixSlider);

    setSize (kWidth, kListTop + kMaxRows * kRowHeight + 24);
    startTimerHz (15);
}

MixwaveHubEditor::~MixwaveHubEditor()
{
    stopTimer();
}

void MixwaveHubEditor::resized()
{
    mixSlider.setBounds (110, 76, kWidth - 130, 24);
}

void MixwaveHubEditor::timerCallback()
{
    numTracks = hubProcessor.getLink().snapshotTracks (tracks.data(), (int) tracks.size());

    const float peak = hubProcessor.masterPeakDb.exchange (-120.0f, std::memory_order_relaxed);
    masterMeterDb = std::max (peak, masterMeterDb - 1.4f);

    repaint();
}

void MixwaveHubEditor::paint (juce::Graphics& g)
{
    using namespace mixwave::theme;

    g.fillAll (background);

    g.setColour (text);
    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawText ("MIXWAVE HUB", 20, 14, 300, 26, juce::Justification::centredLeft);

    const auto& link = hubProcessor.getLink();

    juce::String status;
    juce::Colour statusColour = textDim;

    if (! link.isConnected())
    {
        status = "Shared memory unavailable";
        statusColour = clip;
    }
    else if (link.isActiveHub())
    {
        status = "Active Hub  -  " + juce::String (link.liveTrackCount()) + " track(s) linked";
        statusColour = accent;
    }
    else
    {
        status = "Standby: another MIXWAVE Hub is already active in this project";
        statusColour = warn;
    }

    g.setFont (juce::FontOptions (13.0f));
    g.setColour (statusColour);
    g.drawText (status, 20, 44, kWidth - 40, 20, juce::Justification::centredLeft);

    g.setColour (textDim);
    g.drawText ("Mix Amount", 20, 76, 90, 24, juce::Justification::centredLeft);

    // ---- track table -----------------------------------------------------------
    auto header = juce::Rectangle<int> (20, kListTop - 22, kWidth - 40, 20);
    g.setFont (juce::FontOptions (11.0f));
    g.setColour (textDim);
    g.drawText ("SLOT",  header.removeFromLeft (44),  juce::Justification::centredLeft);
    g.drawText ("TRACK", header.removeFromLeft (170), juce::Justification::centredLeft);
    g.drawText ("CH",    header.removeFromLeft (40),  juce::Justification::centredLeft);
    g.drawText ("RMS",   header.removeFromLeft (70),  juce::Justification::centredLeft);
    g.drawText ("PEAK",  header,                      juce::Justification::centredLeft);

    g.setColour (panel);
    g.fillRect (20, kListTop - 2, kWidth - 40, 1);

    if (numTracks == 0)
    {
        g.setColour (textDim);
        g.setFont (juce::FontOptions (13.0f));
        g.drawText ("No MIXWAVE Track instances found. Load MIXWAVE Track on your mixer inserts.",
                    20, kListTop + 8, kWidth - 40, 20, juce::Justification::centredLeft);
        return;
    }

    const int rows = std::min (numTracks, kMaxRows);

    for (int i = 0; i < rows; ++i)
    {
        const auto& t = tracks[(size_t) i];
        auto row = juce::Rectangle<int> (20, kListTop + i * kRowHeight, kWidth - 40, kRowHeight);

        if (i % 2 == 1)
        {
            g.setColour (panel.withAlpha (0.5f));
            g.fillRect (row);
        }

        const auto name = juce::String::fromUTF8 (t.name);
        const auto label = name.isNotEmpty() ? name : ("Track " + juce::String (t.slotIndex + 1));
        const auto channels = t.channels == 1 ? juce::String ("M") : juce::String ("S");

        g.setFont (juce::FontOptions (13.0f));
        g.setColour (textDim);
        g.drawText (juce::String (t.slotIndex + 1), row.removeFromLeft (44), juce::Justification::centredLeft);
        g.setColour (text);
        g.drawText (label, row.removeFromLeft (170), juce::Justification::centredLeft);
        g.setColour (textDim);
        g.drawText (channels, row.removeFromLeft (40), juce::Justification::centredLeft);
        g.drawText (juce::String (t.rmsDb, 1) + " dB", row.removeFromLeft (70), juce::Justification::centredLeft);

        auto bar = row.reduced (0, 6);
        g.setColour (background);
        g.fillRoundedRectangle (bar.toFloat(), 2.0f);
        g.setColour (levelColour (t.peakDb));
        g.fillRoundedRectangle (bar.withWidth ((int) (dbToFraction (t.peakDb) * (float) bar.getWidth())).toFloat(), 2.0f);
    }

    if (numTracks > rows)
    {
        g.setColour (textDim);
        g.drawText ("+ " + juce::String (numTracks - rows) + " more track(s)",
                    20, kListTop + rows * kRowHeight + 2, kWidth - 40, 18, juce::Justification::centredLeft);
    }
}
