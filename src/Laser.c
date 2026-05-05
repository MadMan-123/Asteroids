#define DRUID_SYSTEM_EXPORT
#include "Laser.h"
#include "Asteroid.h"
#include "GameAudio.h"
#include <math.h>

#define LASER_PUSH 15.0f

DEFINE_ARCHETYPE(Laser, LASER_FIELDS)

#define LASER_SPEED        500.0f
#define LASER_LIFETIME_MAX   3.0f

static Archetype *s_arch = NULL;

void laserInit(Archetype *arch)
{
    s_arch = arch;
}

void laserUpdate(Archetype *arch, f32 dt)
{
    if (!arch || arch->arena[0].count == 0) return;

    u32 chunkCount = (arch->arena[0].count + arch->chunkCapacity - 1) / arch->chunkCapacity;

    for (u32 c = 0; c < chunkCount; c++)
    {
        void **fields = getArchetypeFields(arch, c);
        if (!fields) continue;

        b8  *Alive    = (b8  *)fields[LASER_ALIVE];
        f32 *PosX     = (f32 *)fields[LASER_POSITION_X];
        f32 *PosY     = (f32 *)fields[LASER_POSITION_Y];
        f32 *PosZ     = (f32 *)fields[LASER_POSITION_Z];
        f32 *VelX     = (f32 *)fields[LASER_VELOCITY_X];
        f32 *VelY     = (f32 *)fields[LASER_VELOCITY_Y];
        f32 *VelZ     = (f32 *)fields[LASER_VELOCITY_Z];
        f32 *Lifetime = (f32 *)fields[LASER_LIFETIME];

        u32 localCount = arch->chunkCapacity;
        if (c == chunkCount - 1)
            localCount = arch->arena[0].count - c * arch->chunkCapacity;

        for (u32 i = 0; i < localCount; i++)
        {
            if (!Alive[i]) continue;

            Lifetime[i] -= dt;
            if (Lifetime[i] <= 0.0f)
            {
                archetypePoolDespawn(arch, c * arch->chunkCapacity + i);
                continue;
            }

            PosX[i] += VelX[i] * dt;
            PosY[i] += VelY[i] * dt;
            PosZ[i] += VelZ[i] * dt;
        }
    }
}

void laserDestroy(void)
{
    s_arch = NULL;
}

Archetype    *laserGetArchetype(void) { return s_arch; }
StructLayout *laserGetLayout(void)    { return &Laser_layout; }

#define LASER_COLLIDER_RADIUS 2.0f

void laserFire(Vec3 position, Vec3 direction)
{
    if (!s_arch) return;

    u32 poolIdx = prefabSpawn(s_arch, 0, position);
    if (poolIdx == (u32)-1) return;

    u32 chunkIdx = poolIdx / s_arch->chunkCapacity;
    u32 localIdx = poolIdx % s_arch->chunkCapacity;

    void **fields = getArchetypeFields(s_arch, chunkIdx);
    if (!fields) return;

    ((f32 *)fields[LASER_VELOCITY_X])[localIdx] = direction.x * LASER_SPEED;
    ((f32 *)fields[LASER_VELOCITY_Y])[localIdx] = direction.y * LASER_SPEED;
    ((f32 *)fields[LASER_VELOCITY_Z])[localIdx] = direction.z * LASER_SPEED;
    ((f32 *)fields[LASER_LIFETIME]  )[localIdx] = LASER_LIFETIME_MAX;

    // Set emissive for bloom once, on first fire
    static b8 s_bloomSet = 0;
    if (!s_bloomSet)
    {
        u32 modelId = ((u32 *)fields[LASER_MODEL_ID])[localIdx];
        Model *m = resGetModel(modelId);
        if (m && m->materialCount > 0)
        {
            Material *mat = resGetMaterial(m->materialIndices[0]);
            if (mat) mat->emissive = 8.0f;
        }
        s_bloomSet = 1;
    }
}

