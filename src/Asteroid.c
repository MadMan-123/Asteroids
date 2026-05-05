#define DRUID_SYSTEM_EXPORT
#include "Asteroid.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define NUM_ASTEROID_MODELS  43
#define SCALE_MIN            0.01f
#define SCALE_MAX            0.25f
#define SPIN_BASE            0.8f

// Attraction toward player
#define PULL_RADIUS          300.0f
#define PULL_RADIUS_SQ       (PULL_RADIUS * PULL_RADIUS)
#define PULL_FORCE           1.2f
#define PULL_MAX_SPEED       10.0f

// Player collision
#define PLAYER_RADIUS        10.0f
#define PLAYER_HIT_DAMAGE    20.0f
#define PLAYER_HIT_PUSH      60.0f
#define ASTEROID_PUSH        25.0f
#define HIT_COOLDOWN         1.5f
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

static Archetype *s_arch         = NULL;
static u32        s_modelIds[NUM_ASTEROID_MODELS];
static u32        s_modelCount   = 0;

// Player state (written by shipUpdate, read by asteroidUpdate)
static Vec3 s_playerPos          = {0.0f, 0.0f, 0.0f};
static f32  s_playerHitTimer     = 0.0f;
static f32  s_pendingDamage      = 0.0f;
static Vec3 s_pendingPush        = {0.0f, 0.0f, 0.0f};

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
    // Decay global player hit timer once per frame
    s_playerHitTimer -= dt;
    if (s_playerHitTimer < 0.0f) s_playerHitTimer = 0.0f;

    for (u32 _ch = 0; _ch < arch->activeChunkCount; _ch++)
    {
        void **fields = getArchetypeFields(arch, _ch);
        if (!fields) continue;
        u32 count = arch->arena[_ch].count;

        b8   *alive    = (b8   *)fields[ASTEROID_ALIVE];
        f32  *posX     = (f32  *)fields[ASTEROID_POSITION_X];
        f32  *posY     = (f32  *)fields[ASTEROID_POSITION_Y];
        f32  *posZ     = (f32  *)fields[ASTEROID_POSITION_Z];
        Vec4 *rot      = (Vec4 *)fields[ASTEROID_ROTATION];
        f32  *velX     = (f32  *)fields[ASTEROID_LINEAR_VELOCITY_X];
        f32  *velY     = (f32  *)fields[ASTEROID_LINEAR_VELOCITY_Y];
        f32  *velZ     = (f32  *)fields[ASTEROID_LINEAR_VELOCITY_Z];
        f32  *radius   = (f32  *)fields[ASTEROID_SPHERE_RADIUS];
        f32  *health   = (f32  *)fields[ASTEROID_HEALTH];
        f32  *hitTimer = (f32  *)fields[ASTEROID_HIT_TIMER];

        for (u32 i = 0; i < count; i++)
        {
            if (!alive[i]) continue;

            // Kill if health depleted
            if (health[i] <= 0.0f)
            {
                archetypePoolDespawn(arch, _ch * arch->chunkCapacity + i);
                continue;
            }

            // Decay per-asteroid player collision cooldown
            if (hitTimer[i] > 0.0f)
            {
                hitTimer[i] -= dt;
                if (hitTimer[i] < 0.0f) hitTimer[i] = 0.0f;
            }

            // Vector from asteroid to player
            f32 dx = s_playerPos.x - posX[i];
            f32 dy = s_playerPos.y - posY[i];
            f32 dz = s_playerPos.z - posZ[i];
            f32 distSq = dx*dx + dy*dy + dz*dz;
            f32 rSum   = radius[i] + PLAYER_RADIUS;

            if (distSq < rSum * rSum)
            {
                // Body collision with player
                if (s_playerHitTimer <= 0.0f && hitTimer[i] <= 0.0f)
                {
                    f32 dist    = sqrtf(distSq);
                    f32 invDist = dist > 0.0001f ? 1.0f / dist : 0.0f;

                    // Push asteroid away from player
                    velX[i] -= dx * invDist * ASTEROID_PUSH;
                    velY[i] -= dy * invDist * ASTEROID_PUSH;
                    velZ[i] -= dz * invDist * ASTEROID_PUSH;

                    // Queue player push (away from asteroid = toward player = +dx direction)
                    s_pendingPush.x  += dx * invDist * PLAYER_HIT_PUSH;
                    s_pendingPush.y  += dy * invDist * PLAYER_HIT_PUSH;
                    s_pendingPush.z  += dz * invDist * PLAYER_HIT_PUSH;
                    s_pendingDamage  += PLAYER_HIT_DAMAGE;
                    s_playerHitTimer  = HIT_COOLDOWN;
                    hitTimer[i]       = HIT_COOLDOWN;
                }
            }
            else if (distSq < PULL_RADIUS_SQ && distSq > 0.01f)
            {
                // Gravitational pull — linear falloff, speed capped
                f32 dist    = sqrtf(distSq);
                f32 falloff = 1.0f - dist / PULL_RADIUS;
                f32 force   = PULL_FORCE * falloff / dist;

                velX[i] += dx * force * dt;
                velY[i] += dy * force * dt;
                velZ[i] += dz * force * dt;

                f32 spd = sqrtf(velX[i]*velX[i] + velY[i]*velY[i] + velZ[i]*velZ[i]);
                if (spd > PULL_MAX_SPEED)
                {
                    f32 inv = PULL_MAX_SPEED / spd;
                    velX[i] *= inv; velY[i] *= inv; velZ[i] *= inv;
                }
            }

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

Archetype    *asteroidGetArchetype(void) { return s_arch; }
StructLayout *asteroidGetLayout(void)    { return &Asteroid_layout; }

void asteroidSetPlayerPos(Vec3 pos)
{
    s_playerPos = pos;
}

f32 asteroidConsumePlayerHit(Vec3 *outPush)
{
    f32 dmg = s_pendingDamage;
    if (outPush) *outPush = s_pendingPush;
    s_pendingDamage  = 0.0f;
    s_pendingPush    = (Vec3){0.0f, 0.0f, 0.0f};
    return dmg;
}

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

    // Health scales with size: small = 2 hits, large = 7 hits
    f32 t = (scale - SCALE_MIN) / (SCALE_MAX - SCALE_MIN);
    ((f32  *)fields[ASTEROID_HEALTH           ])[localIdx] = 2.0f + t * 5.0f;
    ((f32  *)fields[ASTEROID_HIT_TIMER        ])[localIdx] = 0.0f;
}

static void asteroidInitPlugin(void) {}

void druidGetECSSystem_Asteroid(ECSSystemPlugin *out)
{
    out->init    = asteroidInitPlugin;
    out->update  = asteroidUpdate;
    out->render  = NULL;
    out->destroy = asteroidDestroy;
}
