#include <iostream>
#include <vector>
#include <string>
#include "../头文件/map.h"
#include "../头文件/Role.h"
using namespace std;
// ==========================================
// 全局渲染函数：将地图和实体融合显示
// ==========================================
void renderGame(const Map& gameMap, const Player& player, const Enemy& enemy) {
    // 简单清屏指令（Windows 用 "cls"，Linux/Mac 用 "clear"）
    // 这能让画面保持在同一个位置刷新，而不是一直往下滚
    system("cls"); 
    
    std::cout << "=== 《Fulfilling Knight》 测试版 ===" << std::endl;
    std::cout << "玩家HP: " << /* 这里未来可以接入 player.getHp() */ " 经验: " << std::endl;
    std::cout << "------------------------------------" << std::endl;

    // 假设地图的高度是 50，宽度也是 50 (根据你初始化的 gameMap(50,50))
    // 为了终端显示得下，你可以暂时把 map.csv 做小一点，比如 20x10
    for (int y = 0; y < 15; ++y) {       // 假设我们只渲染前 15 行
        for (int x = 0; x < 30; ++x) {   // 假设我们只渲染前 30 列
            
            // 1. 优先渲染玩家
            if (player.getX() == x && player.getY() == y) {
                std::cout << "@ "; // 玩家用 @ 表示
            }
            // 2. 其次渲染怪物
            else if (enemy.getX() == x && enemy.getY() == y) {
                std::cout << "E "; // 怪物用 E 表示
            }
            // 3. 最后渲染地图地形
            else {
                TileType tile = gameMap.getTileAt(x, y);
                switch (tile) {
                    case TileType::Wall:       std::cout << "[]"; break;
                    case TileType::Platform:   std::cout << "=="; break;
                    case TileType::Empty:      std::cout << "  "; break;
                    default:                   std::cout << "  "; break;
                }
            }
        }
        std::cout << std::endl;
    }
}
int main() {
    // 1. 建立关卡播放列表：存储所有地图文件的名称
    vector<std::string> levels = {
        "level1.csv",
        "level2.csv",
        "level3.csv" // 假设你有这三个文件
    };

    // 2. 追踪玩家的关卡进度（0 代表第一个关卡）
    int currentLevelIndex = 0;
    
    // 实例化我们的地图对象
    Map gameMap(50,50); 

   cout << "=== 欢迎来到《fulfilling knight》C++重制版 ===" << endl;

    // 3. 游戏主循环模拟
    // 只要当前关卡索引还没超出列表总数，游戏就继续
    while (currentLevelIndex < levels.size()) {
        
        // 获取当前应该加载的文件名
        std::string currentFile = levels[currentLevelIndex];
        std::cout << "\n>>> 系统提示：正在加载关卡 " << currentLevelIndex + 1 
                  << " (" << currentFile << ") <<<" << std::endl;

        // 4. 动态调用：将文件名变量传给加载函数
        if (gameMap.loadFromCSV(currentFile)) {
            // 加载成功，渲染地图
            gameMap.printMap();
            
            // ---------------------------------------------------
            // 这里是模拟游戏过程：在真正的游戏中，这里会是角色的移动逻辑、
            // 碰撞检测、打怪等。直到角色碰到了“下一关的门”。
            // 我们现在用输入一个字符来模拟“玩家过关了”。
            // ---------------------------------------------------
            std::cout << "\n[模拟操作] 玩家正在探索中...\n";
            std::cout << "输入 'n' 并回车以进入下一关，输入 'q' 退出游戏：";
            char input;
            std::cin >> input;

            if (input == 'q' || input == 'Q') {
                std::cout << "玩家退出了游戏。" << std::endl;
                break; // 跳出循环，结束游戏
            } else if (input == 'n' || input == 'N') {
                // 玩家过关，关卡索引 +1
                currentLevelIndex++; 
            } else {
                std::cout << "未知指令，继续留在当前关卡。" << std::endl;
                // 这里没有 currentLevelIndex++，所以循环会再次加载当前关卡
            }

        } else {
            // 如果加载失败（比如你没创建 level2.csv）
            std::cerr << "致命错误：无法找到关卡文件，游戏崩溃！" << std::endl;
            break; 
        }
    }

    if (currentLevelIndex >= levels.size()) {
        std::cout << "\n恭喜你！通关了所有地图！" << std::endl;
    }

    return 0;
}