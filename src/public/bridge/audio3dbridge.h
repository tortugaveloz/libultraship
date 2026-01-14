#pragma once

/**
 * Audio3D Bridge API
 * 
 * Provides 3D positional audio support for Starship.
 * When OpenAL backend is active, enables true 3D spatialization.
 * For other backends (SDL, WASAPI), provides stub implementations
 * and the game falls back to existing pan-based audio.
 * 
 * Usage from Starship:
 * 1. Initialize once at startup: Audio3D_Init()
 * 2. Each frame, update listener (camera) position
 * 3. For 3D sounds, create sources and set positions
 * 4. Call Audio3D_Update() each frame to clean up finished sounds
 * 5. Shutdown at exit: Audio3D_Shutdown()
 */

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Types
 * ============================================================================ */

/** Source identifier returned by Audio3D_CreateSource */
typedef uint32_t Audio3DSourceId;

/** 3D vector for positions, velocities, and directions */
typedef struct {
    float x, y, z;
} Audio3DVec3;

/** Distance attenuation models */
typedef enum {
    AUDIO3D_ATTENUATION_NONE = 0,
    AUDIO3D_ATTENUATION_INVERSE,
    AUDIO3D_ATTENUATION_INVERSE_CLAMPED,
    AUDIO3D_ATTENUATION_LINEAR,
    AUDIO3D_ATTENUATION_LINEAR_CLAMPED,
    AUDIO3D_ATTENUATION_EXPONENT,
    AUDIO3D_ATTENUATION_EXPONENT_CLAMPED
} Audio3DAttenuationModel;

/** Source playback state */
typedef enum {
    AUDIO3D_STATE_STOPPED = 0,
    AUDIO3D_STATE_PLAYING = 1,
    AUDIO3D_STATE_PAUSED = 2
} Audio3DSourceState;

/* ============================================================================
 * Initialization
 * ============================================================================ */

/**
 * Initialize the 3D audio system.
 * Should be called once after the main audio backend is initialized.
 * @return true if 3D audio is available, false otherwise
 */
bool Audio3D_Init(void);

/**
 * Shutdown the 3D audio system and release all resources.
 */
void Audio3D_Shutdown(void);

/**
 * Check if 3D audio is currently available.
 * Returns false if not initialized or if backend doesn't support 3D.
 */
bool Audio3D_IsAvailable(void);

/* ============================================================================
 * Global Settings
 * ============================================================================ */

/**
 * Set the distance attenuation model.
 * Default is AUDIO3D_ATTENUATION_INVERSE_CLAMPED.
 */
void Audio3D_SetAttenuationModel(Audio3DAttenuationModel model);

/**
 * Set speed of sound for Doppler calculations.
 * @param speed Speed in game units per second (default: 343.3)
 */
void Audio3D_SetSpeedOfSound(float speed);

/**
 * Set Doppler effect multiplier.
 * @param factor 0.0 = no Doppler, 1.0 = normal, >1.0 = exaggerated
 */
void Audio3D_SetDopplerFactor(float factor);

/* ============================================================================
 * Listener (Camera/Player)
 * ============================================================================ */

/**
 * Set listener (camera) position in world space.
 * Call this each frame with the camera position.
 */
void Audio3D_SetListenerPosition(float x, float y, float z);
void Audio3D_SetListenerPositionVec(const Audio3DVec3* pos);

/**
 * Set listener orientation.
 * @param atX,atY,atZ Forward direction vector (where the listener faces)
 * @param upX,upY,upZ Up direction vector
 */
void Audio3D_SetListenerOrientation(float atX, float atY, float atZ,
                                     float upX, float upY, float upZ);

/**
 * Set listener velocity for Doppler effect.
 * Only needed if using Doppler effect.
 */
void Audio3D_SetListenerVelocity(float vx, float vy, float vz);
void Audio3D_SetListenerVelocityVec(const Audio3DVec3* vel);

/**
 * Set master gain for all 3D audio.
 * @param gain 0.0 = silent, 1.0 = normal
 */
void Audio3D_SetListenerGain(float gain);

/* ============================================================================
 * Source Management
 * ============================================================================ */

