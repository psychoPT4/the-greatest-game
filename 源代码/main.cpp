#include <iostream>
#include <string>
#include <windows.h>
#include <chrono>
#include "../头文件/map.h"
#include "../头文件/Role.h"

using namespace std;

string combatLog = "系统初始化...";
#define KEY_PRESSED(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 1 : 0)

void resetCursor() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = {0, 0};
    SetConsoleCursorPosition(hOut, pos);
}

// 渲染函数增加 currentLevel 参数，并在地图上绘制终点 'O'
void renderGame(const Map& gameMap, const Player& player, const Enemy& enemy, int currentLevel, int goalX, int goalY) {
    resetCursor();
    cout << "=== 《Fulfilling Knight》 关卡模式 | 当前层数: " << currentLevel << " ===      \n";
    cout << "操作: [A][D]横移 [K]跳跃 [J]攻击 [Q]退出                     \n";
    cout << "状态: HP [" << player.getHp() << "/" << player.getMaxHp() << "]                                     \n";
    cout << "日志: " << combatLog << "                                           \n";
    cout << "-------------------------------------------------------------\n";

    for (int y = 0; y < 20;++y) {
        for (int x = 0; x < 50; ++x) {
            if (player.getX() == x && player.getY() == y) cout << (player.isFlickering() ? "* " : "@ ");
            else if (enemy.isAlive() && enemy.getX() == x && enemy.getY() == y) cout << (enemy.isFlickering() ? "X " : "E ");
            else if (x == goalX && y == goalY) cout << "O "; // 绘制传送门终点
            else {
                TileType tile = gameMap.getTileAt(x, y);
                if (tile == TileType::Wall) cout << "[]";
                else if (tile == TileType::Platform) cout << "==";
                else cout << "  ";
            }
        }
        if (y < 19) cout << '\n';
    }
    cout << flush;
}

int main() {
    system("chcp 65001");
    system("cls");
    system("mode con cols=120 lines=30"); // 强制控制台至少 120 宽，30 高
    int currentLevel = 1;
    bool gameBeaten = false;

    // ==========================================
    // 外层循环：关卡状态机 (Level Manager)
    // ==========================================
    while (!gameBeaten) {
        // 动态加载地图文件，如 map1.csv, map2.csv
        string mapFile = "map" + to_string(currentLevel) + ".csv";
        Map gameMap(50, 20);
        
        if (!gameMap.loadFromCSV(mapFile)) {
            // 如果加载 map2.csv 失败，说明没有下一关了，游戏通关！
            system("cls");
            cout << "\n\n\t\tTHE END\n\t\t你完成了所有的试炼，骑士！\n\n";
            break; 
        }

        // 每一关重置主角位置 (可以根据关卡号设置不同的出生点)
        Player player(15, 12); 
        Enemy enemy(25, 12, 50);

        // 设置本关的终点坐标
        int goalX = 28, goalY = 5; 
        if (currentLevel == 2) { goalX = 2; goalY = 12; } // 假设第二关终点在左下角

        bool levelComplete = false;
        auto lastTime = chrono::high_resolution_clock::now();
        bool lastJumpState = false; 

        combatLog = "进入第 " + to_string(currentLevel) + " 层...";

        // ==========================================
        // 内层循环：实时物理引擎与战斗 (Game Loop)
        // ==========================================
        while (player.isAlive() && !levelComplete) {
            if (KEY_PRESSED('Q')) { gameBeaten = true; break; }

            // 时间差计算
            auto currentTime = chrono::high_resolution_clock::now();
            float dt = chrono::duration<float>(currentTime - lastTime).count();
            lastTime = currentTime;
            if (dt > 0.05f) dt = 0.05f;

            // 输入处理
            int moveDir = 0;
            if (KEY_PRESSED('A')) moveDir -= 1;
            if (KEY_PRESSED('D')) moveDir += 1;
            player.setMoveIntent(moveDir);

            bool jumpHeld = KEY_PRESSED('K');
            bool jumpPressed = jumpHeld && !lastJumpState; 
            lastJumpState = jumpHeld;
            player.processJump(jumpPressed, jumpHeld);

            static int attackCd = 0;
            if (attackCd > 0) attackCd--;
            if (KEY_PRESSED('J') && attackCd <= 0) {
                player.attack(enemy, gameMap);
                attackCd = 10;
            }

            // 物理更新
            player.update(gameMap, dt);
            enemy.update(gameMap, player, dt);

            // ------------------------------------------
            // 场景交互逻辑
            // ------------------------------------------
            
            // 1. 通关检测：碰到传送门 'O'
            if (player.getX() == goalX && player.getY() == goalY) {
                levelComplete = true;
                combatLog = "抵达出口！准备传送...";
            }

            // 2. 环境陷阱检测：如果掉入 y=14 的深渊（底层）
            if (player.getY() >= 19) {
                // 环境伤害：假装这伤害是来自于 x=15 的位置，产生随机方向击退
                player.takeDamage(20, 15, gameMap); 
                combatLog = "踩到地刺/岩浆！HP -20";
            }

            renderGame(gameMap, player, enemy, currentLevel, goalX, goalY);
            Sleep(15); 
        }

        // --- 单局结算 ---
        if (!player.isAlive()) {
            system("cls");
            cout << "\n\n\t\tYOU DIED\n\t\t意志在黑暗中消散了...\n\n";
            break; // 死亡直接结束外层循环
        }

        if (levelComplete) {
            // 短暂停顿，让玩家看清过关提示
            Sleep(800); 
            system("cls");
            currentLevel++; // 关卡号+1，外层循环将尝试加载下一张地图
        }
    }
    return 0;
}