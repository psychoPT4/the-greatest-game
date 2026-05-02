#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#include <chrono>
#include "map.h"
#include "Role.h"
#include "Camera.h"

using namespace std;

string combatLog = "冒险继续...";
float safeX, safeY;
#define KEY_PRESSED(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 1 : 0)

// renderGame 和 resetCursor 保持原样不动 ...
void resetCursor() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO ci = { 1, FALSE }; SetConsoleCursorInfo(hOut, &ci);
    SetConsoleCursorPosition(hOut, { 0, 0 });
}

void renderGame(const Map& gameMap, const Player& player, const vector<Enemy>& enemies, int currentLevel, SpawnPoint goal, GameCamera& cam) {
    resetCursor();
    cout << "=== 《Fulfilling Knight》 稳健物理融合版 ===                 \n";
    cout << "操作: [A][D]移动 [K]跳跃 [J]攻击 [S]+[J]下劈                 \n";
    printf("状态: HP [%3d/%3d] | 等级: %d | 经验: %d/%d                   \n",
        player.getHp(), player.getMaxHp(), player.getLevel(), player.getExp(), player.getExpToNext());
    cout << "日志: " << (combatLog + "                                        ").substr(0, 60) << "\n";
    cout << "-------------------------------------------------------------\n";

    for (int sy = 0; sy < cam.viewH; ++sy) {
        for (int sx = 0; sx < cam.viewW; ++sx) {
            int worldX = (int)cam.x + sx;
            int worldY = (int)cam.y + sy;

            if (player.getX() == worldX && player.getY() == worldY) { cout << (player.isFlickering() ? "* " : "@ "); continue; }
            bool enemyDrawn = false;
            for (const auto& e : enemies) {
                if (e.isAlive() && e.getX() == worldX && e.getY() == worldY) {
                    cout << (e.isFlickering() ? "X " : "E "); enemyDrawn = true; break;
                }
            }
            if (enemyDrawn) continue;
            if (worldX == goal.x && worldY == goal.y) { cout << "O "; continue; }

            TileType t = gameMap.getTileAt(worldX, worldY);
            switch (t) {
            case TileType::Wall:     cout << "[]"; break;
            case TileType::Platform: cout << "=="; break;
            case TileType::SpikeUp:  cout << "/\\"; break;
            case TileType::Void:     cout << "~~"; break;
            default:                 cout << "  "; break;
            }
        }
        if (sy < cam.viewH - 1) cout << '\n';
    }
}

int main() {
    system("chcp 65001 & cls");
    system("mode con cols=110 lines=32");

    int currentLevel = 1;
    bool gameQuit = false;
    GameCamera cam(50, 20);

    while (!gameQuit) {
        Map gameMap(100, 30);
        if (!gameMap.loadFromCSV("map" + to_string(currentLevel) + ".csv")) break;

        SpawnPoint pSpawn = gameMap.getPlayerSpawn();
        SpawnPoint goal = gameMap.getGoalPoint();
        Player player(pSpawn.x, pSpawn.y);
        safeX = (float)pSpawn.x; safeY = (float)pSpawn.y;

        vector<Enemy> enemies;
        for (const auto& s : gameMap.getEnemySpawns()) {
            if (s.type == 8) enemies.emplace_back(s.x, s.y, 40);
        }

        auto lastTime = chrono::high_resolution_clock::now();
        bool lastJump = false, levelComplete = false;
        static int attackCd = 0;

        while (player.isAlive() && !levelComplete) {
            if (KEY_PRESSED('Q')) { gameQuit = true; break; }

            auto now = chrono::high_resolution_clock::now();
            float dt = chrono::duration<float>(now - lastTime).count();
            lastTime = now;
            if (dt > 0.05f) dt = 0.05f;

            int moveDir = (KEY_PRESSED('D') - KEY_PRESSED('A'));
            player.setMoveIntent(moveDir);

            bool jump = KEY_PRESSED('K');
            player.processJump(jump && !lastJump, jump);
            lastJump = jump;

            if (attackCd > 0) attackCd--;
            if (KEY_PRESSED('J') && attackCd <= 0) {
                AttackResult result = player.attack(enemies, gameMap, KEY_PRESSED('S'));
                if (result.totalXp > 0) player.addExp(result.totalXp);
                attackCd = 12;
            }

            player.update(gameMap, dt);

            if (gameMap.getTileAt(player.getX(), player.getY() + 1) == TileType::Wall) {
                safeX = player.getRealX(); safeY = player.getRealY();
            }

            for (auto& e : enemies) e.update(gameMap, player, dt);

            // 【精准环境判定与回溯】
            Hitbox pBox = player.getHitbox();
            TileType footL = gameMap.getTileAt((int)pBox.left, (int)pBox.bottom);
            TileType footR = gameMap.getTileAt((int)pBox.right, (int)pBox.bottom);

            if (footL == TileType::Void || footR == TileType::Void || player.getY() >= 29) {
                player.takeDamage(player.getMaxHp() / 4, player.getX(), gameMap);
                player.setRealPos(safeX, safeY);
                combatLog = "坠入虚空！强制回溯并扣血。";
            }
            else if (footL == TileType::SpikeUp || footR == TileType::SpikeUp) {
                player.takeDamage(20, player.getX(), gameMap);
            }

            cam.update(player.getRealX(), player.getRealY(), 100, 30);
            if (player.getX() == goal.x && player.getY() == goal.y) levelComplete = true;

            renderGame(gameMap, player, enemies, currentLevel, goal, cam);
            Sleep(8);
        }
        if (!player.isAlive()) {
            system("cls"); cout << "\n\n\t\t[ 战败 ]\n\t勇敢的骑士，你的意志将在下一世传承。\n\n"; Sleep(2000); break;
        }
        currentLevel++;
    }
    return 0;
}