#ifndef PENK_DEF_HPP
#define PENK_DEF_HPP

int height = 600;
int width = 1000;

const char* window_title = "Penk in danger";

int fps = 60;

int blocks_x = 56;
int blocks_y = 56;
int layers = 2;
int seed = 0; /* random */

std::vector<int> seeds;
std::vector<std::pair<float, float>> positions;

#endif