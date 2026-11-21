#pragma once

#include <atomic>
#include <cstdint>

// Layout of the block of memory shared by every MIXWAVE plugin instance that
// lives in the same host process. Rules for this file:
//   * Plain fixed-size data only (no pointers, no std::string, no virtuals).
//   * Every field that is touched by more than one thread/instance is atomic.
//   * An all-zero block is the valid "empty" state (Windows zero-fills new
//     file mappings).
//   * Changing the layout means bumping the version byte in kMagic.
namespace mixwave::ipc
{
    constexpr std::uint32_t kMagic = 0x4D575601u;   // 'M' 'W' 'V' + layout version 1
    constexpr int kMaxTracks = 64;
    constexpr int kNameWords = 12;                  // 48 bytes of UTF-8, zero padded
    constexpr std::int64_t kStaleTimeoutMs = 2500;  // slot is dead if no heartbeat for this long

    enum class TrackRole : std::int32_t
    {
        Unknown = 0, Kick, Bass, Drums, Percussion, Synth, Lead, Pad, Vocal, Fx
    };

    struct TrackSlot
    {
        std::atomic<std::uint64_t> instanceId;      // 0 = slot free
        std::atomic<std::int64_t>  heartbeatMs;     // 0 = claim in progress / released
        std::atomic<float>         rmsDb;           // pre-gain block RMS (Phase 2 replaces with LUFS)
        std::atomic<float>         peakDb;          // pre-gain block peak
        std::atomic<std::int32_t>  role;            // TrackRole (used from Phase 3)
        std::atomic<std::uint32_t> channels;        // 1 = mono, 2 = stereo
        std::atomic<std::uint32_t> nameWords[kNameWords];
    };

    struct SharedMixState
    {
        std::atomic<std::uint32_t> magic;
        std::atomic<std::uint32_t> structSize;
        std::atomic<std::uint64_t> hubInstanceId;   // 0 = no Hub
        std::atomic<std::int64_t>  hubHeartbeatMs;
        TrackSlot tracks[kMaxTracks];
    };

    static_assert (std::atomic<std::uint32_t>::is_always_lock_free);
    static_assert (std::atomic<std::uint64_t>::is_always_lock_free);
    static_assert (std::atomic<std::int64_t>::is_always_lock_free);
    static_assert (std::atomic<float>::is_always_lock_free);
}
