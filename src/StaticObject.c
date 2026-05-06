#define DRUID_SYSTEM_EXPORT
#include "StaticObject.h"

DEFINE_ARCHETYPE(StaticObject, STATICOBJECT_FIELDS)

static Archetype *s_arch = NULL;

void staticObjectInit(Archetype *arch)
{
    s_arch = arch;
}

void staticObjectUpdate(Archetype *arch, f32 dt)
{
    for (u32 _ch = 0; _ch < arch->activeChunkCount; _ch++)
    {
        void **fields = getArchetypeFields(arch, _ch);
        if (!fields) continue;
        u32 count = arch->arena[_ch].count;
        for (u32 i = 0; i < count; i++)
        {
        }
    }
}

void staticObjectDestroy(void)
{
    s_arch = NULL;
}

Archetype *staticObjectGetArchetype(void) { return s_arch; }

void staticObjectSpawn(Vec3 position)
{
    if (!s_arch) return;
    prefabSpawn(s_arch, 0, position);
}

// Optional render — uncomment and set out->render below to use.
/*
void staticObjectRender(Archetype *arch, Renderer *r) { (void)arch; (void)r; }
*/

static void staticObjectInitPlugin(void) {}

void druidGetECSSystem_StaticObject(ECSSystemPlugin *out)
{
    out->init    = staticObjectInitPlugin;
    out->update  = staticObjectUpdate;
    out->render  = NULL;
    out->destroy = staticObjectDestroy;
}