/**
 * Create a new 3D audio source.
 * @return Source ID, or 0 if creation failed
 */
Audio3DSourceId Audio3D_CreateSource(void);

/**
 * Destroy a 3D audio source and release its resources.
 */
void Audio3D_DestroySource(Audio3DSourceId source);

/**
 * Destroy all 3D audio sources.
 */
void Audio3D_DestroyAllSources(void);

/* ============================================================================
 * Source Properties
 * ============================================================================ */

/**
 * Set source position in world space.
 */
void Audio3D_SetSourcePosition(Audio3DSourceId source, float x, float y, float z);
void Audio3D_SetSourcePositionVec(Audio3DSourceId source, const Audio3DVec3* pos);

/**
 * Set source velocity for Doppler effect.
 */
void Audio3D_SetSourceVelocity(Audio3DSourceId source, float vx, float vy, float vz);
void Audio3D_SetSourceVelocityVec(Audio3DSourceId source, const Audio3DVec3* vel);

/**
 * Set source gain (volume).
 * @param gain 0.0 = silent, 1.0 = normal
 */
void Audio3D_SetSourceGain(Audio3DSourceId source, float gain);

/**
 * Set source pitch multiplier.
 * @param pitch 1.0 = normal, 2.0 = octave up, 0.5 = octave down
 */
void Audio3D_SetSourcePitch(Audio3DSourceId source, float pitch);

/**
 * Set whether source should loop.
 */
void Audio3D_SetSourceLooping(Audio3DSourceId source, bool looping);

/**
 * Set reference distance for attenuation.
 * At this distance, gain is 1.0. Closer = louder, farther = quieter.
 */
void Audio3D_SetSourceReferenceDistance(Audio3DSourceId source, float distance);

/**
 * Set maximum distance for attenuation.
 * Beyond this distance, sound is silent (for clamped models).
 */
void Audio3D_SetSourceMaxDistance(Audio3DSourceId source, float distance);

/**
 * Set rolloff factor for distance attenuation.
 * Higher values = faster volume falloff with distance.
 */
void Audio3D_SetSourceRolloff(Audio3DSourceId source, float rolloff);

/**
 * Set whether position is relative to listener.
 * @param relative If true, position is relative to listener (like headphones).
 *                 If false (default), position is in world space.
 */
void Audio3D_SetSourceRelative(Audio3DSourceId source, bool relative);

/* ============================================================================
 * Playback
 * ============================================================================ */

/**
 * Queue audio data on a source for streaming playback.
 * @param source Source to queue on
 * @param data PCM audio data (16-bit signed)
 * @param size Size of data in bytes
 * @param sampleRate Sample rate in Hz
 * @param channels Number of channels (1 = mono, 2 = stereo)
 */
void Audio3D_QueueBuffer(Audio3DSourceId source, const void* data, uint32_t size,
                          uint32_t sampleRate, uint8_t channels);

/**
 * Start or resume playback of a source.
 */
void Audio3D_PlaySource(Audio3DSourceId source);

/**
 * Pause playback of a source (can be resumed).
 */
void Audio3D_PauseSource(Audio3DSourceId source);

/**
 * Stop playback and clear queued buffers.
 */
void Audio3D_StopSource(Audio3DSourceId source);

/**
 * Get current playback state of a source.
 */
Audio3DSourceState Audio3D_GetSourceState(Audio3DSourceId source);

/**
 * Check if a source has finished all playback (stopped and no buffers queued).
 */
bool Audio3D_IsSourceFinished(Audio3DSourceId source);

/* ============================================================================
 * Convenience Functions
 * ============================================================================ */

/**
 * One-shot convenience: play audio at a position.
 * Creates a temporary source that auto-cleans on completion.
 * Best for sound effects (explosions, impacts, etc.)
 * 
 * @return Source ID (can be ignored, auto-cleaned)
 */
Audio3DSourceId Audio3D_PlayAtPosition(const void* data, uint32_t size, uint32_t sampleRate,
                                        float x, float y, float z, float gain, float pitch);

/**
 * Update function - call once per frame.
 * Cleans up finished one-shot sources and performs other maintenance.
 */
void Audio3D_Update(void);

#ifdef __cplusplus
}
#endif
