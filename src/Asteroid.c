#define DRUID_SYSTEM_EXPORT
#include "Asteroid.h"

DEFINE_ARCHETYPE(Asteroid, ASTEROID_FIELDS)

static Archetype *s_arch = NULL;

void asteroidInit(Archetype *arch)
{
    s_arch = arch;
}

void asteroidUpdate(Archetype *arch, f32 dt)
{
    (void)dt;
    for (u32 _ch = 0; _ch < arch->activeChunkCount; _ch++)
    {
        void **fields = getArchetypeFields(arch, _ch);
        if (!fields) continue;
        u32 count = arch->arena[_ch].count;
        b8 *alive = (b8 *)fields[ASTEROID_ALIVE];
        for (u32 i = 0; i < count; i++)
        {
            if (!alive[i]) continue;
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

    ((f32 *)fields[ASTEROID_LINEAR_VELOCITY_X])[localIdx] = velocity.x;
    ((f32 *)fields[ASTEROID_LINEAR_VELOCITY_Y])[localIdx] = velocity.y;
    ((f32 *)fields[ASTEROID_LINEAR_VELOCITY_Z])[localIdx] = velocity.z;
}

static void asteroidInitPlugin(void) {}

void druidGetECSSystem_Asteroid(ECSSystemPlugin *out)
{
    out->init    = asteroidInitPlugin;
    out->update  = asteroidUpdate;
    out->render  = NULL;
    out->destroy = asteroidDestroy;
}
