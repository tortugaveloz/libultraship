#pragma once
#include "AudioPlayer.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <queue>

namespace Ship {

// Number of buffers to use for streaming audio
constexpr int NUM_OPENAL_BUFFERS = 4;

class OpenALAudioPlayer final : public AudioPlayer {
  public:
    OpenALAudioPlayer(AudioSettings settings) : AudioPlayer(settings) {
    }
    ~OpenALAudioPlayer();

    int32_t Buffered() override;
    void Play(const uint8_t* buf, size_t len) override;

  protected:
    bool DoInit() override;

  private:
    ALCdevice* mDevice = nullptr;
    ALCcontext* mContext = nullptr;
    ALuint mSource = 0;
    ALuint mBuffers[NUM_OPENAL_BUFFERS] = {};
    ALenum mFormat = AL_FORMAT_STEREO16;
    int32_t mNumChannels = 2;
    
    // Queue of available (free) buffers
    std::queue<ALuint> mFreeBuffers;
    
    // Get the appropriate OpenAL format for the current channel configuration
    ALenum GetOpenALFormat();
    
    // Reclaim processed buffers back to the free queue
    void ReclaimProcessedBuffers();
};

} // namespace Ship
