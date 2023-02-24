#ifndef PENK_SMAP_HPP
#define PENK_SMAP_HPP

#include "raylib.h"
#include "penkGraphics.cpp"

#include "TinyPngOut.cpp"
#include <sys/types.h>

#include <fstream>
#include <iostream>


std::vector<Model> cubicmap_models;
std::vector<Mesh> cubicmap_meshes;
std::vector<Texture2D> cubicmap_textures;
Image cubicmap_image;
std::vector<Texture2D> atlas_textures;
std::vector<Color*> map_pixels_vector;

int current_layer;
int max_layers;

int GetPixel(std::pair<int, int> coords, int width) {
    return (coords.first * width + coords.second) * 3;
}

bool InBorder(std::pair<int, int> coords, int width, int height) {
    return (coords.first < 1 || coords.second < 1) ||
        (coords.first > width - 2 || coords.second > height - 2) ? true : false;
}

std::pair<int, int> directions[] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}}; // B(ack) R(ight) U(p) L(eft)

std::pair<int, int> no_direction = std::make_pair(-1000, -1000);

std::pair<int, int> NewDirection(std::pair<int, int> direction) {
    std::pair<int, int> new_direction;
    while(direction == new_direction) {
        new_direction = directions[rand() % 4];
        if(direction == no_direction) {
            break;
        }
    }
    return new_direction;
}

int Clamp(int n, int lower, int upper) {
  return std::max(lower, std::min(n, upper));
}

int Randint(int min, int max) {
    return min + ( std::rand() % ( max - min + 1 ) );
}

std::pair<int, int> ReverseDirection(std::pair<int, int> direction) {
    std::pair<int, int> output;
    if(direction.first != 0) {
        output.first = direction.first * -1;
    } else {
        output.first = 0;
    }
    if(direction.second != 0) {
        output.second = direction.second * -1;
    } else {
        output.second = 0;
    }
    return output;
}

std::pair<int, int> ShiftDirection(std::pair<int, int> direction) {
    int index = 0;
    for(int i = 0; i < 5; i++) {
        if(directions[i] == direction) {
            index = i;
            break;
        }
        if(i > 3) {
            return std::make_pair(0, 0);
        }
    }
    index++;
    if(index > 3) {
        index = 0;
    }
    
    return directions[index];
}

#include "penk.cpp"
#include "defSettings.cpp"

int NearCorner(std::pair<int, int> coords) {
    return (DIFF(coords.first, blocks_x) * DIFF(coords.second, blocks_y)) + 1;
}

std::pair<float, float> current_teleport_position;

std::pair<int, int> ShiftCoords(std::pair<int, int> coords, int x, int y) {
    return std::make_pair(coords.first + x, coords.second + y);
}

enum MapType {MAPTYPE_NORMAL, MAPTYPE_WIREFRAME};

