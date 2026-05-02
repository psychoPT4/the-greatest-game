#include "../头文件/map.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

Map::Map(int w, int h) : width(w), height(h) {
    grid.assign(height, std::vector<TileType>(width, TileType::Empty));
    buildBorders();
    // 设置默认安全坐标，防止地图文件忘填 10 和 11
    playerSpawn = {4, 15, 10}; 
    goalPoint = {46, 15, 11};  
}

void Map::buildBorders() {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (y == 0 || y == height - 1 || x == 0 || x == width - 1) {
                grid[y][x] = TileType::Wall;
            }
        }
    }
}

TileType Map::getTileAt(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return TileType::Wall;
    return grid[y][x];
}

void Map::loadData(const std::vector<std::vector<int>>& rawData) {
    for (int y = 0; y < height && y < rawData.size(); ++y) {
        for (int x = 0; x < width && x < rawData[y].size(); ++x) {
            grid[y][x] = static_cast<TileType>(rawData[y][x]);
        }
    }
}

bool Map::loadFromCSV(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "错误：无法打开地图文件 -> " << filename << std::endl;
        return false;
    }

    enemySpawns.clear();
    std::vector<std::vector<int>> fileData;
    std::string line;
    int currentY = 0;

    while (std::getline(file, line)) {
        std::vector<int> row;
        std::stringstream ss(line);
        std::string cell;
        int currentX = 0;

        while (std::getline(ss, cell, ',')) {
            try {
                int val = std::stoi(cell);
                // 【分类拦截数据驱动标量】
                if (val == 8 || val == 9) {
                    enemySpawns.push_back({currentX, currentY, val});
                    row.push_back(0); // 怪物非实心，存为空气
                } else if (val == 10) {
                    playerSpawn = {currentX, currentY, val};
                    row.push_back(0); // 玩家非实心，存为空气
                } else if (val == 11) {
                    goalPoint = {currentX, currentY, val};
                    row.push_back(0); // 传送门允许穿过，存为空气
                } else {
                    row.push_back(val);
                }
            } catch (...) { }
            currentX++;
        }
        if (!row.empty()) fileData.push_back(row);
        currentY++;
    }

    file.close();
    loadData(fileData); 
    return true;
}
