#pragma once
#include <druid.h>

#ifdef __cplusplus
extern "C" {
#endif

// <DRUID_GEN_BEGIN Laser>
// DRUID_FLAGS 0x08
// isBuffered
#define LASER_POOL_CAPACITY 2000

#define LASER_FIELDS(FIELD) \
    FIELD(LASER_ALIVE,      "Alive",     b8,   COLD) \
    FIELD(LASER_POSITION_X, "PositionX", f32,  HOT)  \
    FIELD(LASER_POSITION_Y, "PositionY", f32,  HOT)  \
    FIELD(LASER_POSITION_Z, "PositionZ", f32,  HOT)  \
    FIELD(LASER_ROTATION,   "Rotation",  Vec4, HOT)  \
    FIELD(LASER_SCALE,      "Scale",     Vec3, HOT)  \
    FIELD(LASER_MODEL_ID,   "ModelID",   u32,  COLD) \
    FIELD(LASER_VELOCITY_X, "VelocityX", f32,  HOT)  \
    FIELD(LASER_VELOCITY_Y, "VelocityY", f32,  HOT)  \
    FIELD(LASER_VELOCITY_Z, "VelocityZ", f32,  HOT)  \
    FIELD(LASER_LIFETIME,   "Lifetime",  f32,  HOT)

DECLARE_ARCHETYPE(Laser, LASER_FIELDS)
// <DRUID_GEN_END Laser>

DSAPI void      laserInit(Archetype *arch);
DSAPI void      laserUpdate(Archetype *arch, f32 dt);
DSAPI void      laserDestroy(void);
DSAPI Archetype *laserGetArchetype(void);
DSAPI void      laserFire(Vec3 position, Vec3 direction);
DSAPI void      laserCheckCollisions(Archetype *asteroids);

DSAPI void druidGetECSSystem_Laser(ECSSystemPlugin *out);

#ifdef __cplusplus
}
#endif
