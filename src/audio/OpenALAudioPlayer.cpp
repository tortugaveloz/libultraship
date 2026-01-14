#include "OpenALAudioPlayer.h"
#include <spdlog/spdlog.h>
#include <AL/alext.h>

namespace Ship {

OpenALAudioPlayer::~OpenALAudioPlayer() {
    SPDLOG_TRACE("destruct OpenAL audio player");
    
    if (mSource != 0) {
        alSourceStop(mSource);
        alSourcei(mSource, AL_BUFFER, 0);  // Detach buffers
        alDeleteSources(1, &mSource);
    }
    
    alDeleteBuffers(NUM_OPENAL_BUFFERS, mBuffers);
    
    if (mContext != nullptr) {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(mContext);
    }
    
    if (mDevice != nullptr) {
        alcCloseDevice(mDevice);
    }
}

ALenum OpenALAudioPlayer::GetOpenALFormat() {
    // Determine the appropriate OpenAL format based on channel configuration
    // We use 16-bit signed integer samples (S16) to match the game's audio format
    
    // Note: 5.1 surround (AL_FORMAT_51CHN16) requires specific channel ordering
    // and may not work reliably on all systems even with AL_EXT_MCFORMATS.
    // For now, we use stereo which is universally supported.
    // TODO: Investigate proper 5.1 support with correct channel ordering
    
    if (GetAudioChannels() == AudioChannelsSetting::audioSurround51) {
        // Check if 5.1 surround is actually supported and working
        if (alIsExtensionPresent("AL_EXT_MCFORMATS")) {
            // Test if AL_FORMAT_51CHN16 is actually valid by checking if
            // alGetEnumValue returns a non-zero value
            ALenum format51 = alGetEnumValue("AL_FORMAT_51CHN16");
            if (format51 != 0 && format51 != AL_INVALID_ENUM) {
                // Try to create a small test buffer to verify the format works
                ALuint testBuffer;
                alGenBuffers(1, &testBuffer);
                if (alGetError() == AL_NO_ERROR) {
                    // Small silent test data (6 channels * 2 bytes * 8 samples)
                    int16_t testData[6 * 8] = {0};
                    alBufferData(testBuffer, format51, testData, sizeof(testData), 32000);
                    ALenum error = alGetError();
                    alDeleteBuffers(1, &testBuffer);
                    
                    if (error == AL_NO_ERROR) {
                        mNumChannels = 6;
                        SPDLOG_INFO("OpenAL: Using 5.1 surround format (0x{:X})", format51);
                        return format51;
                    } else {
                        SPDLOG_WARN("OpenAL: 5.1 format test failed (error: 0x{:X}), falling back to stereo", error);
                    }
                }
            }
        }
        SPDLOG_WARN("OpenAL: 5.1 surround not available, using stereo");
    }
    
    mNumChannels = 2;
    return AL_FORMAT_STEREO16;
}

bool OpenALAudioPlayer::DoInit() {
    SPDLOG_INFO("OpenAL AudioPlayer: Starting initialization");
    
    // Open the default audio device
    mDevice = alcOpenDevice(nullptr);
    if (mDevice == nullptr) {
        SPDLOG_ERROR("OpenAL: Failed to open audio device");
        return false;
    }
    
    // Log the device name
    const ALCchar* deviceName = nullptr;
    if (alcIsExtensionPresent(mDevice, "ALC_ENUMERATE_ALL_EXT")) {
        deviceName = alcGetString(mDevice, ALC_ALL_DEVICES_SPECIFIER);
    }
    if (deviceName == nullptr || alcGetError(mDevice) != ALC_NO_ERROR) {
        deviceName = alcGetString(mDevice, ALC_DEVICE_SPECIFIER);
    }
    SPDLOG_INFO("OpenAL AudioPlayer: Opened device \"{}\"", deviceName ? deviceName : "unknown");
    
    // Create and activate an audio context
    mContext = alcCreateContext(mDevice, nullptr);
    if (mContext == nullptr) {
        SPDLOG_ERROR("OpenAL: Failed to create context");
        alcCloseDevice(mDevice);
        mDevice = nullptr;
        return false;
    }
    
    if (alcMakeContextCurrent(mContext) == ALC_FALSE) {
        SPDLOG_ERROR("OpenAL: Failed to make context current");
        alcDestroyContext(mContext);
        alcCloseDevice(mDevice);
        mContext = nullptr;
        mDevice = nullptr;
        return false;
    }
    
    // Clear any pending errors
    alGetError();
    
    // Get the appropriate format for our channel configuration
    mFormat = GetOpenALFormat();
    SPDLOG_INFO("OpenAL AudioPlayer: Using format 0x{:X} with {} channels", mFormat, mNumChannels);
    
    // Generate buffers for streaming
    alGenBuffers(NUM_OPENAL_BUFFERS, mBuffers);
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        SPDLOG_ERROR("OpenAL: Failed to generate buffers (error: 0x{:X})", error);
        return false;
    }
    
