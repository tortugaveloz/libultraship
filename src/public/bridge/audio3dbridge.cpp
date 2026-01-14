#include "audio3dbridge.h"
#include "audio/Audio3D.h"

using namespace Ship;

/* ============================================================================
 * Initialization
 * ============================================================================ */

bool Audio3D_Init(void) {
    return Audio3DManager::GetInstance()->Init();
}

void Audio3D_Shutdown(void) {
    Audio3DManager::DestroyInstance();
}

bool Audio3D_IsAvailable(void) {
    return Audio3DManager::GetInstance()->IsAvailable();
}

/* ============================================================================
 * Global Settings
 * ============================================================================ */

void Audio3D_SetAttenuationModel(Audio3DAttenuationModel model) {
    Audio3DManager::GetInstance()->SetAttenuationModel(static_cast<int>(model));
}

void Audio3D_SetSpeedOfSound(float speed) {
    Audio3DManager::GetInstance()->SetSpeedOfSound(speed);
}

void Audio3D_SetDopplerFactor(float factor) {
    Audio3DManager::GetInstance()->SetDopplerFactor(factor);
}

/* ============================================================================
 * Listener
 * ============================================================================ */

void Audio3D_SetListenerPosition(float x, float y, float z) {
    Audio3DManager::GetInstance()->SetListenerPosition(x, y, z);
}

void Audio3D_SetListenerPositionVec(const Audio3DVec3* pos) {
    if (pos) {
        Audio3DManager::GetInstance()->SetListenerPosition(pos->x, pos->y, pos->z);
    }
}

void Audio3D_SetListenerOrientation(float atX, float atY, float atZ,
                                     float upX, float upY, float upZ) {
    Audio3DManager::GetInstance()->SetListenerOrientation(atX, atY, atZ, upX, upY, upZ);
}

void Audio3D_SetListenerVelocity(float vx, float vy, float vz) {
    Audio3DManager::GetInstance()->SetListenerVelocity(vx, vy, vz);
}

void Audio3D_SetListenerVelocityVec(const Audio3DVec3* vel) {
    if (vel) {
        Audio3DManager::GetInstance()->SetListenerVelocity(vel->x, vel->y, vel->z);
    }
}

void Audio3D_SetListenerGain(float gain) {
    Audio3DManager::GetInstance()->SetListenerGain(gain);
}

/* ============================================================================
 * Source Management
 * ============================================================================ */

Audio3DSourceId Audio3D_CreateSource(void) {
    return Audio3DManager::GetInstance()->CreateSource();
}

void Audio3D_DestroySource(Audio3DSourceId source) {
    Audio3DManager::GetInstance()->DestroySource(source);
}

void Audio3D_DestroyAllSources(void) {
    Audio3DManager::GetInstance()->DestroyAllSources();
}

/* ============================================================================
 * Source Properties
 * ============================================================================ */

void Audio3D_SetSourcePosition(Audio3DSourceId source, float x, float y, float z) {
    Audio3DManager::GetInstance()->SetSourcePosition(source, x, y, z);
}

void Audio3D_SetSourcePositionVec(Audio3DSourceId source, const Audio3DVec3* pos) {
    if (pos) {
        Audio3DManager::GetInstance()->SetSourcePosition(source, pos->x, pos->y, pos->z);
    }
}

void Audio3D_SetSourceVelocity(Audio3DSourceId source, float vx, float vy, float vz) {
    Audio3DManager::GetInstance()->SetSourceVelocity(source, vx, vy, vz);
}

void Audio3D_SetSourceVelocityVec(Audio3DSourceId source, const Audio3DVec3* vel) {
    if (vel) {
        Audio3DManager::GetInstance()->SetSourceVelocity(source, vel->x, vel->y, vel->z);
    }
}

void Audio3D_SetSourceGain(Audio3DSourceId source, float gain) {
    Audio3DManager::GetInstance()->SetSourceGain(source, gain);
}

void Audio3D_SetSourcePitch(Audio3DSourceId source, float pitch) {
    Audio3DManager::GetInstance()->SetSourcePitch(source, pitch);
}

void Audio3D_SetSourceLooping(Audio3DSourceId source, bool looping) {
    Audio3DManager::GetInstance()->SetSourceLooping(source, looping);
}

void Audio3D_SetSourceReferenceDistance(Audio3DSourceId source, float distance) {
    Audio3DManager::GetInstance()->SetSourceReferenceDistance(source, distance);
}

void Audio3D_SetSourceMaxDistance(Audio3DSourceId source, float distance) {
    Audio3DManager::GetInstance()->SetSourceMaxDistance(source, distance);
}

void Audio3D_SetSourceRolloff(Audio3DSourceId source, float rolloff) {
    Audio3DManager::GetInstance()->SetSourceRolloff(source, rolloff);
}

void Audio3D_SetSourceRelative(Audio3DSourceId source, bool relative) {
    Audio3DManager::GetInstance()->SetSourceRelative(source, relative);
}

/* ============================================================================
 * Playback
 * ============================================================================ */

void Audio3D_QueueBuffer(Audio3DSourceId source, const void* data, uint32_t size,
                          uint32_t sampleRate, uint8_t channels) {
    Audio3DManager::GetInstance()->QueueBuffer(source, data, size, sampleRate, channels);
}

void Audio3D_PlaySource(Audio3DSourceId source) {
    Audio3DManager::GetInstance()->PlaySource(source);
}

void Audio3D_PauseSource(Audio3DSourceId source) {
    Audio3DManager::GetInstance()->PauseSource(source);
}

void Audio3D_StopSource(Audio3DSourceId source) {
    Audio3DManager::GetInstance()->StopSource(source);
}

Audio3DSourceState Audio3D_GetSourceState(Audio3DSourceId source) {
    return static_cast<Audio3DSourceState>(Audio3DManager::GetInstance()->GetSourceState(source));
}

bool Audio3D_IsSourceFinished(Audio3DSourceId source) {
    return Audio3DManager::GetInstance()->IsSourceFinished(source);
}

/* ============================================================================
 * Convenience Functions
 * ============================================================================ */

Audio3DSourceId Audio3D_PlayAtPosition(const void* data, uint32_t size, uint32_t sampleRate,
                                        float x, float y, float z, float gain, float pitch) {
    return Audio3DManager::GetInstance()->PlayAtPosition(data, size, sampleRate, x, y, z, gain, pitch);
}

void Audio3D_Update(void) {
    Audio3DManager::GetInstance()->Update();
}
