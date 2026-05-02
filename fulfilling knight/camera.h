#ifndef camera_h
#define camera_h

#include <algorithm>

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
};

#endif
