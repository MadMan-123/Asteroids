#include "game.h"
#include "Asteroid.h"


static void gameInit(const c8 *projectDir)
{
    RuntimeConfig config = {
        {0.0f, 0.0f, 0.0f},  // gravity - no gravity for space asteroids
        1.0f / 60.0f,
        60.0f,
        0.1f,
        1000.0f,
        {0.0f, 5.0f, 20.0f},
        1280.0f / 720.0f
    };
    runtimeCreate(projectDir, config);

}

f64 time = 0.0;
f64 lastSpawnTime = 0.0;

static void gameUpdate(f32 dt)
{
    time += dt;
    runtimeUpdate(runtime, dt);

    if(time - lastSpawnTime >= 1.0) // every 1 second
    {
        lastSpawnTime = time;
        // Spawn an asteroid
        f32 xCos = cosf(time);
        f32 zSin = sinf(time);
        Vec3 spawnPos = {
            runtime->camera->pos.x + xCos * 10.0f,
            runtime->camera->pos.y,
            runtime->camera->pos.z + zSin * 10.0f
        };

        asteroidSpawn(spawnPos);
        INFO("Spawned asteroid at (%.2f, %.2f, %.2f)", spawnPos.x, spawnPos.y, spawnPos.z);
    }
}

static void gameRender(f32 dt)
{
    runtimeBeginScenePass(runtime, dt);
    runtimeEndScenePass(runtime);
}

static void gameDestroy(void)
{
    runtimeDestroy(runtime);
}

void druidGetPlugin(GamePlugin *out)
{
    out->init    = gameInit;
    out->update  = gameUpdate;
    out->render  = gameRender;
    out->destroy = gameDestroy;
}
