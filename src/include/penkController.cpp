#ifndef PENK_CONTROLLER_CPP
#define PENK_CONTROLLER_CPP

#include <raylib.h>
#include <raymath.h>
#include "penk.cpp"

struct MovementKeys {
    int forward = KEY_W;
    int backward = KEY_S;
    int left = KEY_D;
    int right = KEY_A;
};

struct Controller {
    Camera *camera;
    MovementKeys movement_keys;
    float walk_speed = 2.5f;
    float run_speed = 3.f;
    float look_sensitivity = .5f;
    float look_x_limit = 45.f;
    Vector3 move_direction = Vector3Zero();
    float rotation_x = 0.f;
    bool can_move = true;
    bool running = false;
    float target_distance;
    Vector2 angle;
    float eyes_position = 0.5f;
    float look_min_clamp = 89.0f;
    float look_max_clamp = -89.0f;
    bool swing = true;
};

#ifndef PI
    #define PI 3.14159265358979323846
#endif

#ifndef DEG2RAD
    #define DEG2RAD (PI/180.0f)
#endif

#ifndef RAD2DEG
    #define RAD2DEG (180.0f/PI)
#endif

void SwitchToController(Controller *controller) {
    Vector3 v1 = controller->camera->position;
    Vector3 v2 = controller->camera->target;

    float dx = v2.x - v1.x;
    float dy = v2.y - v1.y;
    float dz = v2.z - v1.z;

    controller->target_distance = sqrtf(dx*dx + dy*dy + dz*dz);

    controller->angle.x = atan2f(dx, dz);
    controller->angle.y = atan2f(dy, sqrtf(dx*dx + dz*dz));
    DisableCursor();
}

Vector3 right = {0.f, 0.f, 1.f};
Vector3 left = {0.f, 0.f, -1.f};

#define MOVE_FRONT  0
#define MOVE_BACK   1
#define MOVE_LEFT   2
#define MOVE_RIGHT  3

int sine_time = GetTime();

