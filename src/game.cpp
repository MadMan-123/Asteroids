#include "game.h"
#include "Asteroid.h"
#include "Spaceship.h"
#include "Laser.h"
#include <math.h>
#include <stdlib.h>

static Archetype g_asteroidArch = {0};
static Archetype g_shipArch     = {0};
static Archetype g_laserArch    = {0};

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
    config.camFar    = 5000.0f;
    runtimeCreate(projectDir, config);

    // Asteroid archetype — buffered pool + physics
    g_asteroidArch.flags = 0;
    FLAG_SET(g_asteroidArch.flags, ARCH_BUFFERED);
    FLAG_SET(g_asteroidArch.flags, ARCH_PHYSICS_BODY);
    if (!createArchetype(&Asteroid_layout, POOL_CAPACITY, &g_asteroidArch))
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
    if (!createArchetype(&Spaceship_layout, 1, &g_shipArch))
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
    if (!createArchetype(&Laser_layout, LASER_POOL_CAPACITY, &g_laserArch))
    {
        ERROR("Failed to create Laser archetype");
        return;
    }
    laserInit(&g_laserArch);
    runtimeRegisterArchetype(runtime, &g_laserArch);

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
}

static void gameRender(f32 dt)
{
    runtimeBeginScenePass(runtime, dt);
    rendererDefaultArchetypeRender(&g_asteroidArch, renderer);
    rendererDefaultArchetypeRender(&g_laserArch, renderer);
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

    runtimeDestroy(runtime);
}

u32 druidGetGameArchetypes(Archetype **out, u32 max)
{
    u32 n = 0;
    if (n < max && g_asteroidArch.arena) out[n++] = &g_asteroidArch;
    if (n < max && g_shipArch.arena)     out[n++] = &g_shipArch;
    if (n < max && g_laserArch.arena)    out[n++] = &g_laserArch;
    return n;
}

void druidGetPlugin(GamePlugin *out)
{
    out->init    = gameInit;
    out->update  = gameUpdate;
    out->render  = gameRender;
    out->destroy = gameDestroy;
}
