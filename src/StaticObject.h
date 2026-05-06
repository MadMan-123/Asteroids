#pragma once
#include <druid.h>

#ifdef __cplusplus
extern "C" {
#endif

// <DRUID_GEN_BEGIN StaticObject>
// DRUID_FLAGS 0x04
// isPhysicsBody

#define STATICOBJECT_FIELDS(FIELD) \
    FIELD(STATICOBJECT_POSITION_X, "PositionX", f32, HOT) \
    FIELD(STATICOBJECT_POSITION_Y, "PositionY", f32, HOT) \
    FIELD(STATICOBJECT_POSITION_Z, "PositionZ", f32, HOT) \
    FIELD(STATICOBJECT_ROTATION, "Rotation", Vec4, HOT) \
    FIELD(STATICOBJECT_SCALE, "Scale", Vec3, HOT) \
    FIELD(STATICOBJECT_MODEL_ID, "ModelID", u32, COLD) \
    FIELD(STATICOBJECT_LINEAR_VELOCITY_X, "LinearVelocityX", f32, COLD) \
    FIELD(STATICOBJECT_LINEAR_VELOCITY_Y, "LinearVelocityY", f32, COLD) \
    FIELD(STATICOBJECT_LINEAR_VELOCITY_Z, "LinearVelocityZ", f32, COLD) \
    FIELD(STATICOBJECT_FORCE_X, "ForceX", f32, COLD) \
    FIELD(STATICOBJECT_FORCE_Y, "ForceY", f32, COLD) \
    FIELD(STATICOBJECT_FORCE_Z, "ForceZ", f32, COLD) \
    FIELD(STATICOBJECT_PHYSICS_BODY_TYPE, "PhysicsBodyType", u32, COLD) \
    FIELD(STATICOBJECT_MASS, "Mass", f32, COLD) \
    FIELD(STATICOBJECT_INV_MASS, "InvMass", f32, COLD) \
    FIELD(STATICOBJECT_RESTITUTION, "Restitution", f32, COLD) \
    FIELD(STATICOBJECT_LINEAR_DAMPING, "LinearDamping", f32, COLD) \
    FIELD(STATICOBJECT_SPHERE_RADIUS, "SphereRadius", f32, COLD) \
    FIELD(STATICOBJECT_COLLIDER_HALF_X, "ColliderHalfX", f32, COLD) \
    FIELD(STATICOBJECT_COLLIDER_HALF_Y, "ColliderHalfY", f32, COLD) \
    FIELD(STATICOBJECT_COLLIDER_HALF_Z, "ColliderHalfZ", f32, COLD) \
    FIELD(STATICOBJECT_COLLIDER_OFFSET_X, "ColliderOffsetX", f32, COLD) \
    FIELD(STATICOBJECT_COLLIDER_OFFSET_Y, "ColliderOffsetY", f32, COLD) \
    FIELD(STATICOBJECT_COLLIDER_OFFSET_Z, "ColliderOffsetZ", f32, COLD) \
    FIELD(STATICOBJECT_HEALTH, "Health", f32, COLD) \
    FIELD(STATICOBJECT_HIT_TIMER, "HitTimer", f32, COLD) \
    FIELD(STATICOBJECT_MATERIAL_ID, "materialID", u32, COLD)

DECLARE_ARCHETYPE(StaticObject, STATICOBJECT_FIELDS)
// <DRUID_GEN_END StaticObject>

DSAPI void staticObjectInit(Archetype *arch);
DSAPI void staticObjectUpdate(Archetype *arch, f32 dt);
DSAPI void staticObjectDestroy(void);
DSAPI Archetype *staticObjectGetArchetype(void);
DSAPI void staticObjectSpawn(Vec3 position);

DSAPI void druidGetECSSystem_StaticObject(ECSSystemPlugin *out);

// In gameInit: createArchetype(&StaticObject_layout, STATICOBJECT_POOL_CAPACITY, &g_staticObjectArch);
//             staticObjectInit(&g_staticObjectArch); runtimeRegisterArchetype(runtime, &g_staticObjectArch);

#ifdef __cplusplus
}
#endif
