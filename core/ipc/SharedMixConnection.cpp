#include "SharedMixConnection.h"

#ifndef NOMINMAX
 #define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
 #define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <algorithm>
#include <chrono>
#include <cstring>

namespace mixwave::ipc
{
    namespace
    {
        // One shared-memory block per host PROCESS. Two FL Studio instances (or two
        // projects open at once) therefore never see each other's tracks.
        // "Local\" = visible inside the current logon session only.
        std::wstring makeMappingName()
        {
            return L"Local\\MixwaveAI_v1_" + std::to_wstring (static_cast<unsigned long> (GetCurrentProcessId()));
        }

        std::uint64_t makeInstanceId()
        {
            static std::atomic<std::uint32_t> counter { 0 };
            const auto pid = static_cast<std::uint64_t> (GetCurrentProcessId());
            return (pid << 32) | static_cast<std::uint64_t> (counter.fetch_add (1) + 1u);   // never 0
        }
    }

    std::int64_t SharedMixConnection::nowMs() noexcept
    {
        using namespace std::chrono;
        return duration_cast<milliseconds> (steady_clock::now().time_since_epoch()).count();
    }

    SharedMixConnection::SharedMixConnection (std::int64_t staleMs)
        : staleTimeoutMs (staleMs)
    {
        if (! region.openOrCreate (makeMappingName(), sizeof (SharedMixState)))
            return;

        auto* s = static_cast<SharedMixState*> (region.data());

        std::uint32_t expectedMagic = 0;
        s->magic.compare_exchange_strong (expectedMagic, kMagic);

        std::uint32_t expectedSize = 0;
        s->structSize.compare_exchange_strong (expectedSize, static_cast<std::uint32_t> (sizeof (SharedMixState)));

        if (s->magic.load() == kMagic && s->structSize.load() == sizeof (SharedMixState))
            state = s;
    }

    SharedMixConnection::~SharedMixConnection()
    {
        releaseTrackSlot();
        releaseHub();
    }

    bool SharedMixConnection::isFresh (std::int64_t heartbeat, std::int64_t now) const noexcept
    {
        return heartbeat != 0 && (now - heartbeat) <= staleTimeoutMs;
    }

    // ------------------------------------------------------------------ Track

    int SharedMixConnection::claimTrackSlot()
    {
        std::lock_guard<std::mutex> lock (mutex);
        return claimTrackSlotLocked();
    }

    int SharedMixConnection::claimTrackSlotLocked()
    {
        if (state == nullptr)
            return -1;

        if (slot.load() >= 0)
            return slot.load();

        if (instanceId == 0)
            instanceId = makeInstanceId();

        const auto now = nowMs();

        // Pass 0: take a free slot. Pass 1: take over a slot whose owner stopped
        // sending heartbeats (crashed or unloaded without cleaning up).
        for (int pass = 0; pass < 2; ++pass)
        {
            for (int i = 0; i < kMaxTracks; ++i)
            {
                auto& s = state->tracks[i];
                std::uint64_t current = s.instanceId.load();

                if (pass == 0)
                {
                    if (current != 0)
                        continue;
                }
                else
                {
                    const auto hb = s.heartbeatMs.load();
                    if (current == 0 || hb == 0 || isFresh (hb, now))
                        continue;
                }

                if (! s.instanceId.compare_exchange_strong (current, instanceId))
                    continue;

                // We own the slot. Heartbeat stays 0 ("claim in progress") until
                // the slot is fully initialised, so readers ignore it meanwhile.
                s.heartbeatMs.store (0);
                s.rmsDb.store (-120.0f);
                s.peakDb.store (-120.0f);
                s.role.store (static_cast<std::int32_t> (TrackRole::Unknown));
                s.channels.store (0);
                for (auto& w : s.nameWords)
                    w.store (0);

                slot.store (i);
                writeInfoLocked();
                s.heartbeatMs.store (nowMs());
                return i;
            }
        }

        return -1;
    }

    void SharedMixConnection::releaseTrackSlot()
    {
        std::lock_guard<std::mutex> lock (mutex);

        const int i = slot.exchange (-1);
        if (state == nullptr || i < 0)
            return;

        auto& s = state->tracks[i];
        std::uint64_t expected = instanceId;
        if (s.instanceId.load() == instanceId)
        {
            s.heartbeatMs.store (0);
            s.instanceId.compare_exchange_strong (expected, 0);
        }
    }

    bool SharedMixConnection::heartbeatTrack()
    {
        std::lock_guard<std::mutex> lock (mutex);

        if (state == nullptr)
            return false;

        const int i = slot.load();
        if (i >= 0 && state->tracks[i].instanceId.load() == instanceId)
        {
            state->tracks[i].heartbeatMs.store (nowMs());
            return true;
        }

        // Slot was taken over (e.g. our message thread stalled for > timeout) or never claimed.
        slot.store (-1);
        return claimTrackSlotLocked() >= 0;
    }

    void SharedMixConnection::setChannels (int numChannels)
    {
        std::lock_guard<std::mutex> lock (mutex);
        cachedChannels = numChannels;
        writeInfoLocked();
    }