std::pair<std::vector<int>, std::vector<std::pair<float, float>>> CreateSpaceMap(int mapsize_x, int mapsize_y, int up, int seed, MapType map_type, int layer) {
    printf("[PENK.SMAP] Images (layers) to write: %i\n", up);
    printf("[PENK.SMAP] Map size: %i, %i\n", mapsize_x, mapsize_y);

    std::vector<int> seeds;
    std::vector<std::pair<float, float>> out_positions;

    int output_seed;

    uint8_t rgb[up][mapsize_x * mapsize_y * 3];
    
    printf("[PENK.SMAP] Allocated %i bytes for all layers (%i layer/s)\n", 
                (int)((mapsize_x * mapsize_y * 3 * up) * sizeof(unsigned char)), up);

    for(int y = 0; y < up; y++) {
            for(int p = 0; p < mapsize_x * mapsize_y * 3; p++) {
                rgb[y][p] = 0;
            }
    }

    std::pair<int, int> coords = std::make_pair(mapsize_x / 2, mapsize_y / 2);
    // randint(1, mapsize_x - 1), randint(1, mapsize_y - 1)
    // (must be in center)

    printf("    Generation step 1 : \"Map creating base algorithm\" ");

    std::pair<int, int> direction = NewDirection(no_direction);

    for(int layer = 0; layer < up; layer++) {
        if(seed == 0) {
            output_seed = time(NULL);
            printf("seed=%i (random)\n", output_seed);
            srand(output_seed);
        } else {
            printf("seed=%i\n", seed);
            output_seed = seed;
            srand((unsigned int)seed);
        }
        seeds.push_back(output_seed);

        int start_point = GetPixel(coords, mapsize_x);
        rgb[layer][start_point] = 50;
        rgb[layer][start_point + 1] = 50;
        rgb[layer][start_point + 2] = 50;

        printf("%i, %i\n", direction.first, direction.second);

        int maximum = (rand() % mapsize_x * mapsize_y + 1) * 2;
        int step = 5;

        for(int i = 0; i < maximum; i++) {
            step--;
            if(i >= maximum - 1) {
                out_positions.push_back(coords);
                rgb[layer][GetPixel(coords, mapsize_x)] = 50;
                rgb[layer][GetPixel(coords, mapsize_x) + 1] = 50;
                rgb[layer][GetPixel(coords, mapsize_x) + 2] = 50;
                continue;
            }
            coords.first += direction.first;
            coords.second += direction.second;

            if(InBorder(coords, mapsize_x, mapsize_y)) {
                coords.first -= direction.first;
                coords.second -= direction.second;
                direction = ReverseDirection(direction);
                continue;
            } else {
                if(!(rand() % NearCorner(coords))) {
                    direction = ShiftDirection(direction);
                }

                rgb[layer][GetPixel(coords, mapsize_x)] = 50;
                rgb[layer][GetPixel(coords, mapsize_x) + 1] = 50;
                rgb[layer][GetPixel(coords, mapsize_x) + 2] = 50;

                if(step <= 0) {
                    direction = NewDirection(direction);
                    step = 5 + rand() % 15;
                }
            }
        }
    }

    printf("    Generation step 2 : \"Pathing\"\n");

    for(int layer = 0; layer < up; layer++) {
        for(int x = 0; x < mapsize_x; x++) {
            for(int y = 0; y < mapsize_y; y++) {
                std::pair<int, int> coords = std::make_pair(x, y);
                int current_pixel = GetPixel(coords, mapsize_x);
                if(rgb[layer][current_pixel] != 50) continue;
                rgb[layer][current_pixel] = 10;
                rgb[layer][current_pixel + 1] = 10;
                rgb[layer][current_pixel + 2] = 10;
                
                for(std::pair<int, int> direction : directions) {
                    int pixel = GetPixel(ShiftCoords(coords, direction.first, direction.second), mapsize_x);
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
        for(int x = 0; x < mapsize_x; x++) {
            for(int y = 0; y < mapsize_y; y++) {
                std::pair<int, int> coords = std::make_pair(x, y);
                int current_pixel = GetPixel(coords, mapsize_x);
                if(rgb[layer][current_pixel] == 10) {
                    rgb[layer][current_pixel] = 0;
                    rgb[layer][current_pixel + 1] = 0;
                    rgb[layer][current_pixel + 2] = 0;
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
        TinyPngOut pngout(static_cast<std::uint32_t>(mapsize_x), static_cast<std::uint32_t>(mapsize_y), file);
		pngout.write(rgb[i], static_cast<size_t>(mapsize_x * mapsize_y));
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
        const char* path = NULL;
        switch(map_type)
        {
        case MAPTYPE_NORMAL:
            path = "resources/atlas.png";
            break;
        
        case MAPTYPE_WIREFRAME:
            path = "resources/wireframe.png";
            break;

        default:
            printf("[PENK.SMAP] Invalid map type!\n");
            break;
        }
        atlas_textures.push_back(LoadTexture(path));
        cubicmap_models[i].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = atlas_textures[i];

        printf("    Loading colors...\n");
        map_pixels_vector.push_back(LoadImageColors(cubicmap_image));
        UnloadImage(cubicmap_image);
    }

    current_layer = layer;
    max_layers = up;

    return std::make_pair(seeds, out_positions);
}

void UpdateSpaceMap() {
    DrawModel(cubicmap_models[current_layer], (Vector3){0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
}

void SpaceFree() {
    for(int i = 0; i < max_layers; i++) {
        UnloadImageColors(map_pixels_vector[i]);
        UnloadTexture(cubicmap_textures[i]);
        UnloadTexture(atlas_textures[i]);
        UnloadModel(cubicmap_models[i]);
    }
}

void SpaceClear() {
    cubicmap_meshes.clear();
    cubicmap_textures.clear();
    atlas_textures.clear();
    map_pixels_vector.clear();
}

#endif 
