#include "../头文件/map.h"
#include <iostream>
#include <fstream>  // 新增：File Stream，用于读写文件
#include <sstream>  // 新增：String Stream，用于切割字符串
using namespace std;
// 构造函数的实现
Map::Map(int w, int h) : width(w), height(h) {
    grid.assign(height, std::vector<TileType>(width, TileType::Empty));
    buildBorders();
}

// 辅助函数的实现
void Map::buildBorders() {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (y == 0 || y == height - 1 || x == 0 || x == width - 1) {
                grid[y][x] = TileType::Wall;
            }
        }
    }
}

// 打印功能的实现
void Map::printMap() const {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // 使用 switch 语句比一堆 if-else 更清晰高效
            switch (grid[y][x]) {
                case TileType::Wall:       std::cout << "[]"; break; // 用方括号代表墙壁更厚实
                case TileType::Empty:      std::cout << "  "; break; // 空气用空格，画面更干净
                case TileType::Platform:   std::cout << "=="; break; // 双横线代表平台
                case TileType::Void:       std::cout << "~~"; break; // 波浪号代表虚空深渊
                case TileType::SpikeUp:    std::cout << "/\\"; break; // 朝上的尖刺
                case TileType::SpikeDown:  std::cout << "\\/"; break; // 朝下的尖刺
                case TileType::SpikeLeft:  std::cout << "<<"; break;
                case TileType::SpikeRight: std::cout << ">>"; break;
                default:                   std::cout << "??"; break; // 未知错误瓦片
            }
        }
        std::cout << std::endl;
    }
}

// 获取坐标瓦片（提供给外部做物理判断）
TileType Map::getTileAt(int x, int y) const {
    // 越界保护
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return TileType::Wall; // 默认地图外围都是实心墙
    }
    return grid[y][x];
}

// 数据驱动的体现：通过传入数据改变地图，而不是创建子类
void Map::loadData(const std::vector<std::vector<int>>& rawData) {
    // 实际项目中，这些 rawData 可能是从 txt 或 csv 文件中读取出来的
    for (int y = 0; y < height && y < rawData.size(); ++y) {
        for (int x = 0; x < width && x < rawData[y].size(); ++x) {
            grid[y][x] = static_cast<TileType>(rawData[y][x]);
        }
    }
}
    bool Map::loadFromCSV(const std::string& filename) {
    std::ifstream file(filename); // 尝试打开传入路径的文件
    
    // 错误处理：如果文件不存在或被占用，报错并返回 false
    if (!file.is_open()) {
        std::cerr << "错误：无法打开地图文件 -> " << filename << std::endl;
        return false;
    }

    std::vector<std::vector<int>> fileData; // 准备一个空的二维数组来装数据
    std::string line;

    // 外层循环：逐行读取文件，直到读完
    while (std::getline(file, line)) {
        std::vector<int> row;
        std::stringstream ss(line); // 把读到的这一行变成一个“流”
        std::string cell;

        // 内层循环：在这一行流中，以逗号 ',' 为界限，逐个切割单元格
        while (std::getline(ss, cell, ',')) {
            try {
                // std::stoi 将切割出来的纯数字字符串转换为 int
                row.push_back(std::stoi(cell));
            } catch (...) {
                // 如果遇到非数字字符（比如行尾多余的空格），忽略它，防止程序崩溃
            }
        }
        
        // 如果这一行确实读到了数据，就把它加入总数据中
        if (!row.empty()) {
            fileData.push_back(row);
        }
    }

    file.close(); // 养成好习惯：读完文件后关闭它

    // 重点：复用我们之前写好的 loadData 函数！
    // 直接把读出来的纯数字二维数组，转换为地图的 TileType 枚举
    loadData(fileData); 
    return true;
}
