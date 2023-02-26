#ifndef PENK_CPP
#define PENK_CPP

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <bits/stdc++.h>

const std::pair<float, float> no_position = std::make_pair(FLT_MAX, FLT_MAX);

void PenkError(char* proc, const char* error) {
    if(proc[0] > 64 && proc[0] < 91) {
        proc[0] -= 32;
    }
    printf("[ERROR] %s failed : %s", proc, error);
    exit(1);
}

void PenkAssert(bool boolean, const char* error) {
    if(!boolean) PenkError((char*)"asserting", error);
}

int map(int x, int min, int max, int min2, int max2) {
    return min2 + (max2 - min2) * (x - min) / (max - min);
}

float sine_between(float min, float max, float t) {
    return ((max - min) * sinf(t) + max + min) / 2;
}

#define DIFF(x, y) (x > y ? x - y : y - x)

#endif
