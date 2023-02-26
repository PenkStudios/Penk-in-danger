#ifndef PENK_DEF_CPP
#define PENK_DEF_CPP

#include <vector>

int height = 600;
int width = 1000;

const char* window_title = "Penk in danger";

int fps = 60;

int blocks_x = 56;
int blocks_y = 56;
int layers = 10;
int seed = 0; /* random */

std::vector<int> seeds;
std::vector<std::pair<float, float>> positions;
std::vector<std::vector<std::pair<std::pair<int, int>, std::pair<int, int>>>> furniture_positions;
int max_furniture = 4;

#endif // PENK_DEF_CPP