#include "GameAudio.h"

#define THRUST_COOLDOWN   0.45f
#define WHISPER_COOLDOWN  8.0f

static f32 s_thrustTimer   = 0.0f;
static f32 s_whisperTimer  = 0.0f;
static i32 s_thrustVoice   = -1;

// 3D audio listener position
static Vec3 s_listenerPos = {0.0f, 0.0f, 0.0f};

void gameAudioInit(void) {}

// 3D audio helper: set the listener position for spatial audio
static void setAudioListener(Vec3 pos)
{
    s_listenerPos = pos;
}

// 3D audio helper: play sound with spatial attenuation based on distance from listener
static void playSoundAt(const c8 *name, Vec3 pos, f32 maxDist, f32 volume)
{
    f32 distance = v3Dis(pos, s_listenerPos);
    f32 attenuatedVolume = volume;
    
    if (distance > maxDist)
    {
        return; // Sound is beyond max distance, don't play it
    }
    
    if (distance > 0.0f)
    {
        // Inverse square law: intensity decreases with square of distance
        f32 normalizedDist = distance / maxDist;
        attenuatedVolume = volume * (1.0f - (normalizedDist * normalizedDist));
    }
    
    if (attenuatedVolume > 0.001f)
    {
        playSound(name, attenuatedVolume);
    }
}

void gameAudioSetListener(Vec3 pos)
{
    setAudioListener(pos);
}

void gameAudioThrust(b8 thrusting, b8 boosting, f32 dt)
{
    if (!thrusting)
    {
        if (s_thrustVoice >= 0)
        {
            stopSound(s_thrustVoice);
            s_thrustVoice = -1;
        }
        s_thrustTimer = 0.0f;
        return;
    }

    s_thrustTimer -= dt;
    if (s_thrustTimer > 0.0f) return;

    if (s_thrustVoice >= 0)
    {
        stopSound(s_thrustVoice);
        s_thrustVoice = -1;
    }

    if (boosting)
        s_thrustVoice = playSound("thrustmax.mp3", 0.3f);
    else
        s_thrustVoice = playSound("thrustnormal.mp3", 0.2f);

    s_thrustTimer = THRUST_COOLDOWN;
}

void gameAudioShipHit(void)
{
    playSound("ShipHit.mp3", 0.2f);
}

void gameAudioShipDeath(void)
{
    playSound("death.mp3", 0.6f);

    //wait until death sound is almost done before stopping music
    SDL_Delay(1500);
    stopMusic();
}

void gameAudioLaser(Vec3 pos)
{
    playSoundAt("laser.mp3", pos, 400.0f, 0.1f);
}

void gameAudioAsteroidImpact(Vec3 pos)
{
    playSoundAt("asteroidImpact.mp3", pos, 500.0f, 0.1f);
}

void gameAudioAsteroidDestroy(Vec3 pos)
{
    playSoundAt("RockDestroy.mp3", pos, 600.0f, 0.5f);
}

void gameAudioCrystalCollect(void)
{
    playSound("gem.mp3", 0.35f);
}

void gameAudioCrystalWhisper(Vec3 pos)
{
    playSoundAt("GemWhispering.mp3", pos, 800.0f, 0.35f);
}

// Call once per frame from crystalUpdate; fires nearest-crystal whisper on cooldown
void gameAudioTickWhispers(f32 dt, f32 nearestDist, Vec3 nearestPos)
{
    s_whisperTimer -= dt;
    if (s_whisperTimer > 0.0f) return;
    if (nearestDist >= 800.0f) return;

    gameAudioCrystalWhisper(nearestPos);
    s_whisperTimer = WHISPER_COOLDOWN;
}
