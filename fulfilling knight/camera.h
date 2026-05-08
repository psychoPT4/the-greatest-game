#ifndef camera_h
#define camera_h

#include <algorithm>
#include "raylib.h"

struct GameCamera {
    float x, y;          // ��������Ͻǵ���������
    int viewW, viewH;    // ��Ļ�ܿ����ĸ�������

    GameCamera(int w, int h) : x(0), y(0), viewW(w), viewH(h) {}

    // �������ƽ������Ŀ�꣬�������ڵ�ͼ�߽���
    void update(float targetX, float targetY, int mapW, int mapH) {
        // ��Ŀ�괦�ڴ�������
        x = targetX - viewW / 2.0f;
        y = targetY - viewH / 2.0f;

        // �߽�ǯ�ƣ�������������ͼ���棨��ֹ�±�Խ�絼�±�����
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x > (float)mapW - viewW) x = (float)mapW - viewW;
        if (y > (float)mapH - viewH) y = (float)mapH - viewH;
    }
    Camera2D getCamera() const {
        Camera2D cam = { 0 };
        // ��� x �� y �ǡ����ӵ��������ꡱ��
        // Raylib ���������Ҫ����ʵ���������ꡱ�����Ա�����Ը��ӵ����ش�С��
        // ��ע�⡿��������� TILE_SIZE �� 40.0f������㶨����ȫ�ֳ��� TILE_SIZE����� 40.0f ��������
        cam.target = { x * 48.0f, y * 48.0f };

        cam.offset = { 0.0f, 0.0f }; // ��Ļê�������Ͻ�
        cam.rotation = 0.0f;
        cam.zoom = 1.0f;

        return cam;
    }
};

#endif
