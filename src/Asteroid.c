#define DRUID_SYSTEM_EXPORT
#include "Asteroid.h"
#include "game.h"

DEFINE_ARCHETYPE(Asteroid, ASTEROID_FIELDS)

static Archetype *s_arch = NULL;

void asteroidInit(void)
{
    INFO("asteroidInit called");
    // arch is obtained from druid runtime
}

void asteroidUpdate(Archetype *arch, f32 dt)
{
    if (!s_arch && arch)
    {
        INFO("asteroidUpdate: Setting s_arch for the first time");
    }
    s_arch = arch;
    
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
    if (s_arch)
    {
        s_arch = NULL;
    }
}

Archetype *asteroidGetArchetype(void) { return s_arch; }

void asteroidSpawn(Vec3 position)
{
    if (!s_arch) 
    {
        ERROR("asteroidSpawn: s_arch is NULL - archetype not initialized!");
        return;
    }

    const c8 *prefabName = "Asteroid1.prefab";
    u32 prefabIdx = prefabLoadDirectory(prefabName);
    if (prefabIdx == (u32)-1)
    {
        ERROR("asteroidSpawn: Prefab '%s' not found", prefabName);
        return;
    }

    u32 entity = prefabSpawn(s_arch, prefabIdx, position);
    if (entity == (u32)-1)
    {
        ERROR("asteroidSpawn: Failed to spawn prefab entity");
        return;
    }

    // Clear the asteroid's velocity after spawning
    u64 chunkId = (entity >> 24) & 0xFF;
    u64 localIdx = entity & 0xFFFFFF;
    
    void **fields = getArchetypeFields(s_arch, chunkId);
    if (fields)
    {
        f32 *velX = (f32 *)fields[ASTEROID_LINEAR_VELOCITY_X];
        f32 *velY = (f32 *)fields[ASTEROID_LINEAR_VELOCITY_Y];
        f32 *velZ = (f32 *)fields[ASTEROID_LINEAR_VELOCITY_Z];
        
        if (velX) velX[localIdx] = 0.0f;
        if (velY) velY[localIdx] = 0.0f;
        if (velZ) velZ[localIdx] = 0.0f;
    }
}

static void asteroidInitPlugin(void) {}

void druidGetECSSystem_Asteroid(ECSSystemPlugin *out)
{
    out->init    = asteroidInitPlugin;
    out->update  = asteroidUpdate;
    out->render  = NULL;
    out->destroy = asteroidDestroy;
}
