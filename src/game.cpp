#include "game.h"
#include "Asteroid.h"
#include "Spaceship.h"
#include "Laser.h"
#include "Crystal.h"
#include "GameAudio.h"
#include <math.h>
#include <stdlib.h>

static Archetype g_asteroidArch = {0};
static Archetype g_shipArch     = {0};
static Archetype g_laserArch    = {0};
static Archetype g_crystalArch  = {0};


static u32 g_bloomCapFBO      = 0;   // FBO wrapping the capture texture
static u32 g_bloomCapTex      = 0;   // full-res screen capture
static u32 g_bloomPingFBO[2]  = {0, 0};
static u32 g_bloomPingTex[2]  = {0, 0};
static u32 g_bloomBrightProg  = 0;
static u32 g_bloomBlurProg    = 0;
static u32 g_bloomCompProg    = 0;
static u32 g_bloomQuadVAO     = 0;
static u32 g_bloomQuadVBO     = 0;
static i32 g_bloomW = 0, g_bloomH = 0;
// Cached uniform locations (set once after shader load)
static i32 g_uBrightScene = -1, g_uBrightThresh = -1;
static i32 g_uBlurImage = -1, g_uBlurHoriz = -1;
static i32 g_uCompScene = -1, g_uCompBloom = -1, g_uCompStrength = -1;

