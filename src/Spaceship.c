#define DRUID_SYSTEM_EXPORT
#include "Spaceship.h"

DEFINE_ARCHETYPE(Spaceship, SPACESHIP_FIELDS)

#define THRUST_FORCE_DEFAULT    60.0f
#define LINEAR_DAMPING_DEFAULT   0.8f
#define ANGULAR_ACCEL_DEFAULT    6.0f
#define ANGULAR_DAMPING_DEFAULT  2.0f

static Archetype *s_arch = NULL;

void shipInit(Archetype *arch)
{
    s_arch = arch;
}

void shipUpdate(Archetype *arch, f32 dt)
{
    void **fields = getArchetypeFields(arch, 0);
    if (!fields || arch->arena[0].count == 0) return;

    f32  *PosX           = (f32  *)fields[SHIP_POSITION_X];
    f32  *PosY           = (f32  *)fields[SHIP_POSITION_Y];
    f32  *PosZ           = (f32  *)fields[SHIP_POSITION_Z];
    Vec4 *Rot            = (Vec4 *)fields[SHIP_ROTATION];
    f32  *VelX           = (f32  *)fields[SHIP_LINEAR_VELOCITY_X];
    f32  *VelY           = (f32  *)fields[SHIP_LINEAR_VELOCITY_Y];
    f32  *VelZ           = (f32  *)fields[SHIP_LINEAR_VELOCITY_Z];
    f32  *YawRate        = (f32  *)fields[SHIP_YAW_RATE];
    f32  *PitchRate      = (f32  *)fields[SHIP_PITCH_RATE];
    f32  *RollRate       = (f32  *)fields[SHIP_ROLL_RATE];
    f32  *ThrustForce    = (f32  *)fields[SHIP_THRUST_FORCE];
    f32  *LinearDamping  = (f32  *)fields[SHIP_LINEAR_DAMPING];
    f32  *AngularAccel   = (f32  *)fields[SHIP_ANGULAR_ACCEL];
    f32  *AngularDamping = (f32  *)fields[SHIP_ANGULAR_DAMPING];

    if (ThrustForce[0]    == 0.0f) ThrustForce[0]    = THRUST_FORCE_DEFAULT;
    if (LinearDamping[0]  == 0.0f) LinearDamping[0]  = LINEAR_DAMPING_DEFAULT;
    if (AngularAccel[0]   == 0.0f) AngularAccel[0]   = ANGULAR_ACCEL_DEFAULT;
    if (AngularDamping[0] == 0.0f) AngularDamping[0] = ANGULAR_DAMPING_DEFAULT;

    //-------------------------------------------------------------------------
    // Rotation

    Vec2 trig = getJoystickAxis(0, JOYSTICK_TRIGGER_LEFT, JOYSTICK_TRIGGER_RIGHT);
    f32 thrustUp   = clamp(-trig.y, 0.0f, 1.0f);  // R2
    f32 thrustDown = clamp(-trig.x, 0.0f, 1.0f);  // L2

    f32 rollInput = 0.0f;
    if (isButtonDown(0, BUTTON_RIGHTSHOULDER)) rollInput += 1.0f;
    if (isButtonDown(0, BUTTON_LEFTSHOULDER))  rollInput -= 1.0f;

    Vec2 look = getJoystickAxis(0, JOYSTICK_RIGHT_X, JOYSTICK_RIGHT_Y);

    f32 angDamp = 1.0f - AngularDamping[0] * dt;
    if (angDamp < 0.0f) angDamp = 0.0f;

    YawRate[0]   = YawRate[0]   * angDamp + ( look.x   * AngularAccel[0]) * dt;
    PitchRate[0] = PitchRate[0] * angDamp + ( look.y   * AngularAccel[0]) * dt;
    RollRate[0]  = RollRate[0]  * angDamp + (rollInput  * AngularAccel[0]) * dt;

    Vec4 dYaw   = quatFromAxisAngle(v3Up,      YawRate[0]   * dt);
    Vec4 dPitch = quatFromAxisAngle(v3Right,   PitchRate[0] * dt);
    Vec4 dRoll  = quatFromAxisAngle(v3Forward, RollRate[0]  * dt);
    Rot[0] = quatNormalize(quatMul(quatMul(quatMul(Rot[0], dYaw), dPitch), dRoll));

    //-------------------------------------------------------------------------
    // Translation — fully manual: thrust → velocity → position.
    // Kinematic body so physics never touches velocity or position.

    f32 thrustFwd   = -yInputAxis;
    f32 thrustRight =  xInputAxis;
    f32 thrustVert  =  thrustUp - thrustDown;

    Vec3 localThrust = {
        thrustRight * ThrustForce[0],
        thrustVert  * ThrustForce[0],
        thrustFwd   * ThrustForce[0],
    };
    Vec3 worldThrust = quatRotateVec3(Rot[0], localThrust);

    // Accelerate
    VelX[0] += worldThrust.x * dt;
    VelY[0] += worldThrust.y * dt;
    VelZ[0] += worldThrust.z * dt;

    // Damp (exponential decay)
    f32 linDamp = 1.0f - LinearDamping[0] * dt;
    if (linDamp < 0.0f) linDamp = 0.0f;
    VelX[0] *= linDamp;
    VelY[0] *= linDamp;
    VelZ[0] *= linDamp;

    // Integrate position
    PosX[0] += VelX[0] * dt;
    PosY[0] += VelY[0] * dt;
    PosZ[0] += VelZ[0] * dt;

    //-------------------------------------------------------------------------
    // Camera

    if (runtime && runtime->camera)
    {
        runtime->camera->pos         = (Vec3){PosX[0], PosY[0], PosZ[0]};
        runtime->camera->orientation = Rot[0];
    }
}

