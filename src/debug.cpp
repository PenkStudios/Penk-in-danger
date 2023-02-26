#ifndef PENK_DEBUG_CPP

// If space map included
#ifdef PENK_SMAP_CPP

#define PENK_DEBUG_H

#include "raylib.h"
#include "penk.cpp"
#include "penkGraphics.cpp"

class Key {
    public:
        int id;
        bool held = false;
        Key(int key_id) {
            id = key_id;
        }

        bool check() {
            return IsKeyPressed(id);
        }

        bool getSwitch() {
            if(IsKeyPressed(id)) {
                held = !held;
            }
            return held;
        }
};

Key tab(KEY_TAB);

void DrawDebugText(const char* text, int text_index) {
    DrawTextEx(base_font, text, (Vector2){5, (float)text_index * 20 + 5}, 25, 2, DARKGRAY);
}

void DebugHandleKeys() {
    debug_menu = tab.getSwitch();
    if(debug_menu) {
        DrawDebugText("Penk in danger: debug", 0);
        if(IsKeyPressed(KEY_UP)) {
            current_layer++;
            if(current_layer > max_layers - 1) current_layer = 0;
        } else if(IsKeyPressed(KEY_DOWN)) {
            current_layer--;
            if(current_layer < 0) current_layer = max_layers - 1;
        }
        if(IsKeyPressed(KEY_R)) {
            SpaceClear();
            CreateSpaceMap(blocks_x, blocks_y, layers, seeds[current_layer], MAPTYPE_NORMAL, current_layer);
        } else if(IsKeyPressed(KEY_T)) {
            SpaceClear();
            CreateSpaceMap(blocks_x, blocks_y, layers, seeds[current_layer], MAPTYPE_WIREFRAME, current_layer);
        }
        DrawDebugText(TextFormat("Current layer: %i", current_layer), 1);
    }
}
#endif // PENK_SMAP_CPP

#endif // PENK_DEBUG_CPP