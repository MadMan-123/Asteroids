#define DRUID_SYSTEM_EXPORT
#include "Asteroid.h"
#include <stdlib.h>
#include <string.h>

#define NUM_ASTEROID_MODELS  43
#define SCALE_MIN            0.01f
#define SCALE_MAX            0.25f
#define SPIN_BASE            0.8f

// Varied tumble axes (normalised)
static const Vec3 SPIN_AXES[] = {
    { 0.948f,  0.285f,  0.000f },
    { 0.182f,  0.911f,  0.365f },
    { 0.447f,  0.000f,  0.894f },
    { 0.000f,  0.814f,  0.581f },
    { 0.707f,  0.707f,  0.000f },
    { 0.577f,  0.577f,  0.577f },
};
#define NUM_SPIN_AXES 6

DEFINE_ARCHETYPE(Asteroid, ASTEROID_FIELDS)

static Archetype *s_arch              = NULL;
static u32        s_modelIds[NUM_ASTEROID_MODELS];
static u32        s_modelCount        = 0;

void asteroidInit(Archetype *arch)
{
    s_arch = arch;

    s_modelCount = 0;
    for (u32 id = 0; s_modelCount < NUM_ASTEROID_MODELS; id++)
    {
        Model *m = resGetModel(id);
        if (!m || !m->name) break;
        if (strstr(m->name, "asteroids"))
            s_modelIds[s_modelCount++] = id;
    }
}

void asteroidUpdate(Archetype *arch, f32 dt)
{
    for (u32 _ch = 0; _ch < arch->activeChunkCount; _ch++)
    {
        void **fields = getArchetypeFields(arch, _ch);
        if (!fields) continue;
        u32 count = arch->arena[_ch].count;

        b8   *alive = (b8   *)fields[ASTEROID_ALIVE];
        f32  *posX  = (f32  *)fields[ASTEROID_POSITION_X];
        f32  *posY  = (f32  *)fields[ASTEROID_POSITION_Y];
        f32  *posZ  = (f32  *)fields[ASTEROID_POSITION_Z];
        Vec4 *rot   = (Vec4 *)fields[ASTEROID_ROTATION];
        f32  *velX  = (f32  *)fields[ASTEROID_LINEAR_VELOCITY_X];
        f32  *velY  = (f32  *)fields[ASTEROID_LINEAR_VELOCITY_Y];
        f32  *velZ  = (f32  *)fields[ASTEROID_LINEAR_VELOCITY_Z];

        for (u32 i = 0; i < count; i++)
        {
            if (!alive[i]) continue;

            posX[i] += velX[i] * dt;
            posY[i] += velY[i] * dt;
            posZ[i] += velZ[i] * dt;

            f32 spinRate = SPIN_BASE * (0.6f + (f32)(i % 7) * 0.12f);
            Vec4 dRot    = quatFromAxisAngle(SPIN_AXES[i % NUM_SPIN_AXES], spinRate * dt);
            rot[i]       = quatNormalize(quatMul(rot[i], dRot));
        }
    }
}

void asteroidDestroy(void)
{
    s_arch = NULL;
}

Archetype *asteroidGetArchetype(void) { return s_arch; }

void asteroidSpawn(Vec3 position, Vec3 velocity)
{
    if (!s_arch) return;

    u32 poolIdx = prefabSpawn(s_arch, 0, position);
    if (poolIdx == (u32)-1) return;

    u32 chunkIdx = poolIdx / s_arch->chunkCapacity;
    u32 localIdx = poolIdx % s_arch->chunkCapacity;
    void **fields = getArchetypeFields(s_arch, chunkIdx);
    if (!fields) return;

    ((f32  *)fields[ASTEROID_LINEAR_VELOCITY_X])[localIdx] = velocity.x;
    ((f32  *)fields[ASTEROID_LINEAR_VELOCITY_Y])[localIdx] = velocity.y;
    ((f32  *)fields[ASTEROID_LINEAR_VELOCITY_Z])[localIdx] = velocity.z;

    f32 scale   = SCALE_MIN + ((f32)rand() / (f32)RAND_MAX) * (SCALE_MAX - SCALE_MIN);
    u32 modelId = s_modelCount > 0 ? s_modelIds[(u32)rand() % s_modelCount] : 0;
    ((Vec3 *)fields[ASTEROID_SCALE            ])[localIdx] = (Vec3){ scale, scale, scale };
    ((u32  *)fields[ASTEROID_MODEL_ID         ])[localIdx] = modelId;
    ((u32  *)fields[ASTEROID_PHYSICS_BODY_TYPE])[localIdx] = PHYS_BODY_KINEMATIC;
    ((f32  *)fields[ASTEROID_MASS             ])[localIdx] = scale * 100.0f;
    ((f32  *)fields[ASTEROID_INV_MASS         ])[localIdx] = 1.0f / (scale * 100.0f);
    ((f32  *)fields[ASTEROID_RESTITUTION      ])[localIdx] = 0.4f;
    ((f32  *)fields[ASTEROID_LINEAR_DAMPING   ])[localIdx] = 0.0f;

    Model *m = resGetModel(modelId);
    f32 baseRadius = (m && m->boundingRadius > 0.0f) ? m->boundingRadius : 20.0f;
    ((f32  *)fields[ASTEROID_SPHERE_RADIUS    ])[localIdx] = baseRadius * scale;
}

static void asteroidInitPlugin(void) {}

void druidGetECSSystem_Asteroid(ECSSystemPlugin *out)
{
    out->init    = asteroidInitPlugin;
    out->update  = asteroidUpdate;
    out->render  = NULL;
    out->destroy = asteroidDestroy;
}
