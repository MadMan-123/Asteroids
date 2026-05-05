#define DRUID_SYSTEM_EXPORT
#include "Crystal.h"
#include <math.h>
#include <string.h>

#define COLLECTION_RADIUS   40.0f
#define CRYSTAL_SPIN_RATE    0.5f
#define NUM_CRYSTAL_MODELS  16

DEFINE_ARCHETYPE(Crystal, CRYSTAL_FIELDS)

static Archetype *s_arch        = NULL;
static u32        s_modelIds[NUM_CRYSTAL_MODELS];
static u32        s_modelCount  = 0;
static b8         s_emissiveSet = 0;
static u32        s_collected   = 0;

static const Vec3 s_spinAxis = { 0.577f, 0.577f, 0.577f };

void crystalInit(Archetype *arch)
{
    s_arch        = arch;
    s_modelCount  = 0;
    s_emissiveSet = 0;
    s_collected   = 0;

    // Prefer non-asteroid models; fall back to any available model
    for (u32 id = 0; s_modelCount < NUM_CRYSTAL_MODELS; id++)
    {
        Model *m = resGetModel(id);
        if (!m || !m->name) break;
        if (!strstr(m->name, "asteroid"))
            s_modelIds[s_modelCount++] = id;
    }
    if (s_modelCount == 0)
    {
        for (u32 id = 0; s_modelCount < NUM_CRYSTAL_MODELS; id++)
        {
            Model *m = resGetModel(id);
            if (!m || !m->name) break;
            s_modelIds[s_modelCount++] = id;
        }
    }
}

void crystalUpdate(Archetype *arch, f32 dt)
{
    if (!arch) return;
    Vec4 dRot = quatFromAxisAngle(s_spinAxis, CRYSTAL_SPIN_RATE * dt);

    for (u32 c = 0; c < arch->activeChunkCount; c++)
    {
        void **fields = getArchetypeFields(arch, c);
        if (!fields) continue;
        u32   count = arch->arena[c].count;
        b8   *alive = (b8  *)fields[CRYSTAL_ALIVE];
        Vec4 *rot   = (Vec4*)fields[CRYSTAL_ROTATION];

        for (u32 i = 0; i < count; i++)
        {
            if (!alive[i]) continue;
            rot[i] = quatNormalize(quatMul(rot[i], dRot));
        }
    }
}

void crystalDestroy(void)
{
    s_arch = NULL;
}

Archetype    *crystalGetArchetype(void) { return s_arch; }
StructLayout *crystalGetLayout(void)    { return &Crystal_layout; }

void crystalSpawn(Vec3 position, f32 scale)
{
    if (!s_arch || s_modelCount == 0) return;

    u32 poolIdx = prefabSpawn(s_arch, 0, position);
    if (poolIdx == (u32)-1) return;

    u32 chunkIdx = poolIdx / s_arch->chunkCapacity;
    u32 localIdx = poolIdx % s_arch->chunkCapacity;
    void **fields = getArchetypeFields(s_arch, chunkIdx);
    if (!fields) return;

    u32 modelId = s_modelIds[poolIdx % s_modelCount];
    ((Vec3 *)fields[CRYSTAL_SCALE]   )[localIdx] = (Vec3){ scale, scale, scale };
    ((u32  *)fields[CRYSTAL_MODEL_ID])[localIdx] = modelId;
    ((Vec4 *)fields[CRYSTAL_ROTATION])[localIdx] = (Vec4){ 0.0f, 0.0f, 0.0f, 1.0f };

    // Sets emissive on first spawn — assign a unique model to the Crystal prefab
    // in the editor to avoid this affecting asteroid materials
    if (!s_emissiveSet)
    {
        Model *m = resGetModel(modelId);
        if (m && m->materialCount > 0)
        {
            Material *mat = resGetMaterial(m->materialIndices[0]);
            if (mat) mat->emissive = 15.0f;
        }
        s_emissiveSet = 1;
    }
}

void crystalCheckCollection(f32 playerX, f32 playerY, f32 playerZ)
{
    if (!s_arch) return;

    for (u32 c = 0; c < s_arch->activeChunkCount; c++)
    {
        void **fields = getArchetypeFields(s_arch, c);
        if (!fields) continue;
        u32  count = s_arch->arena[c].count;
        b8  *alive = (b8  *)fields[CRYSTAL_ALIVE];
        f32 *posX  = (f32 *)fields[CRYSTAL_POSITION_X];
        f32 *posY  = (f32 *)fields[CRYSTAL_POSITION_Y];
        f32 *posZ  = (f32 *)fields[CRYSTAL_POSITION_Z];

        for (u32 i = 0; i < count; i++)
        {
            if (!alive[i]) continue;
            f32 dx = playerX - posX[i];
            f32 dy = playerY - posY[i];
            f32 dz = playerZ - posZ[i];
            if (dx*dx + dy*dy + dz*dz < COLLECTION_RADIUS * COLLECTION_RADIUS)
            {
                archetypePoolDespawn(s_arch, c * s_arch->chunkCapacity + i);
                s_collected++;
                INFO("[crystal] collected %u", s_collected);
            }
        }
    }
}

static void crystalInitPlugin(void) {}

void druidGetECSSystem_Crystal(ECSSystemPlugin *out)
{
    out->init    = crystalInitPlugin;
    out->update  = crystalUpdate;
    out->render  = NULL;
    out->destroy = crystalDestroy;
}
