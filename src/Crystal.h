#pragma once
#include <druid.h>

#ifdef __cplusplus
extern "C" {
#endif

// <DRUID_GEN_BEGIN Crystal>
// DRUID_FLAGS 0x08
// isBuffered
#define CRYSTAL_POOL_CAPACITY 500

#define CRYSTAL_FIELDS(FIELD) \
    FIELD(CRYSTAL_ALIVE,      "Alive",     b8,   COLD) \
    FIELD(CRYSTAL_POSITION_X, "PositionX", f32,  HOT)  \
    FIELD(CRYSTAL_POSITION_Y, "PositionY", f32,  HOT)  \
    FIELD(CRYSTAL_POSITION_Z, "PositionZ", f32,  HOT)  \
    FIELD(CRYSTAL_ROTATION,   "Rotation",  Vec4, HOT)  \
    FIELD(CRYSTAL_SCALE,      "Scale",     Vec3, HOT)  \
    FIELD(CRYSTAL_MODEL_ID,   "ModelID",   u32,  COLD)

DECLARE_ARCHETYPE(Crystal, CRYSTAL_FIELDS)
// <DRUID_GEN_END Crystal>

DSAPI StructLayout *crystalGetLayout(void);
DSAPI void         crystalInit(Archetype *arch);
DSAPI void         crystalUpdate(Archetype *arch, f32 dt);
DSAPI void         crystalDestroy(void);
DSAPI Archetype   *crystalGetArchetype(void);
DSAPI void         crystalSpawn(Vec3 position, f32 scale);
DSAPI void         crystalCheckCollection(f32 playerX, f32 playerY, f32 playerZ);

#ifdef __cplusplus
}
#endif
