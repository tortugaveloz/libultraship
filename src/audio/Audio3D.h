#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>

// OpenAL 3D support is available when OpenAL Soft is present
// This is defined when building with OpenAL support
#if defined(__APPLE__) || defined(ENABLE_OPENAL)
#define AUDIO3D_OPENAL_SUPPORT 1
#include <AL/al.h>
#include <AL/alc.h>
#endif

namespace Ship {

/**
 * Audio3DManager - Manages 3D positional audio sources
 * 
 * When using OpenAL backend, provides true 3D spatialization.
 * For other backends, provides stub implementations.
 */
class Audio3DManager {
public:
    static Audio3DManager* GetInstance();
    static void DestroyInstance();

    bool Init();
    void Shutdown();
    bool IsAvailable() const { return mInitialized && mHas3DSupport; }

    // Global settings
    void SetAttenuationModel(int model);
    void SetSpeedOfSound(float speed);
    void SetDopplerFactor(float factor);

    // Listener
    void SetListenerPosition(float x, float y, float z);
    void SetListenerOrientation(float atX, float atY, float atZ, float upX, float upY, float upZ);
    void SetListenerVelocity(float vx, float vy, float vz);
    void SetListenerGain(float gain);

    // Source management
    uint32_t CreateSource();
    void DestroySource(uint32_t sourceId);
    void DestroyAllSources();

    // Source properties
    void SetSourcePosition(uint32_t sourceId, float x, float y, float z);
    void SetSourceVelocity(uint32_t sourceId, float vx, float vy, float vz);
    void SetSourceGain(uint32_t sourceId, float gain);
    void SetSourcePitch(uint32_t sourceId, float pitch);
    void SetSourceLooping(uint32_t sourceId, bool looping);
    void SetSourceReferenceDistance(uint32_t sourceId, float distance);
    void SetSourceMaxDistance(uint32_t sourceId, float distance);
    void SetSourceRolloff(uint32_t sourceId, float rolloff);
    void SetSourceRelative(uint32_t sourceId, bool relative);

    // Playback
    void QueueBuffer(uint32_t sourceId, const void* data, uint32_t size, uint32_t sampleRate, uint8_t channels);
    void PlaySource(uint32_t sourceId);
    void PauseSource(uint32_t sourceId);
    void StopSource(uint32_t sourceId);
    int GetSourceState(uint32_t sourceId);
    bool IsSourceFinished(uint32_t sourceId);

    // Convenience
    uint32_t PlayAtPosition(const void* data, uint32_t size, uint32_t sampleRate,
                            float x, float y, float z, float gain, float pitch);
    void Update();

private:
    Audio3DManager() = default;
    ~Audio3DManager();

    static Audio3DManager* sInstance;
    static std::mutex sInstanceMutex;

    bool mInitialized = false;
    bool mHas3DSupport = false;
    bool mOwnsContext = false;  // Whether we created the context ourselves

#ifdef AUDIO3D_OPENAL_SUPPORT
    // OpenAL-specific members
    ALCdevice* mDevice = nullptr;
    ALCcontext* mContext = nullptr;
    
    struct Source3D {
        ALuint alSource = 0;
        std::vector<ALuint> buffers;
        bool isOneShot = false;
        bool isActive = false;
    };
    
    std::unordered_map<uint32_t, Source3D> mSources;
    uint32_t mNextSourceId = 1;
    std::mutex mSourceMutex;
    
    // Helper to get OpenAL format
    ALenum GetALFormat(uint8_t channels);
    
    // Reclaim finished buffers from a source
    void ReclaimBuffers(Source3D& source);
#endif
};

} // namespace Ship
