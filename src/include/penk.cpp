#ifndef PENK_HPP
#define PENK_HPP

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

#define DIFF(x, y) (x > y ? x - y : y - x)

#endif
