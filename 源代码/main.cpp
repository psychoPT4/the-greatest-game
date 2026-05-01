#include <iostream>
#include <string>
#include <conio.h>
#include <windows.h>
#include "../头文件/map.h"
#include "../头文件/Role.h"

using namespace std;

string combatLog = "探索开始...";

void resetCursor() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = {0, 0};
    SetConsoleCursorPosition(hOut, pos);
}

void renderGame(const Map& gameMap, const Player& player, const Enemy& enemy) {
    resetCursor();
    // 精简后的 UI，去掉了没用的坐标显示
    cout << "=== 《Fulfilling Knight》 物理重构版 ===                     \n";
    cout << "操作: [A][D]左右 [K]跳跃 [J]攻击 [Q]退出                     \n";
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

    while (true) {
        if (_kbhit()) {
            char ch = _getch();
            if (ch == 'q') break;
            if (ch == 'a') player.move(-1, 0, gameMap);
            if (ch == 'd') player.move(1, 0, gameMap);
            if (ch == 'k') player.jump();
            if (ch == 'j') player.attack(enemy, gameMap);
        }

        player.update(gameMap);
        enemy.update(gameMap, player);
        renderGame(gameMap, player, enemy);

        // 进一步降低 Sleep，提高响应速度，体感更流畅
        Sleep(25); 
    }
    return 0;
}