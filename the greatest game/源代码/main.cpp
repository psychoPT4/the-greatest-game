#include <iostream>
#include <vector>
#include <string>
#include "../头文件/map.h"
using namespace std;
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