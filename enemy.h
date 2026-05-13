#pragma once
#include <string>
#include <vector>
#include <cmath>
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

    int enemyType; // 0: Crawler, 1: Flyer, 2: False Knight Boss
    float spawnY;
    int moveDirection, attackCooldown;
    float crawlerSpeed;
    int spawnX;

    int currentFrame;
    float animTimer;
    int totalFrames;
    int aiState = 0;
    float stateTimer = 0.0f;

    int bossState;
    bool isEnraged;
    int maceSwingDir;

    std::vector<Shockwave> shockwaves;

public:
    Enemy(int startX, int startY, int type);

    int getX() const { return x; }
    int getY() const { return y; }
    float getRealX() const { return realX; }
    float getRealY() const { return realY; }
    int getHp() const { return hp; }
    int getMaxHp() const { return maxHp; }
    bool isAlive() const { return alive; }
    bool isFlickering() const { return flickerTimer > 0; }
    int getFlickerTimer() const { return flickerTimer; }
    int getVelocityX() const { return moveDirection; }
    int getMoveDirection() const { return moveDirection; }
    int getCurrentFrame() const { return currentFrame; }
    int getTotalFrames() const { return totalFrames; }
    int getEnemyType() const { return enemyType; }
    int getAiState() const { return aiState; }
    float getStateTimer() const { return stateTimer; }
    bool getIsEnraged() const { return isEnraged; }
    int getAttackCooldown() const { return attackCooldown; }
    float getAnimTimer() const { return animTimer; }
    int getMaceSwingDir() const { return maceSwingDir; }
    float getSpawnY() const { return spawnY; }
    int getSpawnX() const { return spawnX; }

    Hitbox getHitbox() const {
        if (enemyType == 2) {
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

    void restoreState(float rx, float ry, int _hp, int _maxHp, bool _alive,
                      int _flickerTimer, int _moveDirection, int _attackCooldown,
                      int _currentFrame, float _animTimer, int _aiState, float _stateTimer,
                      int _bossState, bool _isEnraged, int _maceSwingDir) {
        realX = rx; realY = ry; x = (int)std::round(realX); y = (int)std::round(realY);
        hp = _hp; maxHp = _maxHp; alive = _alive;
        flickerTimer = _flickerTimer; moveDirection = _moveDirection;
        attackCooldown = _attackCooldown; currentFrame = _currentFrame;
        animTimer = _animTimer; aiState = _aiState; stateTimer = _stateTimer;
        bossState = _bossState; isEnraged = _isEnraged; maceSwingDir = _maceSwingDir;
        shockwaves.clear();
    }
};