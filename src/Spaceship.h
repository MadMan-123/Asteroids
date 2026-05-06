#pragma once
#include <druid.h>

#ifdef __cplusplus
extern "C" {
#endif

// <DRUID_GEN_BEGIN Spaceship>
// DRUID_FLAGS 0x05
// isSingle
// isPhysicsBody
#define SHIP_POOL_CAPACITY 1

#define SPACESHIP_FIELDS(FIELD) \
    FIELD(SHIP_POSITION_X,       "PositionX",       f32,  HOT) \
    FIELD(SHIP_POSITION_Y,       "PositionY",       f32,  HOT) \
    FIELD(SHIP_POSITION_Z,       "PositionZ",       f32,  HOT) \
    FIELD(SHIP_ROTATION,         "Rotation",        Vec4, HOT) \
    FIELD(SHIP_SCALE,            "Scale",           Vec3, HOT) \
    FIELD(SHIP_MODEL_ID,         "ModelID",         u32,  COLD) \
    FIELD(SHIP_LINEAR_VELOCITY_X,"LinearVelocityX", f32,  HOT) \
    FIELD(SHIP_LINEAR_VELOCITY_Y,"LinearVelocityY", f32,  HOT) \
    FIELD(SHIP_LINEAR_VELOCITY_Z,"LinearVelocityZ", f32,  HOT) \
    FIELD(SHIP_FORCE_X,          "ForceX",          f32,  HOT) \
    FIELD(SHIP_FORCE_Y,          "ForceY",          f32,  HOT) \
    FIELD(SHIP_FORCE_Z,          "ForceZ",          f32,  HOT) \
    FIELD(SHIP_PHYSICS_BODY_TYPE,"PhysicsBodyType", u32,  COLD) \
    FIELD(SHIP_MASS,             "Mass",            f32,  COLD) \
    FIELD(SHIP_INV_MASS,         "InvMass",         f32,  COLD) \
    FIELD(SHIP_RESTITUTION,      "Restitution",     f32,  COLD) \
    FIELD(SHIP_LINEAR_DAMPING,   "LinearDamping",   f32,  COLD) \
    FIELD(SHIP_SPHERE_RADIUS,    "SphereRadius",    f32,  COLD) \
    FIELD(SHIP_COLLIDER_HALF_X,  "ColliderHalfX",   f32,  COLD) \
    FIELD(SHIP_COLLIDER_HALF_Y,  "ColliderHalfY",   f32,  COLD) \
    FIELD(SHIP_COLLIDER_HALF_Z,  "ColliderHalfZ",   f32,  COLD) \
    FIELD(SHIP_COLLIDER_OFFSET_X,"ColliderOffsetX", f32,  COLD) \
    FIELD(SHIP_COLLIDER_OFFSET_Y,"ColliderOffsetY", f32,  COLD) \
    FIELD(SHIP_COLLIDER_OFFSET_Z,"ColliderOffsetZ", f32,  COLD) \
    FIELD(SHIP_YAW_RATE,         "YawRate",         f32,  HOT) \
    FIELD(SHIP_PITCH_RATE,       "PitchRate",       f32,  HOT) \
    FIELD(SHIP_ROLL_RATE,        "RollRate",        f32,  HOT) \
    FIELD(SHIP_THRUST_FORCE,     "ThrustForce",     f32,  COLD) \
    FIELD(SHIP_ANGULAR_ACCEL,    "AngularAccel",    f32,  COLD) \
    FIELD(SHIP_ANGULAR_DAMPING,  "AngularDamping",  f32,  COLD)

DECLARE_ARCHETYPE(Spaceship, SPACESHIP_FIELDS)
// <DRUID_GEN_END Spaceship>

DSAPI StructLayout *shipGetLayout(void);
DSAPI void      shipInit(Archetype *arch);
DSAPI void      shipUpdate(Archetype *arch, f32 dt);
DSAPI void      shipDestroy(void);
DSAPI Archetype *shipGetArchetype(void);
DSAPI void      shipSpawn(Vec3 position);
DSAPI Vec3      shipGetPos(void);

DSAPI void druidGetECSSystem_Spaceship(ECSSystemPlugin *out);

#ifdef __cplusplus
}
#endif
