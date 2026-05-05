#include "game.h"
#include "Asteroid.h"
#include "Spaceship.h"
#include "Laser.h"
#include "Crystal.h"
#include <math.h>
#include <stdlib.h>

static Archetype g_asteroidArch = {0};
static Archetype g_shipArch     = {0};
static Archetype g_laserArch    = {0};
static Archetype g_crystalArch  = {0};

static u32 s_randomSeed = 12345u;
static f32 randomFloat(f32 min, f32 max)
{
    s_randomSeed = (s_randomSeed * 1103515245u + 12345u) & 0x7fffffffu;
    f32 normalized = (f32)s_randomSeed / (f32)0x7fffffffu;
    return min + normalized * (max - min);
}

static void gameInit(const c8 *projectDir)
{
    RuntimeConfig config = runtimeDefaultConfig();
    config.gravity.x = 0.0f;
    config.gravity.y = 0.0f;
    config.gravity.z = 0.0f;
    config.camNear   = 0.1f;
    config.camFar    = 20000.0f;
    runtimeCreate(projectDir, config);

    // Asteroid archetype — buffered pool + physics
    g_asteroidArch.flags = 0;
    FLAG_SET(g_asteroidArch.flags, ARCH_BUFFERED);
    FLAG_SET(g_asteroidArch.flags, ARCH_PHYSICS_BODY);
    if (!createArchetype(asteroidGetLayout(), POOL_CAPACITY, &g_asteroidArch))
    {
        ERROR("Failed to create Asteroid archetype");
        return;
    }
    asteroidInit(&g_asteroidArch);
    runtimeRegisterArchetype(runtime, &g_asteroidArch);

    // Spaceship archetype — single entity + physics
    g_shipArch.flags = 0;
    FLAG_SET(g_shipArch.flags, ARCH_SINGLE);
    FLAG_SET(g_shipArch.flags, ARCH_PHYSICS_BODY);
    if (!createArchetype(shipGetLayout(), 1, &g_shipArch))
    {
        ERROR("Failed to create Spaceship archetype");
        return;
    }
    shipInit(&g_shipArch);
    runtimeRegisterArchetype(runtime, &g_shipArch);

    Vec3 shipStart = {0.0f, 0.0f, 0.0f};
    shipSpawn(shipStart);

    // Laser archetype — buffered pool, no physics
    g_laserArch.flags = 0;
    FLAG_SET(g_laserArch.flags, ARCH_BUFFERED);
    if (!createArchetype(laserGetLayout(), LASER_POOL_CAPACITY, &g_laserArch))
    {
        ERROR("Failed to create Laser archetype");
        return;
    }
    laserInit(&g_laserArch);
    runtimeRegisterArchetype(runtime, &g_laserArch);

    // Crystal archetype — buffered pool, no physics
    // Requires a Crystal prefab configured in the editor (slot 0), same as Laser/Asteroid
    g_crystalArch.flags = 0;
    FLAG_SET(g_crystalArch.flags, ARCH_BUFFERED);
    if (!createArchetype(crystalGetLayout(), CRYSTAL_POOL_CAPACITY, &g_crystalArch))
    {
        ERROR("Failed to create Crystal archetype");
        return;
    }
    crystalInit(&g_crystalArch);
    runtimeRegisterArchetype(runtime, &g_crystalArch);

    // Spawn 300 crystals in 20 clusters scattered through the asteroid field (100-2000 units)
    for (u32 cluster = 0; cluster < 20; cluster++)
    {
        f32  theta  = randomFloat(0.0f, 6.28318f);
        f32  phi    = randomFloat(0.0f, 3.14159f);
        f32  radius = randomFloat(100.0f, 2000.0f);
        f32  sinPhi = sinf(phi);
        Vec3 center = {
            sinPhi * cosf(theta) * radius,
            sinPhi * sinf(theta) * radius,
            cosf(phi)            * radius,
        };
        for (u32 j = 0; j < 15; j++)
        {
            Vec3 pos = {
                center.x + randomFloat(-200.0f, 200.0f),
                center.y + randomFloat(-200.0f, 200.0f),
                center.z + randomFloat(-200.0f, 200.0f),
            };
            crystalSpawn(pos, randomFloat(1.0f, 1.5f));
        }
    }

    // Burst-spawn 10k asteroids in a 2000-unit shell around the origin
    for (u32 i = 0; i < 10000; i++)
    {
        f32 theta  = randomFloat(0.0f, 6.28318f);
        f32 phi    = randomFloat(0.0f, 3.14159f);
        f32 radius = randomFloat(100.0f, 2000.0f);
        f32 sinPhi = sinf(phi);

        Vec3 pos = {
            sinPhi * cosf(theta) * radius,
            sinPhi * sinf(theta) * radius,
            cosf(phi)            * radius,
        };

        f32 speed = randomFloat(2.0f, 15.0f);
        Vec3 vel  = {
            randomFloat(-1.0f, 1.0f) * speed,
            randomFloat(-1.0f, 1.0f) * speed,
            randomFloat(-1.0f, 1.0f) * speed,
        };

        asteroidSpawn(pos, vel);
    }
}

static void gameUpdate(f32 dt)
{
    runtimeUpdate(runtime, dt);
    shipUpdate(&g_shipArch, dt);
    asteroidUpdate(&g_asteroidArch, dt);
    laserUpdate(&g_laserArch, dt);
    laserCheckCollisions(&g_asteroidArch);
    crystalUpdate(&g_crystalArch, dt);

    if (g_shipArch.arena && g_shipArch.arena[0].count > 0)
    {
        void **sf = getArchetypeFields(&g_shipArch, 0);
        if (sf)
            crystalCheckCollection(((f32*)sf[SHIP_POSITION_X])[0],
                                   ((f32*)sf[SHIP_POSITION_Y])[0],
                                   ((f32*)sf[SHIP_POSITION_Z])[0]);
    }
}

static void gameRender(f32 dt)
{
    runtimeBeginScenePass(runtime, dt);
    rendererDefaultArchetypeRender(&g_asteroidArch, renderer);
    rendererDefaultArchetypeRender(&g_laserArch, renderer);
    rendererDefaultArchetypeRender(&g_crystalArch, renderer);
    runtimeEndScenePass(runtime);
}

static void gameDestroy(void)
{
    shipDestroy();
    destroyArchetype(&g_shipArch);

    asteroidDestroy();
    destroyArchetype(&g_asteroidArch);

    laserDestroy();
    destroyArchetype(&g_laserArch);

    crystalDestroy();
    destroyArchetype(&g_crystalArch);

    runtimeDestroy(runtime);
}

u32 druidGetGameArchetypes(Archetype **out, u32 max)
{
    u32 n = 0;
    if (n < max && g_asteroidArch.arena) out[n++] = &g_asteroidArch;
    if (n < max && g_shipArch.arena)     out[n++] = &g_shipArch;
    if (n < max && g_laserArch.arena)    out[n++] = &g_laserArch;
    if (n < max && g_crystalArch.arena)  out[n++] = &g_crystalArch;
    return n;
}

void druidGetPlugin(GamePlugin *out)
{
    out->init    = gameInit;
    out->update  = gameUpdate;
    out->render  = gameRender;
    out->destroy = gameDestroy;
}
