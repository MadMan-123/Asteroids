#pragma once
#include <druid.h>

#ifdef __cplusplus
extern "C" {
#endif

void gameAudioInit(void);
void gameAudioSetListener(Vec3 pos);

// Call every frame; thrusting=0 cuts the sound immediately
void gameAudioThrust(b8 thrusting, b8 boosting, f32 dt);

// Player-local (non-spatial)
void gameAudioShipHit(void);
void gameAudioShipDeath(void);

// Spatial — attenuated by distance from listener
void gameAudioLaser(Vec3 pos);
void gameAudioAsteroidImpact(Vec3 pos);
void gameAudioAsteroidDestroy(Vec3 pos);
void gameAudioCrystalCollect(void);     // player is right there, non-spatial
void gameAudioCrystalWhisper(Vec3 pos); // long-range ambient to guide the player
void gameAudioTickWhispers(f32 dt, f32 nearestDist, Vec3 nearestPos); // call once/frame from crystalUpdate
void gameAudioTick(f32 dt);  // call once/frame from gameUpdate

#ifdef __cplusplus
}
#endif
