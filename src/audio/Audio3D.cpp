#include "Audio3D.h"
#include <spdlog/spdlog.h>
#include <cstring>

#ifdef AUDIO3D_OPENAL_SUPPORT
#include <AL/alext.h>
#endif

namespace Ship {

Audio3DManager* Audio3DManager::sInstance = nullptr;
std::mutex Audio3DManager::sInstanceMutex;

Audio3DManager* Audio3DManager::GetInstance() {
    std::lock_guard<std::mutex> lock(sInstanceMutex);
    if (sInstance == nullptr) {
        sInstance = new Audio3DManager();
    }
    return sInstance;
}

void Audio3DManager::DestroyInstance() {
    std::lock_guard<std::mutex> lock(sInstanceMutex);
    if (sInstance != nullptr) {
        sInstance->Shutdown();
        delete sInstance;
        sInstance = nullptr;
    }
}

Audio3DManager::~Audio3DManager() {
    Shutdown();
}

bool Audio3DManager::Init() {
    if (mInitialized) {
        return mHas3DSupport;
    }

#ifdef AUDIO3D_OPENAL_SUPPORT
    // Check if there's already an active OpenAL context (e.g., from OpenALAudioPlayer)
    ALCcontext* existingContext = alcGetCurrentContext();
    if (existingContext != nullptr) {
        // Reuse the existing context - don't create a new one
        mContext = existingContext;
        mDevice = alcGetContextsDevice(mContext);
        mOwnsContext = false;  // We don't own this context
        
        // Set default distance model
        alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
        
        const ALCchar* deviceName = alcGetString(mDevice, ALC_DEVICE_SPECIFIER);
        SPDLOG_INFO("Audio3D: Reusing existing OpenAL context (device: \"{}\")", 
                    deviceName ? deviceName : "unknown");

        mInitialized = true;
        mHas3DSupport = true;
        return true;
    }

    // No existing context, create our own
    mDevice = alcOpenDevice(nullptr);
    if (mDevice == nullptr) {
        SPDLOG_WARN("Audio3D: Could not open OpenAL device for 3D audio");
        mInitialized = true;
        mHas3DSupport = false;
        return false;
    }

    // Create context
    mContext = alcCreateContext(mDevice, nullptr);
    if (mContext == nullptr) {
        SPDLOG_WARN("Audio3D: Could not create OpenAL context for 3D audio");
        alcCloseDevice(mDevice);
        mDevice = nullptr;
        mInitialized = true;
        mHas3DSupport = false;
        return false;
    }

    if (alcMakeContextCurrent(mContext) == ALC_FALSE) {
        SPDLOG_WARN("Audio3D: Could not make OpenAL context current");
        alcDestroyContext(mContext);
        alcCloseDevice(mDevice);
        mContext = nullptr;
        mDevice = nullptr;
        mInitialized = true;
        mHas3DSupport = false;
        return false;
    }

    // Set default distance model
    alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
    
    // Log device info
    const ALCchar* deviceName = alcGetString(mDevice, ALC_DEVICE_SPECIFIER);
    SPDLOG_INFO("Audio3D: Initialized with device \"{}\"", deviceName ? deviceName : "unknown");

    mInitialized = true;
    mHas3DSupport = true;
    mOwnsContext = true;  // We created this context
    return true;
#else
    // No 3D audio support on this platform
    mInitialized = true;
    mHas3DSupport = false;
    SPDLOG_INFO("Audio3D: 3D audio not available on this platform");
    return false;
#endif
}

void Audio3DManager::Shutdown() {
    if (!mInitialized) {
        return;
    }

#ifdef AUDIO3D_OPENAL_SUPPORT
    DestroyAllSources();

    // Only destroy the context/device if we created them
    if (mOwnsContext) {
        if (mContext != nullptr) {
            alcMakeContextCurrent(nullptr);
            alcDestroyContext(mContext);
        }

        if (mDevice != nullptr) {
            alcCloseDevice(mDevice);
        }
    }
    
    mContext = nullptr;
    mDevice = nullptr;
#endif

    mInitialized = false;
    mHas3DSupport = false;
    mOwnsContext = false;
}

void Audio3DManager::SetAttenuationModel(int model) {
#ifdef AUDIO3D_OPENAL_SUPPORT
    if (!mHas3DSupport) return;
    
    ALenum alModel;
    switch (model) {
        case 0: alModel = AL_NONE; break;
        case 1: alModel = AL_INVERSE_DISTANCE; break;
        case 2: alModel = AL_INVERSE_DISTANCE_CLAMPED; break;
        case 3: alModel = AL_LINEAR_DISTANCE; break;
        case 4: alModel = AL_LINEAR_DISTANCE_CLAMPED; break;
        case 5: alModel = AL_EXPONENT_DISTANCE; break;
        case 6: alModel = AL_EXPONENT_DISTANCE_CLAMPED; break;
        default: alModel = AL_INVERSE_DISTANCE_CLAMPED; break;
    }
    alDistanceModel(alModel);
#endif
}

void Audio3DManager::SetSpeedOfSound(float speed) {
#ifdef AUDIO3D_OPENAL_SUPPORT
    if (!mHas3DSupport) return;
    alSpeedOfSound(speed);
#endif
}

void Audio3DManager::SetDopplerFactor(float factor) {
#ifdef AUDIO3D_OPENAL_SUPPORT
    if (!mHas3DSupport) return;
    alDopplerFactor(factor);
#endif
}

void Audio3DManager::SetListenerPosition(float x, float y, float z) {
#ifdef AUDIO3D_OPENAL_SUPPORT
    if (!mHas3DSupport) return;
    alListener3f(AL_POSITION, x, y, z);
#endif
}

void Audio3DManager::SetListenerOrientation(float atX, float atY, float atZ, 
                                             float upX, float upY, float upZ) {
#ifdef AUDIO3D_OPENAL_SUPPORT
    if (!mHas3DSupport) return;
    float orientation[6] = { atX, atY, atZ, upX, upY, upZ };
    alListenerfv(AL_ORIENTATION, orientation);
#endif
}

void Audio3DManager::SetListenerVelocity(float vx, float vy, float vz) {
#ifdef AUDIO3D_OPENAL_SUPPORT
    if (!mHas3DSupport) return;
    alListener3f(AL_VELOCITY, vx, vy, vz);
#endif
}

void Audio3DManager::SetListenerGain(float gain) {
#ifdef AUDIO3D_OPENAL_SUPPORT
    if (!mHas3DSupport) return;
    alListenerf(AL_GAIN, gain);
#endif
}

uint32_t Audio3DManager::CreateSource() {
#ifdef AUDIO3D_OPENAL_SUPPORT
    if (!mHas3DSupport) return 0;

    std::lock_guard<std::mutex> lock(mSourceMutex);
    
    ALuint alSource;
    alGenSources(1, &alSource);
    if (alGetError() != AL_NO_ERROR) {
        SPDLOG_ERROR("Audio3D: Failed to create source");
        return 0;
    }

    // Configure for 3D
    alSourcei(alSource, AL_SOURCE_RELATIVE, AL_FALSE);
    alSourcef(alSource, AL_ROLLOFF_FACTOR, 1.0f);
    alSourcef(alSource, AL_REFERENCE_DISTANCE, 100.0f);  // Reasonable default for game units
    alSourcef(alSource, AL_MAX_DISTANCE, 5000.0f);

    uint32_t sourceId = mNextSourceId++;
    Source3D source;
    source.alSource = alSource;
    source.isActive = true;
    mSources[sourceId] = source;

    return sourceId;
#else
    return 0;
#endif
}

void Audio3DManager::DestroySource(uint32_t sourceId) {
#ifdef AUDIO3D_OPENAL_SUPPORT
    if (!mHas3DSupport || sourceId == 0) return;

    std::lock_guard<std::mutex> lock(mSourceMutex);
    
    auto it = mSources.find(sourceId);
    if (it == mSources.end()) return;

    Source3D& source = it->second;
    
    alSourceStop(source.alSource);
    alSourcei(source.alSource, AL_BUFFER, 0);
    
    // Delete buffers
    if (!source.buffers.empty()) {
        alDeleteBuffers(static_cast<ALsizei>(source.buffers.size()), source.buffers.data());
    }
    
    alDeleteSources(1, &source.alSource);
    mSources.erase(it);
#endif
}

void Audio3DManager::DestroyAllSources() {
#ifdef AUDIO3D_OPENAL_SUPPORT
    if (!mHas3DSupport) return;

    std::lock_guard<std::mutex> lock(mSourceMutex);
    
    for (auto& pair : mSources) {
        Source3D& source = pair.second;
        alSourceStop(source.alSource);
        alSourcei(source.alSource, AL_BUFFER, 0);
        if (!source.buffers.empty()) {
            alDeleteBuffers(static_cast<ALsizei>(source.buffers.size()), source.buffers.data());
        }
        alDeleteSources(1, &source.alSource);
    }
    mSources.clear();
#endif
}

void Audio3DManager::SetSourcePosition(uint32_t sourceId, float x, float y, float z) {
#ifdef AUDIO3D_OPENAL_SUPPORT
    if (!mHas3DSupport || sourceId == 0) return;

    std::lock_guard<std::mutex> lock(mSourceMutex);
    auto it = mSources.find(sourceId);
    if (it == mSources.end()) return;
    
    alSource3f(it->second.alSource, AL_POSITION, x, y, z);
#endif
}

void Audio3DManager::SetSourceVelocity(uint32_t sourceId, float vx, float vy, float vz) {
#ifdef AUDIO3D_OPENAL_SUPPORT
    if (!mHas3DSupport || sourceId == 0) return;

    std::lock_guard<std::mutex> lock(mSourceMutex);
    auto it = mSources.find(sourceId);
    if (it == mSources.end()) return;
    
    alSource3f(it->second.alSource, AL_VELOCITY, vx, vy, vz);
#endif
}

void Audio3DManager::SetSourceGain(uint32_t sourceId, float gain) {
#ifdef AUDIO3D_OPENAL_SUPPORT
    if (!mHas3DSupport || sourceId == 0) return;

    std::lock_guard<std::mutex> lock(mSourceMutex);
    auto it = mSources.find(sourceId);
    if (it == mSources.end()) return;
    
    alSourcef(it->second.alSource, AL_GAIN, gain);
#endif
}

void Audio3DManager::SetSourcePitch(uint32_t sourceId, float pitch) {
#ifdef AUDIO3D_OPENAL_SUPPORT
    if (!mHas3DSupport || sourceId == 0) return;

    std::lock_guard<std::mutex> lock(mSourceMutex);
    auto it = mSources.find(sourceId);
    if (it == mSources.end()) return;
    
    alSourcef(it->second.alSource, AL_PITCH, pitch);
#endif
}

void Audio3DManager::SetSourceLooping(uint32_t sourceId, bool looping) {
#ifdef AUDIO3D_OPENAL_SUPPORT
    if (!mHas3DSupport || sourceId == 0) return;

    std::lock_guard<std::mutex> lock(mSourceMutex);
    auto it = mSources.find(sourceId);
    if (it == mSources.end()) return;
    
    alSourcei(it->second.alSource, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
#endif
}

void Audio3DManager::SetSourceReferenceDistance(uint32_t sourceId, float distance) {
#ifdef AUDIO3D_OPENAL_SUPPORT
    if (!mHas3DSupport || sourceId == 0) return;

    std::lock_guard<std::mutex> lock(mSourceMutex);
    auto it = mSources.find(sourceId);
    if (it == mSources.end()) return;
    
    alSourcef(it->second.alSource, AL_REFERENCE_DISTANCE, distance);
#endif
}

void Audio3DManager::SetSourceMaxDistance(uint32_t sourceId, float distance) {
#ifdef AUDIO3D_OPENAL_SUPPORT
    if (!mHas3DSupport || sourceId == 0) return;

    std::lock_guard<std::mutex> lock(mSourceMutex);
    auto it = mSources.find(sourceId);
    if (it == mSources.end()) return;
    
    alSourcef(it->second.alSource, AL_MAX_DISTANCE, distance);
#endif
}

void Audio3DManager::SetSourceRolloff(uint32_t sourceId, float rolloff) {
#ifdef AUDIO3D_OPENAL_SUPPORT
    if (!mHas3DSupport || sourceId == 0) return;

    std::lock_guard<std::mutex> lock(mSourceMutex);
    auto it = mSources.find(sourceId);
    if (it == mSources.end()) return;
    
    alSourcef(it->second.alSource, AL_ROLLOFF_FACTOR, rolloff);
#endif
}

void Audio3DManager::SetSourceRelative(uint32_t sourceId, bool relative) {
#ifdef AUDIO3D_OPENAL_SUPPORT
    if (!mHas3DSupport || sourceId == 0) return;

    std::lock_guard<std::mutex> lock(mSourceMutex);
    auto it = mSources.find(sourceId);
    if (it == mSources.end()) return;
    
    alSourcei(it->second.alSource, AL_SOURCE_RELATIVE, relative ? AL_TRUE : AL_FALSE);
#endif
}

#ifdef AUDIO3D_OPENAL_SUPPORT
ALenum Audio3DManager::GetALFormat(uint8_t channels) {
    if (channels == 1) {
        return AL_FORMAT_MONO16;
    }
    return AL_FORMAT_STEREO16;
}

void Audio3DManager::ReclaimBuffers(Source3D& source) {
    ALint processed = 0;
    alGetSourcei(source.alSource, AL_BUFFERS_PROCESSED, &processed);
    
    while (processed > 0) {
        ALuint buffer;
        alSourceUnqueueBuffers(source.alSource, 1, &buffer);
        alDeleteBuffers(1, &buffer);
        
        // Remove from our tracking
        auto it = std::find(source.buffers.begin(), source.buffers.end(), buffer);
        if (it != source.buffers.end()) {
            source.buffers.erase(it);
        }
        processed--;
    }
}
#endif

void Audio3DManager::QueueBuffer(uint32_t sourceId, const void* data, uint32_t size, 
                                  uint32_t sampleRate, uint8_t channels) {
#ifdef AUDIO3D_OPENAL_SUPPORT
    if (!mHas3DSupport || sourceId == 0 || data == nullptr || size == 0) return;

    std::lock_guard<std::mutex> lock(mSourceMutex);
    auto it = mSources.find(sourceId);
    if (it == mSources.end()) return;
    
    Source3D& source = it->second;
    
    // Reclaim any finished buffers first
    ReclaimBuffers(source);
    
    // Create new buffer
    ALuint buffer;
    alGenBuffers(1, &buffer);
    if (alGetError() != AL_NO_ERROR) {
        SPDLOG_ERROR("Audio3D: Failed to generate buffer");
        return;
    }
    
    // Fill buffer with data
    ALenum format = GetALFormat(channels);
    alBufferData(buffer, format, data, static_cast<ALsizei>(size), static_cast<ALsizei>(sampleRate));
    if (alGetError() != AL_NO_ERROR) {
        SPDLOG_ERROR("Audio3D: Failed to fill buffer with data");
        alDeleteBuffers(1, &buffer);
        return;
    }
    
    // Queue buffer on source
    alSourceQueueBuffers(source.alSource, 1, &buffer);
    if (alGetError() != AL_NO_ERROR) {
        SPDLOG_ERROR("Audio3D: Failed to queue buffer");
        alDeleteBuffers(1, &buffer);
        return;
    }
    
    source.buffers.push_back(buffer);
#endif
}

void Audio3DManager::PlaySource(uint32_t sourceId) {
#ifdef AUDIO3D_OPENAL_SUPPORT
    if (!mHas3DSupport || sourceId == 0) return;

    std::lock_guard<std::mutex> lock(mSourceMutex);
    auto it = mSources.find(sourceId);
    if (it == mSources.end()) return;
    
    alSourcePlay(it->second.alSource);
#endif
}

void Audio3DManager::PauseSource(uint32_t sourceId) {
#ifdef AUDIO3D_OPENAL_SUPPORT
    if (!mHas3DSupport || sourceId == 0) return;

    std::lock_guard<std::mutex> lock(mSourceMutex);
    auto it = mSources.find(sourceId);
    if (it == mSources.end()) return;
    
    alSourcePause(it->second.alSource);
#endif
}

void Audio3DManager::StopSource(uint32_t sourceId) {
#ifdef AUDIO3D_OPENAL_SUPPORT
    if (!mHas3DSupport || sourceId == 0) return;

    std::lock_guard<std::mutex> lock(mSourceMutex);
    auto it = mSources.find(sourceId);
    if (it == mSources.end()) return;
    
    Source3D& source = it->second;
    alSourceStop(source.alSource);
    
    // Clear queued buffers
    alSourcei(source.alSource, AL_BUFFER, 0);
    if (!source.buffers.empty()) {
        alDeleteBuffers(static_cast<ALsizei>(source.buffers.size()), source.buffers.data());
        source.buffers.clear();
    }
#endif
}

int Audio3DManager::GetSourceState(uint32_t sourceId) {
#ifdef AUDIO3D_OPENAL_SUPPORT
    if (!mHas3DSupport || sourceId == 0) return 0;

    std::lock_guard<std::mutex> lock(mSourceMutex);
    auto it = mSources.find(sourceId);
    if (it == mSources.end()) return 0;
    
    ALint state;
    alGetSourcei(it->second.alSource, AL_SOURCE_STATE, &state);
    
    switch (state) {
        case AL_PLAYING: return 1;
        case AL_PAUSED: return 2;
        default: return 0;
    }
#else
    return 0;
#endif
}

bool Audio3DManager::IsSourceFinished(uint32_t sourceId) {
#ifdef AUDIO3D_OPENAL_SUPPORT
    if (!mHas3DSupport || sourceId == 0) return true;

    std::lock_guard<std::mutex> lock(mSourceMutex);
    auto it = mSources.find(sourceId);
    if (it == mSources.end()) return true;
    
    ALint state;
    alGetSourcei(it->second.alSource, AL_SOURCE_STATE, &state);
    
    if (state == AL_STOPPED) {
        ALint queued;
        alGetSourcei(it->second.alSource, AL_BUFFERS_QUEUED, &queued);
        return queued == 0;
    }
    return false;
#else
    return true;
#endif
}

uint32_t Audio3DManager::PlayAtPosition(const void* data, uint32_t size, uint32_t sampleRate,
                                         float x, float y, float z, float gain, float pitch) {
#ifdef AUDIO3D_OPENAL_SUPPORT
    if (!mHas3DSupport || data == nullptr || size == 0) return 0;

    uint32_t sourceId = CreateSource();
    if (sourceId == 0) return 0;
    
    {
        std::lock_guard<std::mutex> lock(mSourceMutex);
        auto it = mSources.find(sourceId);
        if (it != mSources.end()) {
            it->second.isOneShot = true;
        }
    }
    
    SetSourcePosition(sourceId, x, y, z);
    SetSourceGain(sourceId, gain);
    SetSourcePitch(sourceId, pitch);
    
    // For one-shots, use a single buffer (not streaming)
    QueueBuffer(sourceId, data, size, sampleRate, 1);  // Mono for 3D
    PlaySource(sourceId);
    
    return sourceId;
#else
    return 0;
#endif
}

void Audio3DManager::Update() {
#ifdef AUDIO3D_OPENAL_SUPPORT
    if (!mHas3DSupport) return;

    std::lock_guard<std::mutex> lock(mSourceMutex);
    
    // Clean up finished one-shot sources
    std::vector<uint32_t> toRemove;
    
    for (auto& pair : mSources) {
        if (pair.second.isOneShot) {
            ALint state;
            alGetSourcei(pair.second.alSource, AL_SOURCE_STATE, &state);
            
            if (state == AL_STOPPED) {
                toRemove.push_back(pair.first);
            }
        }
    }
    
    // Remove outside the loop to avoid iterator invalidation
    for (uint32_t id : toRemove) {
        auto it = mSources.find(id);
        if (it != mSources.end()) {
            Source3D& source = it->second;
            alSourcei(source.alSource, AL_BUFFER, 0);
            if (!source.buffers.empty()) {
                alDeleteBuffers(static_cast<ALsizei>(source.buffers.size()), source.buffers.data());
            }
            alDeleteSources(1, &source.alSource);
            mSources.erase(it);
        }
    }
#endif
}

} // namespace Ship
