#include <iostream>
#include <vector>
#include <string>
#include <conio.h>   
#include <windows.h> 
#include "../头文件/map.h"
#include "../头文件/Role.h"

using namespace std;

// 全局渲染函数：放弃坐标控制，改用极速全清
void renderGame(const Map& gameMap, const Player& player, const Enemy& enemy) {
    // 强制每帧彻底清空控制台历史，防止堆叠
    system("cls"); 
    
    // 快速拼接画面到缓冲区
    string output = "";
    output += "=== 《Fulfilling Knight》 暴力渲染模式 ===\n";
    output += "操作: [A][D]移动 [K]跳跃 [Q]退出\n";
    output += "坐标: (" + to_string(player.getX()) + ", " + to_string(player.getY()) + ")\n";
    output += "------------------------------------------\n";

    for (int y = 0; y < 15; ++y) {       
        for (int x = 0; x < 30; ++x) {   
            if (player.getX() == x && player.getY() == y) output += "@ "; 
            else if (enemy.getX() == x && enemy.getY() == y) output += "E "; 
            else {
                TileType tile = gameMap.getTileAt(x, y);
                if (tile == TileType::Wall) output += "[]";
                else if (tile == TileType::Platform) output += "==";
                else output += "  ";
            }
        }
        output += "\n";
    }
    
    // 一口气喷射出所有字符，减少 system("cls") 带来的闪烁感
    cout << output << flush; 
}

int main() {
    // 你的性能外挂，这次它将配合 system("cls") 发挥奇效
    ios::sync_with_stdio(false);
    cin.tie(0);
    
    system("chcp 65001"); // 解决乱码

    Map gameMap(30, 15); 
    Player player(5, 2);     
    Enemy enemy(15, 10, 50); 

    if (!gameMap.loadFromCSV("map1.csv") && !gameMap.loadFromCSV("源代码/map1.csv")) {
        cout << "Error: Map file not found!" << endl;
        return -1;
    }

    while (true) {
        if (_kbhit()) {
            char ch = _getch(); 
            if (ch == 'q' || ch == 'Q') break;
            if (ch == 'a' || ch == 'A') player.move(-1, 0, gameMap);
            if (ch == 'd' || ch == 'D') player.move(1, 0, gameMap);
            if (ch == 'k' || ch == 'K') player.jump();
        }

        player.update(gameMap);
        enemy.update(gameMap);
        renderGame(gameMap, player, enemy);

        Sleep(60); // 稍微调慢一点帧率，给系统清屏留出时间
    }

    return 0;
}