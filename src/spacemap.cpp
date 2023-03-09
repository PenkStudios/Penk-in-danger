#ifndef PENK_SMAP_CPP
#ifdef PENK_DEF_CPP
#define PENK_SMAP_CPP

#include "raylib.h"
#include "penkGraphics.cpp"

#include "TinyPngOut.cpp"
#include <sys/types.h>

#include <fstream>
#include <iostream>

#include "penk.cpp"


std::vector<Model> cubicmap_models;
std::vector<Mesh> cubicmap_meshes;
std::vector<Texture2D> cubicmap_textures;
Image cubicmap_image;
std::vector<Texture2D> atlas_textures;
std::vector<Color*> map_pixels_vector;

int current_layer;
int max_layers;

struct Furniture {
    PenkVector2 position;
    int id;
    float rotation;
};

int GetPixel(PenkVector2 coords, int map_size) {
    return (coords.x * map_size + coords.y) * 3;
}

bool InBorder(PenkVector2 coords, int map_size) {
    return (coords.x < 1 || coords.y < 1) ||
        (coords.x > map_size - 2 || coords.y > map_size - 2);
}

PenkVector2 directions[] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}}; // B(ack) R(ight) U(p) L(eft)

PenkVector2 no_direction {-1000, -1000};

PenkVector2 NewDirection(PenkVector2 direction) {
    int index = rand() % 4;

    return (directions[index].x == direction.x &&
            directions[index].y == direction.y) ? directions[(index + 1) % 4] : directions[index];
}

int Clamp(int n, int lower, int upper) {
    return std::max(lower, std::min(n, upper));
}

int Randint(int min, int max) {
    return min + (std::rand() % (max - min + 1));
}

PenkVector2 ReverseDirection(PenkVector2 direction) {
    PenkVector2 output;
    if(direction.x != 0) {
        output.x = direction.x * -1;
    } else {
        output.x = 0;
    }
    if(direction.y != 0) {
        output.y = direction.y * -1;
    } else {
        output.y = 0;
    }
    return output;
}

PenkVector2 ShiftDirection(PenkVector2 direction) {
    int index = 0;
    for(int i = 0; i < 5; i++) {
        if(directions[i].x == direction.x && directions[i].y == direction.y) {
            index = i;
            break;
        }
        if(i > 3) {
            return PenkVector2{0, 0};
        }
    }
    index++;
    if(index > 3) {
        index = 0;
    }
    
    return directions[index];
}

int NearCorner(PenkVector2 coords) {
    return (DIFF(coords.x, map_size) * DIFF(coords.y, map_size)) + 1;
}

PenkWorldVector2 current_teleport_position;

PenkVector2 ShiftCoords(PenkVector2 coords, int x, int y) {
    return PenkVector2{coords.x + x, coords.y + y};
}

struct SpaceMapType {
    std::vector<PenkWorldVector2> positions;
    std::vector<std::vector<Furniture>> furniture;
};

using FurnitureType = std::vector<std::vector<Furniture>>;
using PositionsType = std::vector<PenkWorldVector2>;

