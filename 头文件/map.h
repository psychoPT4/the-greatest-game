#ifndef MAP_H
#define MAP_H

#include <vector>
#include <string>
// 扩充瓦片类型枚举
enum class TileType {
    Empty = 0,       // 空气
    Wall = 1,        // 实体墙壁
    Platform = 2,    // 半透平台 (可从下往上跳穿)
    Void = 3,        // 虚空 (掉落死亡/重置)
    SpikeUp = 4,     // 地刺 (朝上)
    SpikeDown = 5,   // 地刺 (朝下)
    SpikeLeft = 6,   // 地刺 (朝左)
    SpikeRight = 7   // 地刺 (朝右)
};

class Map {
private:
    int width;
    int height;
    std::vector<std::vector<TileType>> grid;

    void buildBorders();

public:
    Map(int w, int h);
    void printMap() const;
    TileType getTileAt(int x, int y) const;
    void loadData(const std::vector<std::vector<int>>& rawData);
    bool loadFromCSV(const std::string& filename);
};

#endif // MAP_H