#ifndef PENK_G_HPP
#define PENK_G_HPP

#include <raylib.h>
#include <raymath.h>
#include <vector>
#include <stdio.h>
#include <cstring>

Font base_font;

enum ButtonState {BUTTON_NORMAL, BUTTON_DISABLED, BUTTON_PRESSED};

struct ButtonView {
    Texture2D normal;
    Texture2D disabled;
    Texture2D clicked;
};

struct Button {
    ButtonView view;
    ButtonState state = BUTTON_DISABLED;
    bool pressed = false;
};

Vector2 mousePoint = {0.0f, 0.0f};
Rectangle collisionPoints;
Texture2D currentButton;

void DrawTextureScale(Texture2D texture, Vector2 position, Vector2 scale, Color tint) {
    DrawTexturePro(texture,
        (Rectangle){0.0f, 0.0f, (float)texture.width, (float)texture.height},
        (Rectangle){position.x, position.y, scale.x * texture.width, scale.y * texture.height},
        (Vector2){(float)(texture.width * scale.x) / 2, (float)(texture.height * scale.y) / 2},
        0,
        tint
    );
}

void DrawButton(Button *button, Vector2 position, Vector2 scale, const char* text, Color text_color, int font_scale = 4, bool update = true) {
    button->pressed = false;

    switch(button->state) {
        case BUTTON_NORMAL: {
            currentButton = button->view.normal;
            break;
        }
        case BUTTON_DISABLED: {
            currentButton = button->view.disabled;
            break;
        }
        case BUTTON_PRESSED: {
            currentButton = button->view.clicked;
            break;
        }
    }

    DrawTextureScale(currentButton, position, scale, WHITE);
    Vector2 text_position = {
        position.x - (currentButton.width * scale.x / 2) + strlen(text),
        position.y - (currentButton.height * scale.y / 2) + strlen(text)
    };
    float plus_x = button->state == BUTTON_PRESSED ? 30 : 0;
    float plus_y = button->state == BUTTON_PRESSED ? 25 : 0;
    DrawTextEx(base_font, text, (Vector2){text_position.x + plus_x, text_position.y + plus_y}, base_font.baseSize*((float)font_scale), 2, text_color);

    if(update) {
        mousePoint = GetMousePosition();
        collisionPoints = {position.x - (currentButton.width * scale.x / 2), position.y - (currentButton.height * scale.y / 2), (float)(currentButton.width * scale.x), (float)(currentButton.height * scale.y)};
        if(CheckCollisionPointRec(mousePoint, collisionPoints)) {
            button->state = BUTTON_NORMAL;
            if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                button->state = BUTTON_PRESSED;
            } else if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                button->state = BUTTON_NORMAL;
                button->pressed = true;
            }
        } else {
            button->state = BUTTON_DISABLED;
        }
    }
}

bool IsButtonPressed(Button button) {
    return button.pressed;
}

bool GetItemCollision(Vector3 item_position, float item_radius, Camera camera) {
    return ((camera.position.x < item_position.x + item_radius && camera.position.x > item_position.x - item_radius) &&
        (camera.position.z < item_position.z + item_radius && camera.position.z > item_position.z - item_radius));
}

void UnloadButton(Button button) {
    UnloadTexture(button.view.normal);
    UnloadTexture(button.view.disabled);
    UnloadTexture(button.view.clicked);
}

void CenterWindow(float window_width, float window_height) {
    int monitor = GetCurrentMonitor();
    int monitor_width = GetMonitorWidth(monitor);
    int monitor_height = GetMonitorHeight(monitor);
    SetWindowPosition((int)(monitor_width / 2) - (int)(window_width / 2), (int)(monitor_height / 2) - (int)(window_height / 2));
}

#endif