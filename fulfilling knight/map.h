#pragma once
#include <vector>
#include <string>
#include "raylib.h"
using namespace std;
struct ParallaxLayer {
    Texture2D tex;
    float scrollSpeed;
};
enum class TileType {
    Empty = 0,
    Wall = 1,
    Platform = 2,
    Void = 3,
    SpikeUp = 4,
    SpikeDown = 5,
    SpikeLeft = 6,
    SpikeRight = 7,
    SpawnCrawler = 8, // 怪物：爬虫怪出生点
    SpawnFlyer = 9,   // 🌟 新增：飞虫出生点
    SpawnPlayer = 10, // 主角出生点
    LevelGoal = 11    // 关卡终点传送门
};

struct SpawnPoint {
    int x, y;
    int type;
};

class Map {
private:
    int width;
    int height;
    std::vector<std::vector<TileType>> grid; // 动态二维数组，打破 100x30 的结界！

    std::vector<SpawnPoint> enemySpawns;
    SpawnPoint playerSpawn;
    SpawnPoint goalPoint;

    std::vector<ParallaxLayer> bgLayers;
    Texture2D texGroundTop;  // 表层泥土/草地 (如 plain_grass.png)
    Texture2D texGroundDeep; // 深层泥土/岩石 (如 plain_mud.png 或 cave_ground.png)
public:
    // 【核心改动】：构造函数不再需要传写死的宽和高了！
    Map();
    ~Map(); // 【极其重要】：析构函数，用于销毁地图时自动释放显存！

    bool loadLevel(int levelIndex);
    void unloadTheme(); // 卸载当前主题的美术资源
    TileType getTileAt(int x, int y) const;

    // 获取动态的宽高
    int getWidth() const { return width; }
    int getHeight() const { return height; }

    const std::vector<SpawnPoint>& getEnemySpawns() const { return enemySpawns; }
    SpawnPoint getPlayerSpawn() const { return playerSpawn; }
    SpawnPoint getGoalPoint() const { return goalPoint; }
    const std::vector<ParallaxLayer>& getBgLayers() const { return bgLayers; }
    Texture2D getTexGroundTop() const { return texGroundTop; }
    Texture2D getTexGroundDeep() const { return texGroundDeep; }
};