#include "GameAudio.h"

#define THRUST_COOLDOWN   0.45f
#define WHISPER_COOLDOWN  8.0f

static f32 s_thrustTimer   = 0.0f;
static f32 s_whisperTimer  = 0.0f;
static i32 s_thrustVoice   = -1;

void gameAudioInit(void) {}

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
    playSound("ShipHit.mp3", 0.3f);
}

void gameAudioShipDeath(void)
{
    stopMusic();
    playSound("death.mp3", 0.6f);
}

void gameAudioLaser(Vec3 pos)
{
    playSoundAt("laser.mp3", pos, 400.0f, 0.2f);
}

void gameAudioAsteroidImpact(Vec3 pos)
{
    playSoundAt("asteroidImpact.mp3", pos, 500.0f, 0.8f);
}

void gameAudioAsteroidDestroy(Vec3 pos)
{
    playSoundAt("RockDestroy.mp3", pos, 600.0f, 0.7f);
}

void gameAudioCrystalCollect(void)
{
    playSound("gem.mp3", 0.4f);
}

void gameAudioCrystalWhisper(Vec3 pos)
{
    playSoundAt("GemWhispering.mp3", pos, 800.0f, 0.25f);
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
