#ifndef ROLE_H
#define ROLE_H

#include <string>
#include <vector>
#include <cmath>
#include "map.h"

// 战斗结果结构体
struct AttackResult {
    bool pogoSuccess;
    int totalXp;
    bool hitSomething;
    AttackResult() : pogoSuccess(false), totalXp(0),hitSomething(false){}
};

struct Hitbox {
    float left, right, top, bottom;
    bool intersects(const Hitbox& other) const {
        return left <= other.right && right >= other.left &&
            top <= other.bottom && bottom >= other.top;
    }
};

class Role {
protected:
    std::string name;
    int x, y;
    float realX, realY;

    float hbWidth;
    float hbHeight;

    int hp, maxHp;
    int baseDamage;
    bool isGrounded;
    float velocityX, velocityY;
    int facingDirection;
    bool alive;
    int flickerTimer;

public:
    Role(std::string n, int startX, int startY, int maxHealth, int dmg);
    virtual ~Role() = default;

    virtual void update(const Map& gameMap, float dt) = 0;
    virtual void takeDamage(int damage, int sourceX, const Map& gameMap);

    int getX() const { return x; }
    int getY() const { return y; }
    float getRealX() const { return realX; }
    float getRealY() const { return realY; }
    int getHp() const { return hp; }
    int getMaxHp() const { return maxHp; }
    bool isAlive() const { return alive; }
    bool isFlickering() const { return flickerTimer > 0; }

    Hitbox getHitbox() const {
        return {
            realX + (1.0f - hbWidth) / 2.0f,
            realX + (1.0f + hbWidth) / 2.0f,
            realY + 1.0f - hbHeight,
            realY + 1.0f
        };
    }

    void setRealPos(float nx, float ny) {
        realX = nx; realY = ny;
        x = (int)round(realX); y = (int)round(realY);
        velocityX = 0.0f; velocityY = 0.0f;
        flickerTimer = 30;
    }
};

class Player : public Role {
private:
    int level, currentExp, expToNextLevel;
    int mana, jumpCount;
    int moveIntent;

    float maxRunSpeed, runAccel, groundFriction, airDrag;
    float gravity, jumpForce, maxFallSpeed;

    // Dash 核心物理状态
    bool isDashing;
    float dashSpeed;
    float dashDuration;
    float dashTimer;
    float dashCooldown;
    float dashCooldownTimer;
    int dashDirection;
    bool isRunningMode;
public:
    Player(int startX, int startY);
    void update(const Map& gameMap, float dt) override;
    void startDash();
    bool getIsDashing() const { return isDashing; }
    void setMoveIntent(int dir);
    void processJump(bool jumpPressed, bool jumpHeld);
    void takeDamage(int damage, int sourceX, const Map& gameMap) override;
    AttackResult attack(std::vector<class Enemy>& enemies, const Map& gameMap, bool downPressed);
    void setRunningMode(bool run) { isRunningMode = run; }
    int getLevel() const { return level; }
    int getExp() const { return currentExp; }
    int getExpToNext() const { return expToNextLevel; }
    void addExp(int amount);
};

class Enemy : public Role {
private:
    int moveDirection, expReward, attackCooldown;
    float moveTimer;
    int spawnX;
public:
    Enemy(int startX, int startY, int expGive);
    void update(const Map& gameMap, Player& player, float dt);
    void update(const Map& gameMap, float dt) override {}
    void takeDamage(int damage, int sourceX, const Map& gameMap) override;
    int getExpReward() const { return expReward; }
};

#endif