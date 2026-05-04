#pragma once
#include <string>
#include "map.h"
#include "Player.h" // ½èÓÃ Hitbox

class Enemy {
private:
    std::string name;
    int x, y;
    float realX, realY;
    float hbWidth, hbHeight;
    int hp, maxHp, baseDamage;
    bool alive;
    int flickerTimer;

    int moveDirection, expReward, attackCooldown;
    float crawlerSpeed;
    int spawnX;

    int currentFrame;
    float animTimer;
    int totalFrames;
public:
    Enemy(int startX, int startY, int expGive);

    int getX() const { return x; }
    int getY() const { return y; }
    float getRealX() const { return realX; }
    float getRealY() const { return realY; }
    bool isAlive() const { return alive; }
    bool isFlickering() const { return flickerTimer > 0; }
    int getExpReward() const { return expReward; }
    int getVelocityX() const { return moveDirection; }
    int getCurrentFrame() const { return currentFrame; }
    int getTotalFrames() const { return totalFrames; }

    Hitbox getHitbox() const {
        return { realX + (1.0f - hbWidth) / 2.0f, realX + (1.0f + hbWidth) / 2.0f,
                 realY + 1.0f - hbHeight, realY + 1.0f };
    }

    void update(const Map& gameMap, Player& player, float dt);
    void takeDamage(int damage, int sourceX, const Map& gameMap);
};
