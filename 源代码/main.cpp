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
    cout << "=== 《Fulfilling Knight》 物理重构版 ===                     \n";
    cout << "操作: [A][D]横移 [K]跳跃 [J]攻击 [Q]退出                     \n";
    cout << "状态: HP [" << player.getHp() << "/" << player.getMaxHp() << "] | 坐标: (" << player.getX() << "," << player.getY() << ")          \n";
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

    // 出生点：放在中间平台上方
    Player player(15, 5);
    Enemy enemy(25, 12, 50);

    while (true) {
        // 先处理输入：确保按键瞬间逻辑生效
        if (_kbhit()) {
            char ch = _getch();
            if (ch == 'q') break;
            if (ch == 'a') player.move(-1, 0, gameMap);
            if (ch == 'd') player.move(1, 0, gameMap);
            if (ch == 'k') player.jump();
            if (ch == 'j') player.attack(enemy, gameMap);
        }

        // 随后进行物理模拟
        player.update(gameMap);
        enemy.update(gameMap, player);

        // 最后渲染
        renderGame(gameMap, player, enemy);

        Sleep(50); // 维持 20FPS 的稳定感
    }
    return 0;
}