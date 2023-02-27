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

void SwitchToGame(Camera *camera, Controller *controller) {
    controller->camera->position = (Vector3){(float)blocks_x/2, 1.0f, (float)blocks_y/2};
    SwitchToController(controller, PENK_FIRST_PERSON);
    scene = GAME;
    camera->position = (Vector3){(float)blocks_x/2, 1.0f, (float)blocks_y/2};
}

void SwitchToMenu(Camera *camera, Controller *controller) {
    scene = MENU;
    camera->position = (Vector3){-0.3f, 2.0f, 0.0f};
    camera->target = (Vector3){0.0f, 0.0f, 0.0f};
    SwitchToController(controller, PENK_ORBITAL);
}

enum HeldObject {HO_NOTHING, HO_TELEPORT};
HeldObject held_object = HO_NOTHING;

Texture2D teleport_hand;

void DrawItem() {
    Texture to_draw;
    switch(held_object) {
        case HO_TELEPORT:
            to_draw = teleport_hand;
            break;
        case HO_NOTHING: return;
    }
    DrawTextureScale(teleport_hand, (Vector2){(float)width - to_draw.width * 1.f, (float)height - to_draw.height / 1.7f}, (Vector2){1.5f, 1.2f}, WHITE);
}

int main(int argc, char** argv) {
    GameInit();    

    printf("[PENK] Preparing to call 'createSpaceMap'...\n");
    SpaceMapType space_data = CreateSpaceMap(blocks_x, blocks_y, layers);
    positions = std::get<0>(space_data);
    furniture_positions = std::get<1>(space_data);

    Camera camera = { { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f, 0 };

    Controller controller;
    controller.camera = &camera;

    Model teleport = LoadModel("resources/teleport.obj");

    Model redboy = LoadModel("resources/redboy.obj");
    Model house = LoadModel("resources/house.obj");

    teleport_hand = LoadTexture("resources/teleport_hand.png");

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
                        UpdateSpaceMap();
                        DrawFurniture(furniture, furniture_positions);
                        if(held_object == HO_TELEPORT) {
                            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                                held_object = HO_NOTHING;
                                positions[current_layer] = std::make_pair(camera.position.z, camera.position.x);
                            } else if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                                held_object = HO_NOTHING;
                                positions[current_layer] = std::make_pair(camera.position.z, camera.position.x);
                                camera.position = (Vector3){current_teleport_position.second, 1.0f, current_teleport_position.first};
                                current_layer++;
                                current_teleport_position = positions[current_layer];
                            }
                        } else if(held_object == HO_NOTHING) {
                            float y = positions[current_layer].first;
                            float x = positions[current_layer].second;

                            bool collide = GetItemCollision((Vector3){x, 1.f, y}, 1.f, camera);

                            if(collide && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                                held_object = HO_TELEPORT;
                                positions[current_layer] = no_position;
                            }

                            DrawModel(teleport, (Vector3){x, sine_between(.6f, 0.9f, GetTime()*4), y}, .15f, WHITE);
                            teleport.transform = MatrixMultiply(teleport.transform, MatrixRotateY(-1.5f * GetFrameTime()));
                        }
                    } EndMode3D();
                    DrawItem();
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
