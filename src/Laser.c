#define DRUID_SYSTEM_EXPORT
#include "Laser.h"
#include "Asteroid.h"
#include "GameAudio.h"
#include <math.h>
#include <string.h>

#define LASER_PUSH 15.0f

// Rotate from unit vector 'from' to unit vector 'to'
static Vec4 quatFromTo(Vec3 from, Vec3 to)
{
    Vec3 axis = v3Cross(from, to);
    f32  w    = 1.0f + v3Dot(from, to);
    if (w < 0.0001f)
    {
        // Anti-parallel — pick any perpendicular axis and rotate 180 degrees
        Vec3 perp = (from.x * from.x < 0.9f) ? (Vec3){1,0,0} : (Vec3){0,1,0};
        axis = v3Norm(v3Cross(from, perp));
        return (Vec4){axis.x, axis.y, axis.z, 0.0f};
    }
    return quatNormalize((Vec4){axis.x, axis.y, axis.z, w});
}

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

void laserFire(Vec3 position, Vec3 direction, Vec4 cameraOrientation)
{
    if (!s_arch) return;

    u32 poolIdx = prefabSpawn(s_arch, 0, position);
    if (poolIdx == (u32)-1) return;

    u32 chunkIdx = poolIdx / s_arch->chunkCapacity;
    u32 localIdx = poolIdx % s_arch->chunkCapacity;

    void **fields = getArchetypeFields(s_arch, chunkIdx);
    if (!fields) return;

    Vec3 fireDir = v3Norm(direction);

    ((f32 *)fields[LASER_VELOCITY_X])[localIdx] = direction.x * LASER_SPEED;
    ((f32 *)fields[LASER_VELOCITY_Y])[localIdx] = direction.y * LASER_SPEED;
    ((f32 *)fields[LASER_VELOCITY_Z])[localIdx] = direction.z * LASER_SPEED;
    ((f32 *)fields[LASER_LIFETIME]  )[localIdx] = LASER_LIFETIME_MAX;
    ((Vec3*)fields[LASER_SCALE]     )[localIdx] = (Vec3){0.0625f, 0.09375f, 0.1875f};

    // Build laser rotation: laser's +Y points in fire direction, and rotation is consistent with camera
    // Get camera's local axes
    Vec3 camFwd   = quatRotateVec3(cameraOrientation, v3Forward);
    Vec3 camRight = quatRotateVec3(cameraOrientation, v3Right);
    Vec3 camUp    = quatRotateVec3(cameraOrientation, v3Up);

    // Laser's Z-axis points in fire direction (standard graphics convention: +Z is forward)
    Vec3 laserZ = fireDir;

    // Laser's X-axis: project camera right onto plane perpendicular to fire direction, then normalize
    Vec3 laserX = v3Sub(camRight, v3Scale(fireDir, v3Dot(camRight, fireDir)));
    f32 laserXLen = v3Mag(laserX);
    if (laserXLen > 0.001f)
    {
        laserX = v3Scale(laserX, 1.0f / laserXLen);
    }
    else
    {
        // Degenerate case: camera right is parallel to fire direction, use camera up instead
        laserX = v3Sub(camUp, v3Scale(fireDir, v3Dot(camUp, fireDir)));
        laserXLen = v3Mag(laserX);
        if (laserXLen > 0.001f)
        {
            laserX = v3Scale(laserX, 1.0f / laserXLen);
        }
        else
        {
            // Last resort: pick any perpendicular axis
            laserX = (fireDir.x * fireDir.x < 0.9f) ? (Vec3){1,0,0} : (Vec3){0,1,0};
            laserX = v3Norm(v3Cross(fireDir, laserX));
        }
    }

    // Laser's Y-axis: right-hand rule (cross product)
    Vec3 laserY = v3Cross(laserZ, laserX);

    // Convert rotation frame (axes) to quaternion using Shepperd's method
    // We build a 3x3 rotation matrix where the columns are laserX, laserY, laserZ
    Vec4 laserQuat = quatIdentity();
    {
        // Extract 3x3 rotation matrix values
        f32 m00 = laserX.x, m10 = laserX.y, m20 = laserX.z;
        f32 m01 = laserY.x, m11 = laserY.y, m21 = laserY.z;
        f32 m02 = laserZ.x, m12 = laserZ.y, m22 = laserZ.z;

        // Shepperd's method for robust matrix-to-quaternion conversion
        f32 trace = m00 + m11 + m22;
        if (trace > 0.0f)
        {
            f32 s = 0.5f / sqrtf(trace + 1.0f);
            laserQuat.w = 0.25f / s;
            laserQuat.x = (m21 - m12) * s;
            laserQuat.y = (m02 - m20) * s;
            laserQuat.z = (m10 - m01) * s;
        }
        else if (m00 > m11 && m00 > m22)
        {
            f32 s = 2.0f * sqrtf(1.0f + m00 - m11 - m22);
            laserQuat.w = (m21 - m12) / s;
            laserQuat.x = 0.25f * s;
            laserQuat.y = (m01 + m10) / s;
            laserQuat.z = (m02 + m20) / s;
        }
        else if (m11 > m22)
        {
            f32 s = 2.0f * sqrtf(1.0f + m11 - m00 - m22);
            laserQuat.w = (m02 - m20) / s;
            laserQuat.x = (m01 + m10) / s;
            laserQuat.y = 0.25f * s;
            laserQuat.z = (m12 + m21) / s;
        }
        else
        {
            f32 s = 2.0f * sqrtf(1.0f + m22 - m00 - m11);
            laserQuat.w = (m10 - m01) / s;
            laserQuat.x = (m02 + m20) / s;
            laserQuat.y = (m12 + m21) / s;
            laserQuat.z = 0.25f * s;
        }
    }

    // Apply 90-degree rotation around X-axis to match laser model orientation
    Vec4 rot90X = {0.7071067f, 0.0f, 0.0f, 0.7071067f}; // sin(45°), 0, 0, cos(45°)
    laserQuat = quatMul(laserQuat, rot90X);

    ((Vec4*)fields[LASER_ROTATION])[localIdx] = laserQuat;

    // Find and lock in the correct laser model once — search for "enchanted-crystal"
    static u32 s_laserModelId = (u32)-1;
    if (s_laserModelId == (u32)-1)
    {
        for (u32 id = 0; id < 4096; id++)
        {
            Model *m = resGetModel(id);
            if (!m || !m->name) break;
            if (strstr(m->name, "enchanted-crystal") || strstr(m->name, "crystal"))
            {
                s_laserModelId = id;
                if (m->materialCount > 0)
                {
                    Material *mat = resGetMaterial(m->materialIndices[0]);
                    if (mat) mat->emissive = 8.0f;
                }
                break;
            }
        }
    }
    if (s_laserModelId != (u32)-1)
        ((u32 *)fields[LASER_MODEL_ID])[localIdx] = s_laserModelId;
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
