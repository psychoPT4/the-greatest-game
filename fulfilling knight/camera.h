#ifndef camera_h
#define camera_h

#include <algorithm>
#include "raylib.h"

struct GameCamera {
    float x, y;          // 摄像机左上角的世界坐标
    int viewW, viewH;    // 屏幕能看到的格子数量

    GameCamera(int w, int h) : x(0), y(0), viewW(w), viewH(h) {}

    // 让摄像机平滑跟踪目标，并限制在地图边界内
    void update(float targetX, float targetY, int mapW, int mapH) {
        // 让目标处于窗口中心
        x = targetX - viewW / 2.0f;
        y = targetY - viewH / 2.0f;

        // 边界钳制：不允许看到地图外面（防止下标越界导致崩溃）
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x > (float)mapW - viewW) x = (float)mapW - viewW;
        if (y > (float)mapH - viewH) y = (float)mapH - viewH;
    }
    Camera2D getCamera() const {
        Camera2D cam = { 0 };
        // 你的 x 和 y 是“格子的世界坐标”。
        // Raylib 的摄像机需要“真实的像素坐标”，所以必须乘以格子的像素大小！
        // 【注意】：假设你的 TILE_SIZE 是 40.0f。如果你定义了全局常量 TILE_SIZE，请把 40.0f 换成它！
        cam.target = { x * 40.0f, y * 40.0f };

        cam.offset = { 0.0f, 0.0f }; // 屏幕锚点在左上角
        cam.rotation = 0.0f;
        cam.zoom = 1.0f;

        return cam;
    }
};

#endif