void shipDestroy(void)
{
    s_arch = NULL;
}

Archetype *shipGetArchetype(void) { return s_arch; }

void shipSpawn(Vec3 position)
{
    if (!s_arch) return;

    u64 entity = 0;
    if (!createEntityInArchetype(s_arch, &entity)) return;

    void **fields = getArchetypeFields(s_arch, 0);
    if (!fields) return;

    ((f32  *)fields[SHIP_POSITION_X]        )[0] = position.x;
    ((f32  *)fields[SHIP_POSITION_Y]        )[0] = position.y;
    ((f32  *)fields[SHIP_POSITION_Z]        )[0] = position.z;
    ((Vec4 *)fields[SHIP_ROTATION]          )[0] = (Vec4){0.0f, 0.0f, 0.0f, 1.0f};
    ((Vec3 *)fields[SHIP_SCALE]             )[0] = (Vec3){1.0f, 1.0f, 1.0f};
    ((u32  *)fields[SHIP_PHYSICS_BODY_TYPE] )[0] = PHYS_BODY_KINEMATIC;
    ((f32  *)fields[SHIP_MASS]              )[0] = 1.0f;
    ((f32  *)fields[SHIP_INV_MASS]          )[0] = 1.0f;
    ((f32  *)fields[SHIP_LINEAR_DAMPING]    )[0] = LINEAR_DAMPING_DEFAULT;
    ((f32  *)fields[SHIP_SPHERE_RADIUS]     )[0] = 1.0f;
    ((f32  *)fields[SHIP_THRUST_FORCE]      )[0] = THRUST_FORCE_DEFAULT;
    ((f32  *)fields[SHIP_ANGULAR_ACCEL]     )[0] = ANGULAR_ACCEL_DEFAULT;
    ((f32  *)fields[SHIP_ANGULAR_DAMPING]   )[0] = ANGULAR_DAMPING_DEFAULT;
}

static void shipInitPlugin(void) {}

void druidGetECSSystem_Spaceship(ECSSystemPlugin *out)
{
    out->init    = shipInitPlugin;
    out->update  = shipUpdate;
    out->render  = NULL;
    out->destroy = shipDestroy;
}
