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

    int enemyType; // 0: 爬虫, 1: 飞虫, 2: 假骑士 Boss
    float spawnY;  // 记录出生高度
    int moveDirection, attackCooldown;
    float crawlerSpeed;
    int spawnX;

    int currentFrame;
    float animTimer;
    int totalFrames;
    int aiState = 0;
    float stateTimer = 0.0f; // 🌟 统一复用的核心计时器

    int bossState;
    bool isEnraged;   // 二阶段狂暴标志
    int maceSwingDir; // 挥锤方向
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

    std::vector<Shockwave> shockwaves;

    Hitbox getHitbox() const {
        if (enemyType == 2) {
            // 🌟 Boss 专属巨大碰撞箱 (宽 2.5 格, 高 3.0 格)，底部完美锚定地面
            return { realX - 1.25f, realX + 1.25f, realY - 3.0f, realY };
        }
        return { realX + (1.0f - hbWidth) / 2.0f, realX + (1.0f + hbWidth) / 2.0f,
                 realY + 1.0f - hbHeight, realY + 1.0f };
    }
    std::string getName() const { return name; }
    void update(const Map& gameMap, Player& player, float dt);
    bool takeDamage(int damage, int sourceX, const Map& gameMap);
    std::vector<Shockwave>& getShockwaves() { return shockwaves; }
    int getBossState() const { return bossState; }
};