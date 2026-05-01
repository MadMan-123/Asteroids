#pragma once
#include <druid.h>

#ifdef __cplusplus
extern "C" {
#endif

// <DRUID_GEN_BEGIN Asteroid>
// DRUID_FLAGS 0x0C
// isBuffered
// isPhysicsBody
#define POOL_CAPACITY 1000

#define ASTEROID_FIELDS(FIELD) \
    FIELD(ASTEROID_ALIVE, "Alive", b8, COLD) \
    FIELD(ASTEROID_POSITION_X, "PositionX", f32, HOT) \
    FIELD(ASTEROID_POSITION_Y, "PositionY", f32, HOT) \
    FIELD(ASTEROID_POSITION_Z, "PositionZ", f32, HOT) \
    FIELD(ASTEROID_ROTATION, "Rotation", Vec4, HOT) \
    FIELD(ASTEROID_SCALE, "Scale", Vec3, HOT) \
    FIELD(ASTEROID_MODEL_ID, "ModelID", u32, COLD) \
    FIELD(ASTEROID_LINEAR_VELOCITY_X, "LinearVelocityX", f32, COLD) \
    FIELD(ASTEROID_LINEAR_VELOCITY_Y, "LinearVelocityY", f32, COLD) \
    FIELD(ASTEROID_LINEAR_VELOCITY_Z, "LinearVelocityZ", f32, COLD) \
    FIELD(ASTEROID_FORCE_X, "ForceX", f32, COLD) \
    FIELD(ASTEROID_FORCE_Y, "ForceY", f32, COLD) \
    FIELD(ASTEROID_FORCE_Z, "ForceZ", f32, COLD) \
    FIELD(ASTEROID_PHYSICS_BODY_TYPE, "PhysicsBodyType", u32, COLD) \
    FIELD(ASTEROID_MASS, "Mass", f32, COLD) \
    FIELD(ASTEROID_INV_MASS, "InvMass", f32, COLD) \
    FIELD(ASTEROID_RESTITUTION, "Restitution", f32, COLD) \
    FIELD(ASTEROID_LINEAR_DAMPING, "LinearDamping", f32, COLD) \
    FIELD(ASTEROID_SPHERE_RADIUS, "SphereRadius", f32, COLD) \
    FIELD(ASTEROID_COLLIDER_HALF_X, "ColliderHalfX", f32, COLD) \
    FIELD(ASTEROID_COLLIDER_HALF_Y, "ColliderHalfY", f32, COLD) \
    FIELD(ASTEROID_COLLIDER_HALF_Z, "ColliderHalfZ", f32, COLD) \
    FIELD(ASTEROID_COLLIDER_OFFSET_X, "ColliderOffsetX", f32, COLD) \
    FIELD(ASTEROID_COLLIDER_OFFSET_Y, "ColliderOffsetY", f32, COLD) \
    FIELD(ASTEROID_COLLIDER_OFFSET_Z, "ColliderOffsetZ", f32, COLD)

DECLARE_ARCHETYPE(Asteroid, ASTEROID_FIELDS)
// <DRUID_GEN_END Asteroid>

DSAPI void asteroidInit(void);
DSAPI void asteroidUpdate(Archetype *arch, f32 dt);
DSAPI void asteroidRender(Archetype *arch, Renderer *r);
DSAPI void asteroidDestroy(void);

DSAPI void druidGetECSSystem_Asteroid(ECSSystemPlugin *out);

DSAPI void asteroidSpawn(Vec3 position);

#ifdef __cplusplus
}
#endif
