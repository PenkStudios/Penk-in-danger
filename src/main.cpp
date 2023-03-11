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

PositionsType positions;
FurnitureType furniture_positions;

struct stat info;

bool EnvironmentCheck() {
    std::vector<const char*> deps = {
        "resources/atlas.png",
        "resources/temp/",
        "resources/teleport.obj",
        "resources/teleport.mtl",
        "resources/redboy.obj",
        "resources/redboy.png",
        "resources/teleport_hand.png",
        "resources/font.png",
        "resources/house.obj"
    };

    for(int furniture = 0; furniture < max_furniture; furniture++) {
        deps.push_back(TextFormat("resources/furniture%i.obj", furniture));
        deps.push_back(TextFormat("resources/furniture%i.mtl", furniture));
    }

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

enum Scene {MENU, GAME, SERVER_CONSOLE};
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
    positions[current_layer] = PenkWorldVector2 {(float)((int)camera->position.z), (float)((int)camera->position.x)};
    pathMap = Path::PathMap{};
    current_layer++;
    enemyPosition.position = PenkWorldVector2 {(float)positions[current_layer].y, (float)positions[current_layer].x};
}

Vector3 CalculateLookAtTarget(Camera camera) {
    return Vector3Add(Vector3Divide(Vector3Subtract(camera.target, camera.position), (Vector3){8.f, 8.f, 8.f}), camera.position);
}

enum HeldObject {HO_NOTHING, HO_TELEPORT};
HeldObject held_object = HO_NOTHING;

Model teleport;

Controller controller;

void DrawItem(Camera3D camera) {
    Model to_render;
    switch(held_object) {
        case HO_TELEPORT:
            to_render = teleport;
            break;
        case HO_NOTHING: return;
    }

    Vector3 position = CalculateLookAtTarget(camera);

    float rotation = YRotateTowards(PenkWorldVector2{camera.target.x, camera.target.z}, PenkWorldVector2{camera.position.x, camera.position.z}) * RAD2DEG;

    teleport.transform = MatrixIdentity();

    DrawModelEx(to_render, position, (Vector3){0.f, 1.f, 0.f}, rotation, (Vector3){.1f, .1f, .1f}, WHITE);
}

struct ConsoleData {
    int letterPointer;
    int letterMax;
    int maxRows;
    int rowPointer;
    std::string inputText;
    std::vector<std::string> consoleText;
};

ConsoleData GetConsoleData() {
    ConsoleData data;
    data.letterMax = GetScreenWidth() / 19;
    data.maxRows = GetScreenHeight() / 30;

    for(int character = 0; character < data.letterMax; character++) {
        data.inputText += '\0';
    }

    for(int row = 0; row < data.maxRows; row++) {
        data.consoleText.emplace_back();
        for(int character = 0; character < data.letterMax; character++) {
            data.consoleText[row] += '\0';
        }
    }

    return data;
}

ConsoleData consoleData;