    void SharedMixConnection::setName (const std::string& utf8Name)
    {
        std::lock_guard<std::mutex> lock (mutex);
        cachedName = utf8Name;
        writeInfoLocked();
    }

    std::string SharedMixConnection::getCachedName() const
    {
        std::lock_guard<std::mutex> lock (mutex);
        return cachedName;
    }

    void SharedMixConnection::writeInfoLocked()
    {
        const int i = slot.load();
        if (state == nullptr || i < 0)
            return;

        auto& s = state->tracks[i];
        s.channels.store (static_cast<std::uint32_t> (std::max (0, cachedChannels)));

        char buf[kNameWords * 4] = {};
        std::memcpy (buf, cachedName.data(), std::min (cachedName.size(), sizeof (buf) - 1));

        for (int w = 0; w < kNameWords; ++w)
        {
            std::uint32_t word = 0;
            std::memcpy (&word, buf + w * 4, 4);
            s.nameWords[w].store (word);
        }
    }

    void SharedMixConnection::publishLevels (float rmsDb, float peakDb) noexcept
    {
        const int i = slot.load (std::memory_order_acquire);
        if (i < 0)
            return;

        auto& s = state->tracks[i];
        if (s.instanceId.load (std::memory_order_relaxed) != instanceId)
            return;   // slot was taken over; do not write into someone else's data

        s.rmsDb.store (rmsDb, std::memory_order_relaxed);
        s.peakDb.store (peakDb, std::memory_order_relaxed);
    }

    bool SharedMixConnection::isHubPresent() const noexcept
    {
        if (state == nullptr)
            return false;

        return state->hubInstanceId.load() != 0 && isFresh (state->hubHeartbeatMs.load(), nowMs());
    }

    // -------------------------------------------------------------------- Hub

    bool SharedMixConnection::claimHub()
    {
        std::lock_guard<std::mutex> lock (mutex);
        return claimHubLocked();
    }

    bool SharedMixConnection::claimHubLocked()
    {
        if (state == nullptr)
            return false;

        if (hubOwned.load())
            return true;

        if (instanceId == 0)
            instanceId = makeInstanceId();

        const auto now = nowMs();
        std::uint64_t current = state->hubInstanceId.load();
        const auto hb = state->hubHeartbeatMs.load();
        const bool stale = current != 0 && hb != 0 && ! isFresh (hb, now);

        if (current != 0 && ! stale)
            return false;   // another Hub is active

        if (! state->hubInstanceId.compare_exchange_strong (current, instanceId))
            return false;

        state->hubHeartbeatMs.store (now);
        hubOwned.store (true);
        return true;
    }

    void SharedMixConnection::releaseHub()
    {
        std::lock_guard<std::mutex> lock (mutex);

        if (state == nullptr || ! hubOwned.exchange (false))
            return;

        std::uint64_t expected = instanceId;
        if (state->hubInstanceId.load() == instanceId)
        {
            state->hubHeartbeatMs.store (0);
            state->hubInstanceId.compare_exchange_strong (expected, 0);
        }
    }

    bool SharedMixConnection::heartbeatHub()
    {
        std::lock_guard<std::mutex> lock (mutex);

        if (state == nullptr)
            return false;

        if (hubOwned.load())
        {
            if (state->hubInstanceId.load() == instanceId)
            {
                state->hubHeartbeatMs.store (nowMs());
                return true;
            }

            hubOwned.store (false);   // ownership was taken over
        }

        return claimHubLocked();
    }

    int SharedMixConnection::liveTrackCount() const noexcept
    {
        if (state == nullptr)
            return 0;

        int count = 0;
        const auto now = nowMs();

        for (int i = 0; i < kMaxTracks; ++i)
        {
            const auto& s = state->tracks[i];
            if (s.instanceId.load() != 0 && isFresh (s.heartbeatMs.load(), now))
                ++count;
        }

        return count;
    }

    int SharedMixConnection::snapshotTracks (TrackSnapshot* out, int maxOut) const noexcept
    {
        if (state == nullptr || out == nullptr)
            return 0;

        int n = 0;
        const auto now = nowMs();

        for (int i = 0; i < kMaxTracks && n < maxOut; ++i)
        {
            const auto& s = state->tracks[i];
            const auto id = s.instanceId.load();

            if (id == 0 || ! isFresh (s.heartbeatMs.load(), now))
                continue;

            TrackSnapshot& o = out[n++];
            o.slotIndex = i;
            o.instanceId = id;
            o.rmsDb = s.rmsDb.load (std::memory_order_relaxed);
            o.peakDb = s.peakDb.load (std::memory_order_relaxed);
            o.role = s.role.load();
            o.channels = s.channels.load();

            for (int w = 0; w < kNameWords; ++w)
            {
                const std::uint32_t word = s.nameWords[w].load (std::memory_order_relaxed);
                std::memcpy (o.name + w * 4, &word, 4);
            }
            o.name[sizeof (o.name) - 1] = '\0';
        }

        return n;
    }
}
