#pragma once
#include <string>
#include "map.h"
#include "Player.h" 

class Enemy {
private:
    std::string name;
    int x, y;
    float realX, realY;
    float hbWidth, hbHeight;
    int hp, maxHp, baseDamage;
    bool alive;
    int flickerTimer;

    int enemyType; // 0: 爬虫, 1: 飞虫
    float spawnY;  // 记录出生高度
    int moveDirection, attackCooldown;
    float crawlerSpeed;
    int spawnX;

    int currentFrame;
    float animTimer;
    int totalFrames;
    int aiState = 0;
    float stateTimer = 0.0f;
public:
    Enemy(int startX, int startY, int type);

    int getX() const { return x; }
    int getY() const { return y; }
    float getRealX() const { return realX; }
    float getRealY() const { return realY; }
    bool isAlive() const { return alive; }
    bool isFlickering() const { return flickerTimer > 0; }
    int getVelocityX() const { return moveDirection; }
    int getCurrentFrame() const { return currentFrame; }
    int getTotalFrames() const { return totalFrames; }

    Hitbox getHitbox() const {
        return { realX + (1.0f - hbWidth) / 2.0f, realX + (1.0f + hbWidth) / 2.0f,
                 realY + 1.0f - hbHeight, realY + 1.0f };
    }
    std::string getName() const { return name; }
    void update(const Map& gameMap, Player& player, float dt);
    bool takeDamage(int damage, int sourceX, const Map& gameMap);
};