void laserCheckCollisions(Archetype *asteroids)
{
    if (!s_arch || !asteroids) return;

    u32 laserChunks = s_arch->activeChunkCount;
    for (u32 lc = 0; lc < laserChunks; lc++)
    {
        void **lf = getArchetypeFields(s_arch, lc);
        if (!lf) continue;
        u32 lCount = s_arch->arena[lc].count;

        b8  *lAlive = (b8  *)lf[LASER_ALIVE];
        f32 *lPosX  = (f32 *)lf[LASER_POSITION_X];
        f32 *lPosY  = (f32 *)lf[LASER_POSITION_Y];
        f32 *lPosZ  = (f32 *)lf[LASER_POSITION_Z];
        f32 *lVelX  = (f32 *)lf[LASER_VELOCITY_X];
        f32 *lVelY  = (f32 *)lf[LASER_VELOCITY_Y];
        f32 *lVelZ  = (f32 *)lf[LASER_VELOCITY_Z];

        for (u32 li = 0; li < lCount; li++)
        {
            if (!lAlive[li]) continue;

            for (u32 ac = 0; ac < asteroids->activeChunkCount; ac++)
            {
                void **af = getArchetypeFields(asteroids, ac);
                if (!af) continue;
                u32 aCount = asteroids->arena[ac].count;

                b8  *aAlive  = (b8  *)af[ASTEROID_ALIVE];
                f32 *aPosX   = (f32 *)af[ASTEROID_POSITION_X];
                f32 *aPosY   = (f32 *)af[ASTEROID_POSITION_Y];
                f32 *aPosZ   = (f32 *)af[ASTEROID_POSITION_Z];
                f32 *aRadius = (f32 *)af[ASTEROID_SPHERE_RADIUS];
                f32 *aHealth = (f32 *)af[ASTEROID_HEALTH];
                f32 *aVelX   = (f32 *)af[ASTEROID_LINEAR_VELOCITY_X];
                f32 *aVelY   = (f32 *)af[ASTEROID_LINEAR_VELOCITY_Y];
                f32 *aVelZ   = (f32 *)af[ASTEROID_LINEAR_VELOCITY_Z];

                for (u32 ai = 0; ai < aCount; ai++)
                {
                    if (!aAlive[ai]) continue;

                    f32 dx = lPosX[li] - aPosX[ai];
                    f32 dy = lPosY[li] - aPosY[ai];
                    f32 dz = lPosZ[li] - aPosZ[ai];
                    f32 distSq = dx*dx + dy*dy + dz*dz;
                    f32 rSum = LASER_COLLIDER_RADIUS + aRadius[ai];

                    if (distSq < rSum * rSum)
                    {
                        // Push asteroid in laser direction
                        f32 lSpd = sqrtf(lVelX[li]*lVelX[li] + lVelY[li]*lVelY[li] + lVelZ[li]*lVelZ[li]);
                        if (lSpd > 0.0f)
                        {
                            f32 inv = LASER_PUSH / lSpd;
                            aVelX[ai] += lVelX[li] * inv;
                            aVelY[ai] += lVelY[li] * inv;
                            aVelZ[ai] += lVelZ[li] * inv;
                        }

                        aHealth[ai] -= 1.0f;
                        Vec3 aPos = {aPosX[ai], aPosY[ai], aPosZ[ai]};
                        if (aHealth[ai] <= 0.0f)
                        {
                            gameAudioAsteroidDestroy(aPos);
                            archetypePoolDespawn(asteroids, ac * asteroids->chunkCapacity + ai);
                        }
                        else
                        {
                            gameAudioAsteroidImpact(aPos);
                        }

                        archetypePoolDespawn(s_arch, lc * s_arch->chunkCapacity + li);
                        goto next_laser;
                    }
                }
            }
            next_laser:;
        }
    }
}

static void laserInitPlugin(void) {}

void druidGetECSSystem_Laser(ECSSystemPlugin *out)
{
    out->init    = laserInitPlugin;
    out->update  = laserUpdate;
    out->render  = NULL;
    out->destroy = laserDestroy;
}