static void bloomInit(i32 w, i32 h)
{
    g_bloomW = w; g_bloomH = h;

    // Full-res capture texture + FBO — we blit the finished frame into this
    glGenTextures(1, &g_bloomCapTex);
    glBindTexture(GL_TEXTURE_2D, g_bloomCapTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenFramebuffers(1, &g_bloomCapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, g_bloomCapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_bloomCapTex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Half-res ping-pong FBOs for blur
    i32 bw = w / 2, bh = h / 2;
    for (u32 i = 0; i < 2; i++)
    {
        glGenFramebuffers(1, &g_bloomPingFBO[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, g_bloomPingFBO[i]);
        glGenTextures(1, &g_bloomPingTex[i]);
        glBindTexture(GL_TEXTURE_2D, g_bloomPingTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, bw, bh, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_bloomPingTex[i], 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    g_bloomBrightProg = createGraphicsProgram("./res/bloom_bright.vert",    "./res/bloom_bright.frag");
    g_bloomBlurProg   = createGraphicsProgram("./res/bloom_blur.vert",      "./res/bloom_blur.frag");
    g_bloomCompProg   = createGraphicsProgram("./res/bloom_composite.vert", "./res/bloom_composite.frag");

    if (!g_bloomBrightProg) { ERROR("Bloom: failed to load bloom_bright shaders"); }
    if (!g_bloomBlurProg)   { ERROR("Bloom: failed to load bloom_blur shaders"); }
    if (!g_bloomCompProg)   { ERROR("Bloom: failed to load bloom_composite shaders"); }

    if (g_bloomBrightProg)
    {
        g_uBrightScene  = glGetUniformLocation(g_bloomBrightProg, "scene");
        g_uBrightThresh = glGetUniformLocation(g_bloomBrightProg, "u_threshold");
    }
    if (g_bloomBlurProg)
    {
        g_uBlurImage = glGetUniformLocation(g_bloomBlurProg, "image");
        g_uBlurHoriz = glGetUniformLocation(g_bloomBlurProg, "horizontal");
    }
    if (g_bloomCompProg)
    {
        g_uCompScene    = glGetUniformLocation(g_bloomCompProg, "scene");
        g_uCompBloom    = glGetUniformLocation(g_bloomCompProg, "bloomBlur");
        g_uCompStrength = glGetUniformLocation(g_bloomCompProg, "u_bloomStrength");
    }

    if (g_bloomBrightProg && g_bloomBlurProg && g_bloomCompProg)
        INFO("Bloom: initialised (%dx%d, half-res blur %dx%d)", w, h, bw, bh);

    f32 q[] = {
        -1.f, 1.f,  0.f,1.f,   -1.f,-1.f,  0.f,0.f,   1.f,-1.f,  1.f,0.f,
        -1.f, 1.f,  0.f,1.f,    1.f,-1.f,  1.f,0.f,   1.f, 1.f,  1.f,1.f,
    };
    glGenVertexArrays(1, &g_bloomQuadVAO);
    glGenBuffers(1, &g_bloomQuadVBO);
    glBindVertexArray(g_bloomQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_bloomQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(q), q, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(f32), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(f32), (void*)(2*sizeof(f32)));
    glBindVertexArray(0);
}

static void bloomApply(i32 w, i32 h)
{
    if (!g_bloomBrightProg || !g_bloomBlurProg || !g_bloomCompProg || !g_bloomQuadVAO) return;

    // 1. Blit finished frame (FBO 0) → capture texture
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_bloomCapFBO);
    glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(g_bloomQuadVAO);

    // 2. Bright-extract at half-res → pingFBO[0]
    glBindFramebuffer(GL_FRAMEBUFFER, g_bloomPingFBO[0]);
    glViewport(0, 0, w/2, h/2);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(g_bloomBrightProg);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_bloomCapTex);
    glUniform1i(g_uBrightScene,  0);
    glUniform1f(g_uBrightThresh, 0.5f);   // low threshold — catches more of the glow
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // 3. Ping-pong Gaussian blur (10 passes)
    glUseProgram(g_bloomBlurProg);
    glUniform1i(g_uBlurImage, 0);
    u32 horiz = 1;
    for (u32 i = 0; i < 10; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, g_bloomPingFBO[horiz]);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, g_bloomPingTex[horiz ^ 1]);
        glUniform1i(g_uBlurHoriz, (i32)horiz);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        horiz ^= 1;
    }
    // result is in g_bloomPingTex[0]

    // 4. Composite (scene + bloom) → screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, w, h);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(g_bloomCompProg);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_bloomCapTex);
    glUniform1i(g_uCompScene, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g_bloomPingTex[0]);
    glUniform1i(g_uCompBloom,    1);
    glUniform1f(g_uCompStrength, 2.0f);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

static b8 g_requestQuit = 0;
extern "C" void gameSignalQuit(void) { g_requestQuit = 1; }
static b8 gameRequestsQuit(void) { b8 r = g_requestQuit; g_requestQuit = 0; return r; }

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

    setMusicVolume(0.3f);
    playMusic("asteroidIDM.mp3", 1);

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
    gameAudioTick(dt);
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

    // Feed light positions into the deferred lighting shader
    if (runtime && runtime->lightingShader)
    {
        static i32 s_uLightPos       = -2;
        static i32 s_uCrystalLights  = -2;
        static i32 s_uCrystalCount   = -2;
        if (s_uLightPos == -2)
        {
            glUseProgram(runtime->lightingShader);
            s_uLightPos      = glGetUniformLocation(runtime->lightingShader, "u_lightPos");
            s_uCrystalLights = glGetUniformLocation(runtime->lightingShader, "u_crystalLights");
            s_uCrystalCount  = glGetUniformLocation(runtime->lightingShader, "u_crystalLightCount");
        }

        Vec3 sp = shipGetPos();
        glUseProgram(runtime->lightingShader);

        if (s_uLightPos >= 0)
            glUniform3f(s_uLightPos, sp.x, sp.y, sp.z);

        if (s_uCrystalLights >= 0)
        {
            Vec3 crystalPositions[8];
            u32  crystalCount = crystalGetNearestLights(sp, crystalPositions, 8);
            if (s_uCrystalCount >= 0)
                glUniform1i(s_uCrystalCount, (i32)crystalCount);
            if (crystalCount > 0)
                glUniform3fv(s_uCrystalLights, (i32)crystalCount, (f32*)crystalPositions);
        }
    }

    runtimeEndScenePass(runtime);

    if (runtime && runtime->standaloneMode && display)
    {
        i32 w = (i32)display->screenWidth;
        i32 h = (i32)display->screenHeight;
        if (!g_bloomCapTex) bloomInit(w, h);
        bloomApply(w, h);
    }
}

static void gameDestroy(void)
{
    if (g_bloomCapFBO)     { glDeleteFramebuffers(1,  &g_bloomCapFBO);     g_bloomCapFBO    = 0; }
    if (g_bloomCapTex)     { glDeleteTextures(1,      &g_bloomCapTex);     g_bloomCapTex    = 0; }
    for (u32 i = 0; i < 2; i++)
    {
        if (g_bloomPingFBO[i]) { glDeleteFramebuffers(1, &g_bloomPingFBO[i]); g_bloomPingFBO[i] = 0; }
        if (g_bloomPingTex[i]) { glDeleteTextures(1,     &g_bloomPingTex[i]); g_bloomPingTex[i] = 0; }
    }
    if (g_bloomBrightProg) { freeShader(g_bloomBrightProg); g_bloomBrightProg = 0; }
    if (g_bloomBlurProg)   { freeShader(g_bloomBlurProg);   g_bloomBlurProg   = 0; }
    if (g_bloomCompProg)   { freeShader(g_bloomCompProg);   g_bloomCompProg   = 0; }
    if (g_bloomQuadVAO)    { glDeleteVertexArrays(1, &g_bloomQuadVAO); g_bloomQuadVAO = 0; }
    if (g_bloomQuadVBO)    { glDeleteBuffers(1,       &g_bloomQuadVBO); g_bloomQuadVBO = 0; }

    shipDestroy();
    destroyArchetype(&g_shipArch);

    asteroidDestroy();
    destroyArchetype(&g_asteroidArch);

    laserDestroy();
    destroyArchetype(&g_laserArch);

    crystalDestroy();
    destroyArchetype(&g_crystalArch);

    stopMusic();
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
    out->init         = gameInit;
    out->update       = gameUpdate;
    out->render       = gameRender;
    out->destroy      = gameDestroy;
    out->requestsQuit = gameRequestsQuit;
}
