#include <iostream>
#include <string>
#include <vector>
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

void renderGame(const Map& gameMap, const Player& player, const vector<Enemy>& enemies, int currentLevel, SpawnPoint goal) {
    resetCursor();
    cout << "=== 《Fulfilling Knight》 纯数据驱动版 ===                 \n";
    cout << "操作: [A][D]横移 [K]跳跃 [J]平砍 [S]+[J]空中下劈             \n";
    cout << "状态: HP [" << player.getHp() << "/" << player.getMaxHp() << "]                                     \n";
    cout << "日志: " << combatLog << "                                           \n";
    cout << "-------------------------------------------------------------\n";

    for (int y = 0; y < 20; ++y) {
        for (int x = 0; x < 50; ++x) {
            // 绘制骑士
            if (player.getX() == x && player.getY() == y) {
                cout << (player.isFlickering() ? "* " : "@ ");
                continue;
            }
            
            // 绘制怪物
            bool hasEnemy = false;
            bool enemyFlicker = false;
            for (const auto& e : enemies) {
                if (e.isAlive() && e.getX() == x && e.getY() == y) {
                    hasEnemy = true;
                    enemyFlicker = e.isFlickering();
                    break;
                }
            }
            
            if (hasEnemy) {
                cout << (enemyFlicker ? "X " : "E ");
            } 
            // 绘制终点门
            else if (x == goal.x && y == goal.y) {
                cout << "O "; 
            } 
            // 绘制地形
            else {
                TileType tile = gameMap.getTileAt(x, y);
                switch (tile) {
                    case TileType::Wall:       cout << "[]"; break;
                    case TileType::Platform:   cout << "=="; break;
                    case TileType::SpikeUp:    cout << "/\\"; break;
                    case TileType::Void:       cout << "~~"; break;
                    case TileType::Empty:      cout << "  "; break;
                    default:                   cout << "  "; break;
                }
            }
        }
        if (y < 19) cout << '\n';
    }
    cout << flush;
}

int main() {
    system("chcp 65001");
    system("mode con cols=110 lines=30"); 
    system("cls");
    
    int currentLevel = 1;
    bool gameBeaten = false;

    while (!gameBeaten) {
        string mapFile = "map" + to_string(currentLevel) + ".csv";
        Map gameMap(50, 20);
        
        if (!gameMap.loadFromCSV(mapFile)) {
            system("cls");
            cout << "\n\n\t\tTHE END\n\t\t你完成了所有的试炼，骑士！\n\n";
            break; 
        }

        // ==========================================
        // 【核心】从地图中读取主角和传送门坐标
        // ==========================================
        SpawnPoint pSpawn = gameMap.getPlayerSpawn();
        SpawnPoint goal = gameMap.getGoalPoint();
        
        Player player(pSpawn.x, pSpawn.y); 
        
        vector<Enemy> enemies;
        for (const auto& spawn : gameMap.getEnemySpawns()) {
            if (spawn.type == 8) {
                enemies.emplace_back(spawn.x, spawn.y, 20); 
            }
        }

        bool levelComplete = false;
        auto lastTime = chrono::high_resolution_clock::now();
        bool lastJumpState = false; 
        combatLog = "进入第 " + to_string(currentLevel) + " 层...";

        while (player.isAlive() && !levelComplete) {
            if (KEY_PRESSED('Q')) { gameBeaten = true; break; }

            auto currentTime = chrono::high_resolution_clock::now();
            float dt = chrono::duration<float>(currentTime - lastTime).count();
            lastTime = currentTime;
            if (dt > 0.05f) dt = 0.05f;

            int moveDir = 0;
            if (KEY_PRESSED('A')) moveDir -= 1;
            if (KEY_PRESSED('D')) moveDir += 1;
            player.setMoveIntent(moveDir);

            bool jumpHeld = KEY_PRESSED('K');
            bool jumpPressed = jumpHeld && !lastJumpState; 
            lastJumpState = jumpHeld;
            player.processJump(jumpPressed, jumpHeld);

            bool downHeld = KEY_PRESSED('S');

            static int attackCd = 0;
            if (attackCd > 0) attackCd--;
            if (KEY_PRESSED('J') && attackCd <= 0) {
                player.attack(enemies, gameMap, downHeld);
                attackCd = 15;
            }

            player.update(gameMap, dt);
            
            for (auto& enemy : enemies) {
                if (enemy.isAlive()) {
                    enemy.update(gameMap, player, dt);
                }
            }

            // 环境伤害检测
            TileType bodyTile = gameMap.getTileAt(player.getX(), player.getY());
            TileType footTile = gameMap.getTileAt(player.getX(), player.getY() + 1);

            if (bodyTile == TileType::SpikeUp || footTile == TileType::SpikeUp || player.getY() >= 19) {
                player.takeDamage(20, player.getX(), gameMap); 
                combatLog = "踩到地刺/坠渊！HP -20";
            }

            // 通关判定：检测是否到达读取出的门坐标
            if (player.getX() == goal.x && player.getY() == goal.y) {
                levelComplete = true;
                combatLog = "抵达出口！准备传送...";
            }

            renderGame(gameMap, player, enemies, currentLevel, goal);
            Sleep(15); 
        }

        if (!player.isAlive()) {
            system("cls");
            cout << "\n\n\t\tYOU DIED\n\n";
            break; 
        }
        if (levelComplete) {
            Sleep(800); 
            system("cls");
            currentLevel++; 
        }
    }
    return 0;
}