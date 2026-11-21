// Phase 1 checkpoint test for the Track <-> Hub shared-memory link.
// Plain C++ (no JUCE). Each SharedMixConnection below behaves exactly like one
// plugin instance loaded in the same host process.

#include "ipc/SharedMixConnection.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <memory>
#include <thread>
#include <vector>

using namespace mixwave::ipc;

static int failures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (! (cond)) {                                                      \
            std::printf ("FAIL (line %d): %s\n", __LINE__, #cond);           \
            ++failures;                                                      \
        }                                                                    \
    } while (0)

static void testBasicTrackHubFlow()
{
    std::printf ("test: basic Track/Hub flow\n");

    SharedMixConnection trackA, trackB, hub1, hub2;
    CHECK (trackA.isConnected());

    CHECK (trackA.claimTrackSlot() == 0);
    CHECK (trackB.claimTrackSlot() == 1);
    CHECK (trackA.claimTrackSlot() == 0);          // claiming twice keeps the same slot

    CHECK (! trackA.isHubPresent());
    CHECK (hub1.claimHub());
    CHECK (! hub2.claimHub());                     // only one active Hub
    CHECK (hub1.isActiveHub());
    CHECK (! hub2.isActiveHub());
    CHECK (trackA.isHubPresent());

    trackA.setChannels (2);
    trackA.setName ("Kick");
    trackB.setChannels (1);
    trackB.setName ("Bass");
    trackA.publishLevels (-18.0f, -6.0f);
    trackB.publishLevels (-24.0f, -12.0f);

    CHECK (hub1.liveTrackCount() == 2);

    TrackSnapshot snap[kMaxTracks];
    const int n = hub1.snapshotTracks (snap, kMaxTracks);
    CHECK (n == 2);
    if (n == 2)
    {
        CHECK (snap[0].slotIndex == 0);
        CHECK (std::strcmp (snap[0].name, "Kick") == 0);
        CHECK (snap[0].channels == 2);
        CHECK (snap[0].rmsDb == -18.0f);
        CHECK (snap[0].peakDb == -6.0f);
        CHECK (snap[1].slotIndex == 1);
        CHECK (std::strcmp (snap[1].name, "Bass") == 0);
        CHECK (snap[1].channels == 1);
        CHECK (snap[1].peakDb == -12.0f);
    }

    trackB.releaseTrackSlot();
    CHECK (hub1.liveTrackCount() == 1);

    // A new Track reuses the freed slot.
    SharedMixConnection trackC;
    CHECK (trackC.claimTrackSlot() == 1);

    // Standby Hub takes over once the active one is released.
    hub1.releaseHub();
    CHECK (! trackA.isHubPresent());
    CHECK (hub2.heartbeatHub());
    CHECK (hub2.isActiveHub());
    CHECK (trackA.isHubPresent());
}

static void testFullTableAndStaleTakeover()
{
    std::printf ("test: full slot table and stale-slot takeover\n");

    constexpr std::int64_t timeoutMs = 50;
    std::vector<std::unique_ptr<SharedMixConnection>> tracks;

    for (int i = 0; i < kMaxTracks; ++i)
    {
        tracks.push_back (std::make_unique<SharedMixConnection> (timeoutMs));
        CHECK (tracks.back()->claimTrackSlot() == i);
    }

    SharedMixConnection extra (timeoutMs);
    CHECK (extra.claimTrackSlot() == -1);          // table full, everyone alive

    std::this_thread::sleep_for (std::chrono::milliseconds (150));   // everybody goes stale

    // Everybody except instance 7 keeps sending heartbeats ("instance 7 crashed").
    for (int i = 0; i < kMaxTracks; ++i)
        if (i != 7)
            CHECK (tracks[(size_t) i]->heartbeatTrack());

    CHECK (extra.claimTrackSlot() == 7);           // takes over the dead slot
    CHECK (! tracks[7]->heartbeatTrack());         // the old owner cannot get it back

    tracks[7]->publishLevels (-1.0f, -1.0f);       // must NOT overwrite the new owner's data
    TrackSnapshot snap[kMaxTracks];
    const int n = extra.snapshotTracks (snap, kMaxTracks);
    bool found = false;
    for (int i = 0; i < n; ++i)
        if (snap[i].slotIndex == 7)
        {
            found = true;
            CHECK (snap[i].peakDb == -120.0f);
        }
    CHECK (found);
}

int main()
{
    testBasicTrackHubFlow();
    testFullTableAndStaleTakeover();

    if (failures == 0)
        std::printf ("ALL TESTS PASSED\n");
    else
        std::printf ("%d CHECK(S) FAILED\n", failures);

    return failures == 0 ? 0 : 1;
}
