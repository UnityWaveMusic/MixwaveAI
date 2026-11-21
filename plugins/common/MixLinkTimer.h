#pragma once

#include <juce_events/juce_events.h>
#include <functional>

// Message-thread timer that keeps a plugin instance's heartbeat alive in the
// shared-memory link, even while the plugin editor is closed.
class MixLinkTimer : private juce::Timer
{
public:
    explicit MixLinkTimer (std::function<void()> onTick, int intervalMs = 500)
        : callback (std::move (onTick))
    {
        startTimer (intervalMs);
    }

    ~MixLinkTimer() override { stopTimer(); }

private:
    void timerCallback() override { if (callback) callback(); }

    std::function<void()> callback;
};
