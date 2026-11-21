#pragma once

#include "SharedMemoryRegion.h"
#include "SharedMixTypes.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

namespace mixwave::ipc
{
    // Copy of one live Track slot, taken by the Hub (never points into shared memory).
    struct TrackSnapshot
    {
        int slotIndex = -1;
        std::uint64_t instanceId = 0;
        float rmsDb = -120.0f;
        float peakDb = -120.0f;
        std::int32_t role = 0;
        std::uint32_t channels = 0;
        char name[kNameWords * 4] = {};
    };

    // One object per plugin instance. A Track instance uses the *Track* methods,
    // the Hub instance uses the *Hub* methods.
    //
    // Threading:
    //   * publishLevels() is the ONLY method meant for the audio thread. It is
    //     wait-free (a few atomic loads/stores, no locks, no allocation).
    //   * Everything else takes a mutex and must be called from the message
    //     thread / a timer / prepareToPlay - never from processBlock().
    class SharedMixConnection
    {
    public:
        explicit SharedMixConnection (std::int64_t staleTimeoutMs = kStaleTimeoutMs);
        ~SharedMixConnection();

        SharedMixConnection (const SharedMixConnection&) = delete;
        SharedMixConnection& operator= (const SharedMixConnection&) = delete;

        bool isConnected() const noexcept { return state != nullptr; }

        // ---- Track side -------------------------------------------------------
        int  claimTrackSlot();                       // returns slot index or -1
        void releaseTrackSlot();
        bool heartbeatTrack();                       // keeps the slot alive; re-claims if it was lost
        void setChannels (int numChannels);
        void setName (const std::string& utf8Name);
        void publishLevels (float rmsDb, float peakDb) noexcept;   // audio-thread safe

        int  trackSlot() const noexcept { return slot.load(); }
        std::string getCachedName() const;
        bool isHubPresent() const noexcept;

        // ---- Hub side ---------------------------------------------------------
        bool claimHub();                             // true if this instance is the active Hub
        void releaseHub();
        bool heartbeatHub();                         // keeps/claims Hub ownership; true if active
        bool isActiveHub() const noexcept { return hubOwned.load(); }
        int  liveTrackCount() const noexcept;
        int  snapshotTracks (TrackSnapshot* out, int maxOut) const noexcept;

        static std::int64_t nowMs() noexcept;

    private:
        int  claimTrackSlotLocked();
        bool claimHubLocked();
        void writeInfoLocked();
        bool isFresh (std::int64_t heartbeat, std::int64_t now) const noexcept;

        SharedMemoryRegion region;
        SharedMixState* state = nullptr;
        std::int64_t staleTimeoutMs;

        std::uint64_t instanceId = 0;
        std::atomic<int> slot { -1 };
        std::atomic<bool> hubOwned { false };

        mutable std::mutex mutex;
        int cachedChannels = 0;
        std::string cachedName;
    };
}