    // All buffers start as free
    for (int i = 0; i < NUM_OPENAL_BUFFERS; i++) {
        mFreeBuffers.push(mBuffers[i]);
    }
    
    // Generate a source for playback
    alGenSources(1, &mSource);
    error = alGetError();
    if (error != AL_NO_ERROR) {
        SPDLOG_ERROR("OpenAL: Failed to generate source (error: 0x{:X})", error);
        alDeleteBuffers(NUM_OPENAL_BUFFERS, mBuffers);
        return false;
    }
    
    // Configure the source for non-positional audio
    // Set position at origin, relative to listener, no distance attenuation
    alSource3f(mSource, AL_POSITION, 0.0f, 0.0f, 0.0f);
    alSourcei(mSource, AL_SOURCE_RELATIVE, AL_TRUE);
    alSourcef(mSource, AL_ROLLOFF_FACTOR, 0.0f);
    
    SPDLOG_INFO("OpenAL AudioPlayer: Initialized with {} channels at {} Hz", 
                mNumChannels, GetSampleRate());
    
    return true;
}

void OpenALAudioPlayer::ReclaimProcessedBuffers() {
    ALint processed = 0;
    alGetSourcei(mSource, AL_BUFFERS_PROCESSED, &processed);
    
    while (processed > 0) {
        ALuint buffer;
        alSourceUnqueueBuffers(mSource, 1, &buffer);
        mFreeBuffers.push(buffer);
        processed--;
    }
}

int32_t OpenALAudioPlayer::Buffered() {
    // First, reclaim any processed buffers
    ReclaimProcessedBuffers();
    
    // Get how many buffers are currently queued
    ALint queued = 0;
    alGetSourcei(mSource, AL_BUFFERS_QUEUED, &queued);
    
    if (queued == 0) {
        return 0;
    }
    
    // Get the current playback position in samples
    ALint sampleOffset = 0;
    alGetSourcei(mSource, AL_SAMPLE_OFFSET, &sampleOffset);
    
    // Estimate buffered samples based on queued buffers
    // Each buffer holds approximately GetSampleLength() samples
    int32_t bufferedSamples = queued * GetSampleLength() - sampleOffset;
    if (bufferedSamples < 0) {
        bufferedSamples = 0;
    }
    
    return bufferedSamples;
}

void OpenALAudioPlayer::Play(const uint8_t* buf, size_t len) {
    if (len == 0 || buf == nullptr) {
        return;
    }
    
    // Don't queue too much audio data
    if (Buffered() >= 6000) {
        return;
    }
    
    // Reclaim any processed buffers
    ReclaimProcessedBuffers();
    
    // Check if we have a free buffer
    if (mFreeBuffers.empty()) {
        // No buffers available, drop this audio frame
        return;
    }
    
    // Get a free buffer
    ALuint buffer = mFreeBuffers.front();
    mFreeBuffers.pop();
    
    // Fill the buffer with new audio data
    alBufferData(buffer, mFormat, buf, static_cast<ALsizei>(len), GetSampleRate());
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        SPDLOG_ERROR("OpenAL: Failed to buffer audio data (error: 0x{:X}, format: 0x{:X}, len: {}, rate: {})", 
                     error, mFormat, len, GetSampleRate());
        // Put the buffer back since we couldn't use it
        mFreeBuffers.push(buffer);
        return;
    }
    
    // Queue the buffer on the source
    alSourceQueueBuffers(mSource, 1, &buffer);
    error = alGetError();
    if (error != AL_NO_ERROR) {
        SPDLOG_ERROR("OpenAL: Failed to queue buffer (error: 0x{:X})", error);
        // Put the buffer back since we couldn't queue it
        mFreeBuffers.push(buffer);
        return;
    }
    
    // Make sure the source is playing
    ALint state = 0;
    alGetSourcei(mSource, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING) {
        alSourcePlay(mSource);
    }
}

} // namespace Ship
