#pragma execution_character_set("utf-8")
#include "map.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include "raylib.h"

Map::Map() : width(0), height(0) {
    playerSpawn = { 0, 0, 10 };
    goalPoint = { 0, 0, 11 };
}
Map::~Map() {
    unloadTheme();
}
void Map::unloadTheme() {
    for (auto& layer : bgLayers) {
        if (layer.tex.id != 0) UnloadTexture(layer.tex);
    }
    bgLayers.clear();

    if (texGroundTop.id != 0) UnloadTexture(texGroundTop);
    if (texGroundDeep.id != 0) UnloadTexture(texGroundDeep);

    texGroundTop.id = 0;
    texGroundDeep.id = 0;
}
bool Map::loadLevel(int levelIndex) {
    unloadTheme();
    grid.clear();
    enemySpawns.clear();

    std::string csvPath = "map" + std::to_string(levelIndex) + ".csv";
    std::ifstream file(csvPath);
    if (!file.is_open()) {
        std::cout << "致命错误：找不到地图文件 " << csvPath << std::endl;
        return false;
    }

    std::string line;
    int y = 0;
    int maxWidth = 0;
    while (std::getline(file, line)) {
        std::vector<TileType> row;
        std::stringstream ss(line);
        std::string cell;
        int x = 0;

        while (std::getline(ss, cell, ',')) {
            if (cell.empty()) continue;

            int val = std::stoi(cell);
            row.push_back(static_cast<TileType>(val));

            if (val == 10) playerSpawn = { x, y, val };
            else if (val == 11) goalPoint = { x, y, val };
            else if (val == 8 || val == 9) enemySpawns.push_back({ x, y, val }); // 🌟 包含爬虫(8)与飞虫(9)

            x++;
        }
        if (x > maxWidth) maxWidth = x;
        grid.push_back(row);
        y++;
    }
    width = maxWidth;
    height = y;
    file.close();

    if (levelIndex == 1) {
        texGroundTop = LoadTexture("bg_plain/plain_grass.png");
        texGroundDeep = LoadTexture("bg_plain/plain_mud.png");

        float speeds[5] = { 0.05f, 0.1f, 0.15f, 0.2f, 0.3f };
        for (int i = 1; i <= 5; i++) {
            Texture2D t = LoadTexture(("bg_plain/" + std::to_string(i) + ".png").c_str());
            if (t.id != 0) bgLayers.push_back({ t, speeds[i - 1] });
        }
    }
    else if (levelIndex == 2) {
        texGroundTop = LoadTexture("bg_cave/cave_ground.png");
        texGroundDeep = LoadTexture("bg_cave/cave_ground.png");

        float speeds[9] = { 0.02f, 0.05f, 0.08f, 0.12f, 0.18f, 0.22f, 0.28f, 0.35f, 0.45f };
        for (int i = 1; i <= 9; i++) {
            Texture2D t = LoadTexture(("bg_cave/" + std::to_string(i) + ".png").c_str());
            if (t.id != 0) bgLayers.push_back({ t, speeds[i - 1] });
        }
    }

    return true;
}

TileType Map::getTileAt(int x, int y) const {
    if (y < 0) return TileType::Empty;
    if (y >= height) return TileType::Void;
    if (x < 0 || x >= width) return TileType::Wall;
    return grid[y][x];
}