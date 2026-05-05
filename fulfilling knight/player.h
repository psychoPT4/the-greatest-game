#pragma once
#include <string>
#include <vector>
#include "map.h"
enum MagicAction { MAGIC_NONE, CAST_SPELL, HEALED };
// 碰撞盒定义
struct Hitbox {
    float left, right, top, bottom;
    bool intersects(const Hitbox& other) const {
        return left <= other.right && right >= other.left &&
            top <= other.bottom && bottom >= other.top;
    }
};
struct Projectile {
    float x, y;
    float speed;
    int facingDir;
    float lifeTimer;
    bool active;

    Hitbox getHitbox() const {
        return { x - 0.5f, x + 1.5f, y - 0.8f, y + 0.8f };
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
    float stamina;
    float maxStamina;

    int level, currentExp, expToNextLevel, mana, maxMana,jumpCount, moveIntent;
    float maxRunSpeed, runAccel, groundFriction, airDrag;
    float gravity, jumpForce, maxFallSpeed;

    float ignorePlatformTimer;
    bool isDashing;
    float dashSpeed, dashDuration, dashTimer, dashCooldown, dashCooldownTimer;
    int dashDirection;
    bool isRunningMode;

    std::vector<Projectile> projectiles;

    // 【新增】：凝聚系统 (Focus)
    float focusTimer = 0.0f;
    bool isFocusing = false;
    float healFlashTimer = 0.0f; // 耀眼的白光计时器

public:
    Player(int startX, int startY);
    int getJumpCount() const { return jumpCount; }
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
    int getMana() const { return mana; }
    int getMaxMana() const { return maxMana; }
    float getStamina() const { return stamina; }
    float getMaxStamina() const { return maxStamina; }
    const std::vector<Projectile>& getProjectiles() const { return projectiles; }
    Hitbox getHitbox() const {
        return { realX + (1.0f - hbWidth) / 2.0f, realX + (1.0f + hbWidth) / 2.0f,
                 realY + 1.0f - hbHeight, realY + 1.0f };
    }

    void setRealPos(float nx, float ny);
    void takeDamage(int damage, int sourceX, const Map& gameMap);
    void setMoveIntent(int dir);
    void processJump(bool jumpPressed, bool jumpHeld, bool downHeld);
    void update(const Map& gameMap, float dt);
    AttackResult attack(std::vector<Enemy>& enemies, const Map& gameMap, bool downPressed);
    void setRunningMode(bool run) { isRunningMode = run; }
    void addExp(int amount);
    // 增加灵魂（打铁回蓝）
    void addMana(int amount) {
        mana += amount;
        if (mana > maxMana) mana = maxMana;
    }
    void castVengefulSpirit();
    void updateProjectiles(std::vector<Enemy>& enemies, const Map& gameMap, float dt);

    // 【修改原有的 Dash 接口】：让它带上体力判断
    bool tryDash();
    // 消耗灵魂（释放冲击波）
    bool consumeMana(int amount) {
        if (mana >= amount) {
            mana -= amount;
            return true;
        }
        return false;
    }
    bool getIsFocusing() const { return isFocusing; }
    float getHealFlashTimer() const { return healFlashTimer; }

    // 【终极魔法分配器】
    MagicAction processMagic(bool magicHeld, bool magicReleased,  float dt);
};
