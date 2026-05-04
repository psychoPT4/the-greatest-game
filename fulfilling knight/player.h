#pragma once
#include <string>
#include <vector>
#include "map.h"

// 碰撞盒定义
struct Hitbox {
    float left, right, top, bottom;
    bool intersects(const Hitbox& other) const {
        return left <= other.right && right >= other.left &&
            top <= other.bottom && bottom >= other.top;
    }
};

// 攻击结果
struct AttackResult {
    bool pogoSuccess;
    int totalXp;
    bool hitSomething;
    AttackResult() : pogoSuccess(false), totalXp(0), hitSomething(false) {}
};

class Enemy; // 前向声明，防止交叉引用

class Player {
private:
    std::string name;
    int x, y;
    float realX, realY;
    float hbWidth, hbHeight;
    int hp, maxHp, baseDamage;
    bool isGrounded;
    float velocityX, velocityY;
    int facingDirection;
    bool alive;
    int flickerTimer;

    int level, currentExp, expToNextLevel, mana, jumpCount, moveIntent;
    float maxRunSpeed, runAccel, groundFriction, airDrag;
    float gravity, jumpForce, maxFallSpeed;

    bool isDashing;
    float dashSpeed, dashDuration, dashTimer, dashCooldown, dashCooldownTimer;
    int dashDirection;
    bool isRunningMode;

public:
    Player(int startX, int startY);

    int getX() const { return x; }
    int getY() const { return y; }
    float getRealX() const { return realX; }
    float getRealY() const { return realY; }
    int getHp() const { return hp; }
    int getMaxHp() const { return maxHp; }
    bool isAlive() const { return alive; }
    bool isFlickering() const { return flickerTimer > 0; }
    bool getIsGrounded() const { return isGrounded; }
    float getVelocityY() const { return velocityY; }
    bool getIsDashing() const { return isDashing; }
    int getLevel() const { return level; }
    int getExp() const { return currentExp; }
    int getExpToNext() const { return expToNextLevel; }

    Hitbox getHitbox() const {
        return { realX + (1.0f - hbWidth) / 2.0f, realX + (1.0f + hbWidth) / 2.0f,
                 realY + 1.0f - hbHeight, realY + 1.0f };
    }

    void setRealPos(float nx, float ny);
    void takeDamage(int damage, int sourceX, const Map& gameMap);
    void setMoveIntent(int dir);
    void processJump(bool jumpPressed, bool jumpHeld);
    void startDash();
    void update(const Map& gameMap, float dt);
    AttackResult attack(std::vector<Enemy>& enemies, const Map& gameMap, bool downPressed);
    void setRunningMode(bool run) { isRunningMode = run; }
    void addExp(int amount);
};