SpaceMapType CreateSpaceMap(int map_size, int up) {
    printf("[PENK.SMAP] Images (layers) to write: %i\n", up);
    printf("[PENK.SMAP] Map size: %ix%i\n", map_size, map_size);

    PositionsType out_positions;
    FurnitureType furniture_positions;

    int output_seed;

    uint8_t rgb[up][map_size * map_size * 3];
    
    printf("[PENK.SMAP] Allocated %i bytes for all layers (%i layer/s)\n", 
                (int)((map_size * map_size * 3 * up) * sizeof(unsigned char)), up);

    for(int y = 0; y < up; y++) {
            for(int p = 0; p < map_size * map_size * 3; p++) {
                rgb[y][p] = 0;
            }
    }

    PenkVector2 coords {map_size / 2, map_size / 2};

    printf("    Generation step 1 : \"Map creating base algorithm\" ");

    PenkVector2 direction = NewDirection(no_direction);

    for(int layer = 0; layer < up; layer++) {
        int start_point = GetPixel(coords, map_size);
        rgb[layer][start_point] = 50;
        rgb[layer][start_point + 1] = 50;
        rgb[layer][start_point + 2] = 50;

        int maximum = (rand() % (map_size * map_size / 2)) + (map_size * map_size / 2);
        int step = 5;
        int room_step = 15;

        furniture_positions.emplace_back();

        for(int i = 0; i < maximum; i++) {
            step--;
            room_step--;

            coords.x += direction.x;
            coords.y += direction.y;

            if(i >= maximum - 1) {
                out_positions.push_back(PenkWorldVector2{(float)coords.x, (float)coords.y});
                rgb[layer][GetPixel(coords, map_size)] = 50;
                rgb[layer][GetPixel(coords, map_size) + 1] = 50;
                rgb[layer][GetPixel(coords, map_size) + 2] = 50;
                continue;
            }

            if(InBorder(coords, map_size)) {
                coords.x -= direction.x;
                coords.y -= direction.y;
                direction = ReverseDirection(direction);
                continue;
            } else {
                if(!(rand() % NearCorner(coords))) {
                    direction = ShiftDirection(direction);
                }

                if(room_step <= 0) {
                    int room_size = 2 + rand() % 4;
                    for(int x = coords.x - room_size / 2; x < coords.x + room_size / 2; x++) {
                        for(int y = coords.y - room_size / 2; y < coords.y + room_size / 2; y++) {
                            PenkVector2 pixel_coords {Clamp(x, 1, map_size - 2), Clamp(y, 1, map_size - 2)};
                            rgb[layer][GetPixel(pixel_coords, map_size)] = 50;
                            rgb[layer][GetPixel(pixel_coords, map_size) + 1] = 50;
                            rgb[layer][GetPixel(pixel_coords, map_size) + 2] = 50;
                            if(rand() % 25 < 2) {
                                bool found = false;
                                for(auto position : furniture_positions[layer]) {
                                    if(position.position.x == pixel_coords.x && position.position.y == pixel_coords.y) {
                                        found = true;
                                        break;
                                    }
                                }

                                if(found) continue;

                                Furniture furniture;

                                furniture.position = pixel_coords;
                                furniture.id = rand() % max_furniture;
                                furniture.rotation = rand() % 360;

                                furniture_positions[layer].push_back(furniture);
                            }
                        }
                    }
                    room_step = 20 + rand() % 25;
                } else {
                    rgb[layer][GetPixel(coords, map_size)] = 50;
                    rgb[layer][GetPixel(coords, map_size) + 1] = 50;
                    rgb[layer][GetPixel(coords, map_size) + 2] = 50;
                }

                if(step <= 0) {
                    direction = NewDirection(direction);
                    step = 20 + rand() % 5;
                }
            }
        }
    }

    printf("    Generation step 2 : \"Pathing\"\n");

    for(int layer = 0; layer < up; layer++) {
        for(int x = 0; x < map_size; x++) {
            for(int y = 0; y < map_size; y++) {
                PenkVector2 coords {x, y};
                int current_pixel = GetPixel(coords, map_size);
                if(rgb[layer][current_pixel] != 50) continue;
                rgb[layer][current_pixel] = 10;
                rgb[layer][current_pixel + 1] = 10;
                rgb[layer][current_pixel + 2] = 10;
                
                for(PenkVector2 direction : directions) {
                    int pixel = GetPixel(ShiftCoords(coords, direction.x, direction.y), map_size);
                    if(rgb[layer][pixel] == 0) {
                        rgb[layer][pixel] = 255;
                        rgb[layer][pixel + 1] = 255;
                        rgb[layer][pixel + 2] = 255;
                    }
                }
            }
        }
    }

    printf("    Generation step 3 : \"Cleaning\"\n");

    for(int layer = 0; layer < up; layer++) {
        for(int x = 0; x < map_size; x++) {
            for(int y = 0; y < map_size; y++) {
                PenkVector2 coords {x, y};
                int current_pixel = GetPixel(coords, map_size);
                if((coords.x <= 0 || coords.x >= map_size - 1) || (coords.y <= 0 || coords.y >= map_size - 1)) {
                    rgb[layer][current_pixel] = 255;
                    rgb[layer][current_pixel + 1] = 255;
                    rgb[layer][current_pixel + 2] = 255;
                } else {
                    if(rgb[layer][current_pixel] == 10) {
                        rgb[layer][current_pixel] = 0;
                        rgb[layer][current_pixel + 1] = 0;
                        rgb[layer][current_pixel + 2] = 0;
                    }
                }
            }
        }
    }

    printf("[PENK.SMAP] Map generation succeeded\n");

    std::string filename;

    for(int i = 0; i < up; i++) {
        filename = "resources/temp/layer" + std::to_string(i) + ".png";
        printf("[PENK.SMAP] Writing image file '%s'...\n", filename.c_str());

        std::ofstream file(filename.c_str(), std::ios::binary);
        TinyPngOut pngout(static_cast<std::uint32_t>(map_size), static_cast<std::uint32_t>(map_size), file);
		pngout.write(rgb[i], static_cast<size_t>(map_size * map_size));
        file.close();

        printf("[PENK.SMAP] Loading map number %i %s\n", i, i == 0 ? " (current)" : "");

        printf("    Loading image...\n");
        cubicmap_image = LoadImage(TextFormat("resources/temp/layer%i.png", i));

        printf("    Loading texture...\n");
        cubicmap_textures.push_back(LoadTextureFromImage(cubicmap_image));
        
        cubicmap_meshes.push_back(GenMeshCubicmap(cubicmap_image, (Vector3){1.0f, 1.6f, 1.0f}));

        printf("    Loading model...\n");
        cubicmap_models.push_back(LoadModelFromMesh(cubicmap_meshes[i]));

        printf("    Loading atlas texture...\n");
        atlas_textures.push_back(LoadTexture("resources/atlas.png"));
        cubicmap_models[i].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = atlas_textures[i];

        printf("    Loading colors...\n");
        map_pixels_vector.push_back(LoadImageColors(cubicmap_image));
        UnloadImage(cubicmap_image);
    }

    max_layers = up;

    if(seed != 0) {
        srand(time(NULL));
    }

    SpaceMapType output;
    output.positions = out_positions;
    output.furniture = furniture_positions;

    return output;
}

