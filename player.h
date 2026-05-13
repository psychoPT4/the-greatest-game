#pragma once
#include <string>
#include <vector>
#include <cmath>
#include "map.h"

enum MagicAction { MAGIC_NONE, CAST_SPELL, HEALED };

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

struct Shockwave {
    float x, y;
    float speed;
    int direction;
    float lifeTimer;
    float height;

    Hitbox getHitbox() const {
        return { x - 0.4f, x + 0.4f, y - height, y };
    }
};

struct AttackResult {
    bool pogoSuccess;
    bool hitSomething;
    AttackResult() : pogoSuccess(false), hitSomething(false) {}
};

class Enemy;

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

    int mana, maxMana, jumpCount, moveIntent;
    float maxRunSpeed, runAccel, groundFriction, airDrag;
    float gravity, jumpForce, maxFallSpeed;

    float ignorePlatformTimer;
    bool isDashing;
    float dashSpeed, dashDuration, dashTimer, dashCooldown, dashCooldownTimer;
    int dashDirection;
    bool isRunningMode;

    std::vector<Projectile> projectiles;

    float focusTimer = 0.0f;
    bool isFocusing = false;
    float healFlashTimer = 0.0f;

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
    int getFlickerTimer() const { return flickerTimer; }
    bool getIsGrounded() const { return isGrounded; }
    float getVelocityX() const { return velocityX; }
    float getVelocityY() const { return velocityY; }
    bool getIsDashing() const { return isDashing; }
    float getDashCooldownTimer() const { return dashCooldownTimer; }
    float getDashTimer() const { return dashTimer; }
    float getIgnorePlatformTimer() const { return ignorePlatformTimer; }
    bool getIsRunningMode() const { return isRunningMode; }
    float getFocusTimer() const { return focusTimer; }
    int getMana() const { return mana; }
    int getMaxMana() const { return maxMana; }
    float getStamina() const { return stamina; }
    float getMaxStamina() const { return maxStamina; }
    int getFacingDirection() const { return facingDirection; }
    const std::vector<Projectile>& getProjectiles() const { return projectiles; }
    bool getIsFocusing() const { return isFocusing; }
    float getHealFlashTimer() const { return healFlashTimer; }
    Hitbox getHitbox() const {
        return { realX + (1.0f - hbWidth) / 2.0f, realX + (1.0f + hbWidth) / 2.0f,
                 realY + 1.0f - hbHeight, realY + 1.0f };
    }

    void setRealPos(float nx, float ny);
    void takeDamage(int damage, int sourceX, const Map& gameMap);
    void setMoveIntent(int dir);
    void processJump(bool jumpPressed, bool jumpHeld, bool downHeld);
    void update(const Map& gameMap, float dt);
    AttackResult attack(std::vector<Enemy>& enemies, const Map& gameMap, int attackType, int lockedDir);
    void setRunningMode(bool run) { isRunningMode = run; }

    void addMana(int amount) {
        mana += amount;
        if (mana > maxMana) mana = maxMana;
    }

    void updateProjectiles(std::vector<Enemy>& enemies, const Map& gameMap, float dt);
    bool tryDash();
    bool consumeMana(int amount) {
        if (mana >= amount) {
            mana -= amount;
            return true;
        }
        return false;
    }
    MagicAction processMagic(bool magicHeld, bool magicReleased, float dt);

    void restoreState(float rx, float ry, int _hp, int _maxHp, int _mana, int _maxMana,
                      float _stamina, float _maxStamina, int _facingDirection, int _jumpCount,
                      bool _isGrounded, float _dashCooldownTimer, int _flickerTimer,
                      bool _isDashing, float _dashTimer, float _ignorePlatformTimer,
                      bool _isRunningMode, float _focusTimer, bool _isFocusing,
                      float _healFlashTimer, bool _alive, float _velocityX, float _velocityY) {
        realX = rx; realY = ry; x = (int)std::round(realX); y = (int)std::round(realY);
        hp = _hp; maxHp = _maxHp; mana = _mana; maxMana = _maxMana;
        stamina = _stamina; maxStamina = _maxStamina;
        facingDirection = _facingDirection; jumpCount = _jumpCount;
        isGrounded = _isGrounded; dashCooldownTimer = _dashCooldownTimer;
        flickerTimer = _flickerTimer; isDashing = _isDashing; dashTimer = _dashTimer;
        ignorePlatformTimer = _ignorePlatformTimer; isRunningMode = _isRunningMode;
        focusTimer = _focusTimer; isFocusing = _isFocusing;
        healFlashTimer = _healFlashTimer; alive = _alive;
        velocityX = _velocityX; velocityY = _velocityY;
        projectiles.clear();
    }
};