void UpdateController(Controller *controller) {
    bool directions[4] = {IsKeyDown(controller->movement_keys.forward),
                          IsKeyDown(controller->movement_keys.backward),
                          IsKeyDown(controller->movement_keys.right),
                          IsKeyDown(controller->movement_keys.left)};

    Vector2 mouse_position_delta = GetMouseDelta();

    controller->running = IsKeyDown(KEY_LEFT_SHIFT);
    controller->camera->position.x += (sinf(controller->angle.x)*directions[MOVE_BACK] -
                            sinf(controller->angle.x)*directions[MOVE_FRONT] -
                            cosf(controller->angle.x)*directions[MOVE_LEFT] +
                            cosf(controller->angle.x)*directions[MOVE_RIGHT])*(controller->running ? controller->run_speed : controller->walk_speed)*GetFrameTime();

    controller->camera->position.z += (cosf(controller->angle.x)*directions[MOVE_BACK] -
                           cosf(controller->angle.x)*directions[MOVE_FRONT] +
                           sinf(controller->angle.x)*directions[MOVE_LEFT] -
                           sinf(controller->angle.x)*directions[MOVE_RIGHT])*(controller->running ? controller->run_speed : controller->walk_speed)*GetFrameTime();

    controller->angle.x -= mouse_position_delta.x*controller->look_sensitivity*GetFrameTime();
    controller->angle.y -= mouse_position_delta.y*controller->look_sensitivity*GetFrameTime();

    if (controller->angle.y > controller->look_min_clamp*DEG2RAD) controller->angle.y = controller->look_min_clamp*DEG2RAD;
    else if (controller->angle.y < controller->look_max_clamp*DEG2RAD) controller->angle.y = controller->look_max_clamp*DEG2RAD;

    Matrix matrix_translation = { 1.0f, 0.0f, 0.0f, 0.0f,
                            0.0f, 1.0f, 0.0f, 0.0f,
                            0.0f, 0.0f, 1.0f, (controller->target_distance/5.1f),
                            0.0f, 0.0f, 0.0f, 1.0f };

    Matrix matrix_rotation = { 1.0f, 0.0f, 0.0f, 0.0f,
                           0.0f, 1.0f, 0.0f, 0.0f,
                           0.0f, 0.0f, 1.0f, 0.0f,
                           0.0f, 0.0f, 0.0f, 1.0f };

    float cosz = cosf(0.0f);
    float sinz = sinf(0.0f);
    float cosy = cosf(-(PI*2 - controller->angle.x));
    float siny = sinf(-(PI*2 - controller->angle.x));
    float cosx = cosf(-(PI*2 - controller->angle.y));
    float sinx = sinf(-(PI*2 - controller->angle.y));

    matrix_rotation.m0 = cosz*cosy;
    matrix_rotation.m4 = (cosz*siny*sinx) - (sinz*cosx);
    matrix_rotation.m8 = (cosz*siny*cosx) + (sinz*sinx);
    matrix_rotation.m1 = sinz*cosy;
    matrix_rotation.m5 = (sinz*siny*sinx) + (cosz*cosx);
    matrix_rotation.m9 = (sinz*siny*cosx) - (cosz*sinx);
    matrix_rotation.m2 = -siny;
    matrix_rotation.m6 = cosy*sinx;
    matrix_rotation.m10= cosy*cosx;

    Matrix matrix_transform = { 0 };
    matrix_transform.m0 = matrix_translation.m0*matrix_rotation.m0 + matrix_translation.m1*matrix_rotation.m4 + matrix_translation.m2*matrix_rotation.m8 + matrix_translation.m3*matrix_rotation.m12;
    matrix_transform.m1 = matrix_translation.m0*matrix_rotation.m1 + matrix_translation.m1*matrix_rotation.m5 + matrix_translation.m2*matrix_rotation.m9 + matrix_translation.m3*matrix_rotation.m13;
    matrix_transform.m2 = matrix_translation.m0*matrix_rotation.m2 + matrix_translation.m1*matrix_rotation.m6 + matrix_translation.m2*matrix_rotation.m10 + matrix_translation.m3*matrix_rotation.m14;
    matrix_transform.m3 = matrix_translation.m0*matrix_rotation.m3 + matrix_translation.m1*matrix_rotation.m7 + matrix_translation.m2*matrix_rotation.m11 + matrix_translation.m3*matrix_rotation.m15;
    matrix_transform.m4 = matrix_translation.m4*matrix_rotation.m0 + matrix_translation.m5*matrix_rotation.m4 + matrix_translation.m6*matrix_rotation.m8 + matrix_translation.m7*matrix_rotation.m12;
    matrix_transform.m5 = matrix_translation.m4*matrix_rotation.m1 + matrix_translation.m5*matrix_rotation.m5 + matrix_translation.m6*matrix_rotation.m9 + matrix_translation.m7*matrix_rotation.m13;
    matrix_transform.m6 = matrix_translation.m4*matrix_rotation.m2 + matrix_translation.m5*matrix_rotation.m6 + matrix_translation.m6*matrix_rotation.m10 + matrix_translation.m7*matrix_rotation.m14;
    matrix_transform.m7 = matrix_translation.m4*matrix_rotation.m3 + matrix_translation.m5*matrix_rotation.m7 + matrix_translation.m6*matrix_rotation.m11 + matrix_translation.m7*matrix_rotation.m15;
    matrix_transform.m8 = matrix_translation.m8*matrix_rotation.m0 + matrix_translation.m9*matrix_rotation.m4 + matrix_translation.m10*matrix_rotation.m8 + matrix_translation.m11*matrix_rotation.m12;
    matrix_transform.m9 = matrix_translation.m8*matrix_rotation.m1 + matrix_translation.m9*matrix_rotation.m5 + matrix_translation.m10*matrix_rotation.m9 + matrix_translation.m11*matrix_rotation.m13;
    matrix_transform.m10 = matrix_translation.m8*matrix_rotation.m2 + matrix_translation.m9*matrix_rotation.m6 + matrix_translation.m10*matrix_rotation.m10 + matrix_translation.m11*matrix_rotation.m14;
    matrix_transform.m11 = matrix_translation.m8*matrix_rotation.m3 + matrix_translation.m9*matrix_rotation.m7 + matrix_translation.m10*matrix_rotation.m11 + matrix_translation.m11*matrix_rotation.m15;
    matrix_transform.m12 = matrix_translation.m12*matrix_rotation.m0 + matrix_translation.m13*matrix_rotation.m4 + matrix_translation.m14*matrix_rotation.m8 + matrix_translation.m15*matrix_rotation.m12;
    matrix_transform.m13 = matrix_translation.m12*matrix_rotation.m1 + matrix_translation.m13*matrix_rotation.m5 + matrix_translation.m14*matrix_rotation.m9 + matrix_translation.m15*matrix_rotation.m13;
    matrix_transform.m14 = matrix_translation.m12*matrix_rotation.m2 + matrix_translation.m13*matrix_rotation.m6 + matrix_translation.m14*matrix_rotation.m10 + matrix_translation.m15*matrix_rotation.m14;
    matrix_transform.m15 = matrix_translation.m12*matrix_rotation.m3 + matrix_translation.m13*matrix_rotation.m7 + matrix_translation.m14*matrix_rotation.m11 + matrix_translation.m15*matrix_rotation.m15;

    controller->camera->target.x = controller->camera->position.x - matrix_transform.m12;
    controller->camera->target.y = controller->camera->position.y - matrix_transform.m13;
    controller->camera->target.z = controller->camera->position.z - matrix_transform.m14;

    bool pressing_key = false;
    for(int index = 0; index < 4; index++) {
        if(directions[index]) {
            pressing_key = true;
            break;
        }
    }

    if(pressing_key) {
        controller->camera->position.y = sine_between(.95f, 1.05f, sine_time != -1 ? sine_time : GetTime()*8);
        sine_time = -1;
    } else {
        if(sine_time == -1) {
            sine_time = GetTime()*8;
        }
    }
}

#endif // PENK_CONTROLLER_CPP