void UpdateSpaceMap() {
    DrawModel(cubicmap_models[current_layer], (Vector3){0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
}

void SpaceFree() {
    for(int i = 0; i < max_layers; i++) {
        if(remove(TextFormat("resources/temp/layer%i.png", i)) != 0) {
            PenkError("SpaceFree()", "failed to remove");
        }
        UnloadImageColors(map_pixels_vector[i]);
        UnloadTexture(cubicmap_textures[i]);
        UnloadTexture(atlas_textures[i]);
        UnloadModel(cubicmap_models[i]);
    }
}

std::vector<Model> LoadFurniture() {
    std::vector<Model> output;
    for(int i = 0; i < max_furniture; i++) {
        output.push_back(LoadModel(TextFormat("resources/furniture%i.obj", i)));
    }
    return output;
}

void DrawFurniture(std::vector<Model> furnitures, FurnitureType furniture_positions) {
    for(auto position : furniture_positions[current_layer]) {
        if(position.id == -10000) continue;
        DrawModelEx(furnitures[position.id], (Vector3){(float)position.position.y, .15f, (float)position.position.x}, (Vector3){0, 1, 0}, position.rotation, (Vector3){1.f, 1.f, 1.f}, WHITE);
    }
}

void FurnitureFree(std::vector<Model> furnitures) {
    for(Model furniture : furnitures) {
        UnloadModel(furniture);
    }
}

void SpaceClear() {
    cubicmap_meshes.clear();
    cubicmap_textures.clear();
    atlas_textures.clear();
    map_pixels_vector.clear();
}

#endif // PENK_DEF_CPP

#endif // PENK_SMAP_CPP
