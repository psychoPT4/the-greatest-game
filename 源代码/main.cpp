#include <iostream>
#include <string>
#include <windows.h>
#include <chrono> // 引入高精度时钟
#include "../头文件/map.h"
#include "../头文件/Role.h"

using namespace std;

string combatLog = "系统初始化...";

// 宏：检查按键是否被按下
#define KEY_PRESSED(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 1 : 0)

void resetCursor() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = {0, 0};
    SetConsoleCursorPosition(hOut, pos);
}

void renderGame(const Map& gameMap, const Player& player, const Enemy& enemy) {
    resetCursor();
    cout << "=== 《Fulfilling Knight》 纯正动力学引擎 ===                 \n";
    cout << "操作: [A][D]长按横移 [K]长按大跳/轻点小跳 [J]攻击 [Q]退出    \n";
    cout << "状态: HP [" << player.getHp() << "/" << player.getMaxHp() << "]                                     \n";
    cout << "日志: " << combatLog << "                                           \n";
    cout << "-------------------------------------------------------------\n";

    for (int y = 0; y < 15; ++y) {
        for (int x = 0; x < 30; ++x) {
            if (player.getX() == x && player.getY() == y) cout << (player.isFlickering() ? "* " : "@ ");
            else if (enemy.isAlive() && enemy.getX() == x && enemy.getY() == y) cout << (enemy.isFlickering() ? "X " : "E ");
            else {
                TileType tile = gameMap.getTileAt(x, y);
                if (tile == TileType::Wall) cout << "[]";
                else if (tile == TileType::Platform) cout << "==";
                else cout << "  ";
            }
        }
        if (y < 14) cout << '\n';
    }
    cout << flush;
}

int main() {
    system("chcp 65001");
    system("cls");
    
    Map gameMap(30, 15);
    if (!gameMap.loadFromCSV("map1.csv")) return -1;

    Player player(15, 12); 
    Enemy enemy(25, 12, 50);

    // 记录上一帧的时间戳
    auto lastTime = chrono::high_resolution_clock::now();
    bool lastJumpState = false; 

    while (true) {
        if (KEY_PRESSED('Q')) break;

        // 计算物理 Delta Time
        auto currentTime = chrono::high_resolution_clock::now();
        float dt = chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        // 钳制 dt 防止拖动窗口导致的物理爆炸 (最高限制为 0.05 秒)
        if (dt > 0.05f) dt = 0.05f;

        // --- 获取输入状态 ---
        int moveDir = 0;
        if (KEY_PRESSED('A')) moveDir -= 1;
        if (KEY_PRESSED('D')) moveDir += 1;
        player.setMoveIntent(moveDir);

        bool jumpHeld = KEY_PRESSED('K');
        bool jumpPressed = jumpHeld && !lastJumpState; // 提取跳跃按下的瞬间
        lastJumpState = jumpHeld;
        
        player.processJump(jumpPressed, jumpHeld);

        // 为了防止控制台狂刷日志，攻击键加个简单的冷却阻断
        static int attackCd = 0;
        if (attackCd > 0) attackCd--;
        if (KEY_PRESSED('J') && attackCd <= 0) {
            player.attack(enemy, gameMap);
            attackCd = 10;
        }

        // --- 核心物理与逻辑更新 ---
        player.update(gameMap, dt);
        enemy.update(gameMap, player, dt);
        
        renderGame(gameMap, player, enemy);

        Sleep(15); // 让出 CPU 资源，保持极限 60+ FPS 的采样率
    }
    return 0;
}