int main(int argc, char** argv) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);

    GameInit();
    
    printf("[PENK] Preparing to call 'createSpaceMap'...\n");
    SpaceMapType space_data = CreateSpaceMap(map_size, layers);
    positions = space_data.positions;
    furniture_positions = space_data.furniture;

    Camera camera = { { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f, 0 };
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
                    DrawTextureScale(title, (Vector2){(float)GetScreenWidth() / 2, (float)GetScreenHeight() / 2 - 125.f}, (Vector2){6.7f, 6.2f}, WHITE);
                    DrawButton(&new_game_button, (Vector2){(float)GetScreenWidth() / 2 - 155, (float)GetScreenHeight() / 2 + 50.f}, (Vector2){.6f, .6f}, "New game", WHITE, 3.f);
                    DrawButton(&new_server_button, (Vector2){(float)GetScreenWidth() / 2 + 155, (float)GetScreenHeight() / 2 + 50.f}, (Vector2){.6f, .6f}, "New server", WHITE, 3.f);
                    DrawButton(&join_server_button, (Vector2){(float)GetScreenWidth() / 2 - 155, (float)GetScreenHeight() / 2 + 150.f}, (Vector2){.6f, .6f}, "Join server", WHITE, 3.f);
                    DrawButton(&settings_button, (Vector2){(float)GetScreenWidth() / 2 + 155, (float)GetScreenHeight() / 2 + 150.f}, (Vector2){.6f, .6f}, "Settings", WHITE, 3.f);
                    if(IsButtonPressed(new_game_button)) {
                        SwitchToGame(&camera, &controller);
                    } else if(IsButtonPressed(new_server_button)) {
                        consoleData = GetConsoleData();
                        scene = SERVER_CONSOLE;
                    }
                } EndDrawing();
                break;
            }

            case SERVER_CONSOLE: {
                BeginDrawing(); {
                    ClearBackground(Color{25, 25, 30, 255});
                    DrawRectangle(10, 10, GetScreenWidth() - 20, GetScreenHeight() - 20, Color{10, 10, 15, 255});
                    DrawRectangle(13, 13, GetScreenWidth() - 26, GetScreenHeight() - 26, Color{30, 30, 35, 255});
                    DrawRectangle(17, GetScreenHeight() - 52, GetScreenWidth() - 34, 35, Color{20, 20, 25, 255});
                    if(IsKeyPressed(KEY_BACKSPACE)) {
                        consoleData.letterPointer--;
                        if (consoleData.letterPointer < 0) consoleData.letterPointer = 0;
                        consoleData.inputText[consoleData.letterPointer] = '\0';
                    } else if(IsKeyPressed(KEY_ENTER)) {
                        if(consoleData.rowPointer >= consoleData.maxRows) {
                            consoleData.rowPointer = 0;
                            for(int row = 0; row < consoleData.maxRows - 1; row++) {
                                for(int index = 0; index < consoleData.letterMax; index++) {
                                    consoleData.consoleText[row][index] = '\0';
                                }
                            }
                        }
                        consoleData.consoleText[consoleData.rowPointer] = consoleData.inputText;
                        consoleData.rowPointer++;
                        for(int index = 0; index < consoleData.letterMax; index++) {
                            consoleData.inputText[index] = '\0';
                        }
                        consoleData.letterPointer = 0;
                    }
                    int character = GetCharPressed();
                    if(character != 0 && consoleData.letterPointer < consoleData.letterMax) {
                        consoleData.inputText[consoleData.letterPointer] = (char)character;
                        consoleData.inputText[consoleData.letterPointer + 1] = '\0';
                        consoleData.letterPointer++;
                    }
                    DrawTextEx(base_font, consoleData.inputText.c_str(), (Vector2){25, (float)GetScreenHeight() - 50}, 30, 5, WHITE);
                    for(int row = 0; row < consoleData.rowPointer; row++) {
                        DrawTextEx(base_font, consoleData.consoleText[row].c_str(), (Vector2){20, (float)15 + row * 25}, 30, 5, WHITE);
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
                        if ((map_pixels_vector[current_layer][y*cubicmap_textures[current_layer].width + x].r == 255) &&
                            (CheckCollisionCircleRec(playerPos, playerRadius,
                            (Rectangle){ 0 - 0.5f + x*1.0f, 0 - 0.5f + y*1.0f, 1.0f, 1.0f }))) {
                            camera.position = old_camera_position;
                        }
                    }
                }

                BeginDrawing(); {
                    ClearBackground(Color{50, 100, 150, 0});
                    BeginMode3D(camera); {
                        enemyPosition.Tick(map_pixels_vector[current_layer], map_size, &pathMap, PenkWorldVector2 {camera.position.x, camera.position.z}, 10);
                        DrawModelEx(redboy, (Vector3){enemyPosition.position.x, sine_between(.8f, 1.f, GetTime() * 10), enemyPosition.position.y}, (Vector3){0.f, 1.f, 0.f}, (float)enemyPosition.rotation, (Vector3){0.4f, 0.4f, 0.4f}, WHITE);
                        UpdateSpaceMap();
                        DrawFurniture(furniture, furniture_positions);
                        if(held_object == HO_TELEPORT) {
                            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                                bool collide = false;
                                Vector3 lookAt = CalculateLookAtTarget(camera);
                                for (int y = 0; y < cubicmap_textures[current_layer].height; y++) {
                                    for (int x = 0; x < cubicmap_textures[current_layer].width; x++) {
                                        if ((map_pixels_vector[current_layer][y*cubicmap_textures[current_layer].width + x].r == 255) &&
                                            (CheckCollisionCircleRec(Vector2 {lookAt.x, lookAt.z}, 0.2f,
                                            (Rectangle){ 0 - 0.5f + x*1.0f, 0 - 0.5f + y*1.0f, 1.0f, 1.0f }))) {
                                            // TODO: Action
                                            collide = true;
                                        }
                                    }
                                }

                                if(!collide) {
                                    held_object = HO_NOTHING;
                                    positions[current_layer] = PenkWorldVector2 {lookAt.z, lookAt.x};
                                }
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
                                    if(pathMap.map[y][x] > 0) {
                                        DrawRectangle(GetScreenWidth() - cubicmap_textures[current_layer].width*4 - 20 + x * 4, 20 + y * 4, 4, 4, Color{(unsigned char)(pathMap.map[y][x] / 50), (unsigned char)(pathMap.map[y][x] / 50), 0, 255});
                                    } else {
                                        DrawRectangle(GetScreenWidth() - cubicmap_textures[current_layer].width*4 - 20 + x * 4, 20 + y * 4, 4, 4, Color{0, (unsigned char)(-(pathMap.map[y][x]) / 5000), 0, 255});
                                    }
                                }
                            }
                        }

                        Vector3 lookAtTarget = CalculateLookAtTarget(camera);

                        DrawRectangle(GetScreenWidth() - cubicmap_textures[current_layer].width*4 - 20 + lookAtTarget.x*4, 20 + lookAtTarget.z*4, 3, 3, Color {255, 140, 0, 255});

                        DrawRectangle(GetScreenWidth() - cubicmap_textures[current_layer].width*4 - 20 + enemyPosition.position.x*4, 20 + enemyPosition.position.y*4, 4, 4, RED);
                        DrawRectangle(GetScreenWidth() - cubicmap_textures[current_layer].width*4 - 20 + camera.position.x*4, 20 + camera.position.z*4, 4, 4, GREEN);
                        if((int)positions[current_layer].x != 2147483647 && (int)positions[current_layer].y != 2147483647) {
                            DrawRectangle(GetScreenWidth() - cubicmap_textures[current_layer].width*4 - 20 + positions[current_layer].y*4, 20 + positions[current_layer].x*4, 4, 4, BLUE);
                        }
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
