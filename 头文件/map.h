#ifndef MAP_H
#define MAP_H

#include <vector>
#include <string>

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
    SpawnBoss = 9,    // 怪物：Boss 出生点
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
    std::vector<std::vector<TileType>> grid;
    
    // 数据驱动容器
    std::vector<SpawnPoint> enemySpawns; 
    SpawnPoint playerSpawn; 
    SpawnPoint goalPoint;   

    void buildBorders();

public:
    Map(int w, int h);
    void printMap() const;
    TileType getTileAt(int x, int y) const;
    void loadData(const std::vector<std::vector<int>>& rawData);
    bool loadFromCSV(const std::string& filename);
    
    // 提供给外部的接口
    const std::vector<SpawnPoint>& getEnemySpawns() const { return enemySpawns; }
    SpawnPoint getPlayerSpawn() const { return playerSpawn; }
    SpawnPoint getGoalPoint() const { return goalPoint; }
};

#endif // MAP_H