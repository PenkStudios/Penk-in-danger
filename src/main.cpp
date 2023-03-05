/*
Kinda bad code
*/

#include <vector>
#include <time.h>
#include <sys/stat.h>

bool debug_menu = false;

#include "penk.cpp"
#include "penkGraphics.cpp"

#include "settings.cpp"
#include "spacemap.cpp"
#include "debug.cpp"

#include "penkController.cpp"
#include "path.cpp"

std::vector<PenkVector2> positions;
std::vector<std::vector<Furniture>> furniture_positions;

struct stat info;

bool EnvironmentCheck() {
    const char* deps[] = {
        "resources/atlas.png",
        "resources/wireframe.png",
        "resources/temp/",
        "resources/teleport.obj",
        "resources/teleport.mtl",
        "resources/redboy.obj",
        "resources/redboy.png",
        "resources/teleport_hand.png",
        "resources/font.png",
        "resources/house.obj"
    };
    for(const char* dep : deps) {
        if(stat(dep, &info) != 0) return false;
    }
    return true;
}

void GameInit() {
    srand(time(NULL));

    printf("[PENK] Checking environment...");

    if(!EnvironmentCheck()) {
        printf(" BAD\n[PENK][FATAL] Bad environment\n");
        exit(1);
    } else {
        printf(" OK\n");
    }

    InitWindow(width, height, window_title);
    CenterWindow(width, height);
}

Image redboy_image;
Texture redboy_texture;

enum Scene {MENU, GAME};
Scene scene = MENU;

Path::EnemyPosition enemyPosition;

void SwitchToGame(Camera *camera, Controller *controller) {
    controller->camera->position = (Vector3){(float)map_size/2, 1.0f, (float)map_size/2};
    SwitchToController(controller, PENK_FIRST_PERSON);
    scene = GAME;
    camera->position = (Vector3){(float)map_size/2, 1.0f, (float)map_size/2};
    enemyPosition.position = PenkWorldVector2 {(float)positions[current_layer].y, (float)positions[current_layer].x};
}

void SwitchToMenu(Camera *camera, Controller *controller) {
    scene = MENU;
    camera->position = (Vector3){-0.3f, 2.0f, 0.0f};
    camera->target = (Vector3){0.0f, 0.0f, 0.0f};
    SwitchToController(controller, PENK_ORBITAL);
}

Path::PathMap pathMap;

void NextLayer(Camera *camera) {
    positions[current_layer] = PenkVector2 {(int)camera->position.z, (int)camera->position.x};
    pathMap = Path::PathMap{};
    current_layer++;
    enemyPosition.position = PenkWorldVector2 {(float)positions[current_layer].y, (float)positions[current_layer].x};
}

enum HeldObject {HO_NOTHING, HO_TELEPORT};
HeldObject held_object = HO_NOTHING;

Model teleport;

void DrawItem(Camera3D camera) {
    Model to_render;
    switch(held_object) {
        case HO_TELEPORT:
            to_render = teleport;
            break;
        case HO_NOTHING: return;
    }

    Vector3 position = Vector3Add(Vector3Divide(Vector3Subtract(camera.target, camera.position), (Vector3){5, 5, 5}), camera.position);

    int rotation = YRotateTowards(position, camera.position);

    // printf("%f, %f, %f\n", rotation.x, rotation.y, rotation.z);

    DrawModelEx(to_render, position, (Vector3){0.f, 1.f, 0.f}, rotation * 100, (Vector3){.1f, .1f, .1f}, WHITE);
}

int main(int argc, char** argv) {
    GameInit();
    
    printf("[PENK] Preparing to call 'createSpaceMap'...\n");
    SpaceMapType space_data = CreateSpaceMap(map_size, layers);
    positions = space_data.positions;
    furniture_positions = space_data.furniture;

    Camera camera = { { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f, 0 };

    Controller controller;
    controller.camera = &camera;

    teleport = LoadModel("resources/teleport.obj");

    Model redboy = LoadModel("resources/redboy.obj");
    Model house = LoadModel("resources/house.obj");

    redboy_texture = LoadTexture("resources/redboy.png");
    redboy.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = redboy_texture;

    ButtonView button;
    button.normal = LoadTexture("resources/button.png");
    button.disabled =  LoadTexture("resources/button_disabled.png");
    button.clicked = LoadTexture("resources/button_click.png");

    Texture title = LoadTexture("resources/title.png");

    Button new_game_button;
    new_game_button.view = button;

    Button new_server_button;
    new_server_button.view = button;

    Button join_server_button;
    join_server_button.view = button;

    Button settings_button;
    settings_button.view = button;

    base_font = LoadFont("resources/font.png");

    SwitchToMenu(&camera, &controller);

    current_teleport_position = positions[current_layer];

    std::vector<Model> furniture = LoadFurniture();

    SetTargetFPS(fps);

    SetExitKey(0);

    while(!WindowShouldClose()) {
        switch(scene) {
            case MENU: {
                BeginDrawing(); {
                    ClearBackground(Color{10, 128, 180, 255});
                    UpdateController(&controller);

                    BeginMode3D(camera); {
                        DrawModel(house, (Vector3){0, 0, 0}, .1f, WHITE);
                    } EndMode3D();
                    DrawTextureScale(title, (Vector2){(float)width / 2, 150.f}, (Vector2){6.7f, 6.2f}, WHITE);
                    DrawButton(&new_game_button, (Vector2){(float)width / 2 - 155, 325.f}, (Vector2){.6f, .6f}, "New game", WHITE, 3.f);
                    DrawButton(&new_server_button, (Vector2){(float)width / 2 + 155, 325.f}, (Vector2){.6f, .6f}, "New server", WHITE, 3.f);
                    DrawButton(&join_server_button, (Vector2){(float)width / 2 - 155, 425.f}, (Vector2){.6f, .6f}, "Join server", WHITE, 3.f);
                    DrawButton(&settings_button, (Vector2){(float)width / 2 + 155, 425.f}, (Vector2){.6f, .6f}, "Settings", WHITE, 3.f);
                    if(IsButtonPressed(new_game_button)) {
                        SwitchToGame(&camera, &controller);
                    }
                } EndDrawing();
                break;
            }

            case GAME: {
                Vector3 old_camera_position = camera.position;
                UpdateController(&controller);

                Vector2 playerPos = { camera.position.x, camera.position.z };
                float playerRadius = 0.1f;

                int playerCellX = (int)(playerPos.x + 0.5f);
                int playerCellY = (int)(playerPos.y + 0.5f);

                if (playerCellX < 0) playerCellX = 0;
                else if (playerCellX >= cubicmap_textures[current_layer].width) playerCellX = cubicmap_textures[current_layer].width - 1;

                if (playerCellY < 0) playerCellY = 0;
                else if (playerCellY >= cubicmap_textures[current_layer].height) playerCellY = cubicmap_textures[current_layer].height - 1;

                for (int y = 0; y < cubicmap_textures[current_layer].height; y++) {
                    for (int x = 0; x < cubicmap_textures[current_layer].width; x++) {
                        if ((map_pixels_vector[current_layer][y*cubicmap_textures[current_layer].width + x].r == 255) &&       // Collision: white pixel, only check R channel
                            (CheckCollisionCircleRec(playerPos, playerRadius,
                            (Rectangle){ 0 - 0.5f + x*1.0f, 0 - 0.5f + y*1.0f, 1.0f, 1.0f }))) {
                            camera.position = old_camera_position;
                        }
                    }
                }

                BeginDrawing(); {
                    ClearBackground(Color{50, 100, 150, 0});
                    BeginMode3D(camera); {
                        enemyPosition.Tick(map_pixels_vector[current_layer], map_size, &pathMap, 10);
                        DrawModelEx(redboy, (Vector3){enemyPosition.position.x, sine_between(.8f, 1.f, GetTime() * 10), enemyPosition.position.y}, (Vector3){0.f, 1.f, 0.f}, enemyPosition.GetAngle(), (Vector3){0.4f, 0.4f, 0.4f}, WHITE);
                        UpdateSpaceMap();
                        DrawFurniture(furniture, furniture_positions);
                        if(held_object == HO_TELEPORT) {
                            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                                held_object = HO_NOTHING;
                                positions[current_layer] = PenkVector2 {(int)camera.position.z, (int)camera.position.x};
                            } else if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                                held_object = HO_NOTHING;
                                NextLayer(&camera);
                                current_teleport_position = positions[current_layer];
                            }
                        } else if(held_object == HO_NOTHING) {
                            float y = positions[current_layer].x;
                            float x = positions[current_layer].y;

                            bool collide = GetItemCollision((Vector3){x, 1.f, y}, 1.f, camera);

                            if(collide && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                                held_object = HO_TELEPORT;
                                positions[current_layer] = no_position;
                            }

                            DrawModel(teleport, (Vector3){x, sine_between(.6f, 0.9f, GetTime()*4), y}, .15f, WHITE);
                            teleport.transform = MatrixMultiply(teleport.transform, MatrixRotateY(-1.5f * GetFrameTime()));
                        }
                        DrawItem(camera);
                    } EndMode3D();

                    if(debug_menu) {
                        DrawTextureEx(cubicmap_textures[current_layer], (Vector2){ GetScreenWidth() - cubicmap_textures[current_layer].width*4.0f - 20, 20.0f }, 0.0f, 4.0f, WHITE);
                        DrawRectangleLines(GetScreenWidth() - cubicmap_textures[current_layer].width*4 - 20, 20, cubicmap_textures[current_layer].width*4, cubicmap_textures[current_layer].height*4, GREEN);

                        for(int x = 0; x < map_size; x++) {
                            for(int y = 0; y < map_size; y++) {
                                if(map_pixels_vector[current_layer][y * map_size + x].r != 255) {
                                    DrawRectangle(GetScreenWidth() - cubicmap_textures[current_layer].width*4 - 20 + x * 4, 20 + y * 4, 4, 4, Color{(unsigned char)(pathMap.map[y][x] / 50), (unsigned char)(pathMap.map[y][x] / 50), 0, 255});
                                }
                            }
                        }

                        DrawRectangle(GetScreenWidth() - cubicmap_textures[current_layer].width*4 - 20 + enemyPosition.position.x*4, 20 + enemyPosition.position.y*4, 4, 4, RED);
                        DrawRectangle(GetScreenWidth() - cubicmap_textures[current_layer].width*4 - 20 + camera.position.x*4, 20 + camera.position.z*4, 4, 4, GREEN);
                    }

                    DebugHandleKeys();
                } EndDrawing();
                break;
            }
        }
    }

    SpaceFree();

    UnloadModel(teleport);
    UnloadModel(redboy);
    UnloadModel(house);

    UnloadTexture(redboy_texture);
    UnloadTexture(title);
    
    UnloadButton(new_game_button);
    UnloadButton(new_server_button);
    UnloadButton(join_server_button);
    UnloadButton(settings_button);

    FurnitureFree(furniture);

    CloseWindow();
}
