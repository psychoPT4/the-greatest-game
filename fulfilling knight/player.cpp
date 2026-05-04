#pragma execution_character_set("utf-8")
#include "Player.h"
#include "Enemy.h"
#include <cmath>
#include <algorithm>

extern std::string combatLog;

Player::Player(int startX, int startY)
    : name("Knight"), x(startX), y(startY), realX((float)startX), realY((float)startY),
    hbWidth(0.5f), hbHeight(0.8f), hp(100), maxHp(100), baseDamage(25),
    isGrounded(false), velocityX(0.0f), velocityY(0.0f), facingDirection(1),
    alive(true), flickerTimer(0),
    level(1), currentExp(0), expToNextLevel(100), mana(100), jumpCount(0), moveIntent(0),
    maxRunSpeed(12.0f), runAccel(120.0f), groundFriction(160.0f), airDrag(80.0f),
    gravity(85.0f), jumpForce(-26.0f), maxFallSpeed(30.0f),
    isDashing(false), dashSpeed(40.0f), dashDuration(0.2f), dashTimer(0.0f),
    dashCooldown(0.6f), dashCooldownTimer(0.0f), dashDirection(1), isRunningMode(false) {
}

void Player::setRealPos(float nx, float ny) {
    realX = nx; realY = ny;
    x = (int)std::round(realX); y = (int)std::round(realY);
    velocityX = 0.0f; velocityY = 0.0f;
    flickerTimer = 30;
}

void Player::takeDamage(int damage, int sourceX, const Map& gameMap) {
    if (!alive || flickerTimer > 0) return;
    if (isDashing) { combatLog = "闪避！无敌帧生效！ "; return; }
    hp -= damage;
    flickerTimer = 50;

    int kDir = (x > sourceX) ? 1 : -1;
    float targetX = realX + kDir * 1.0f;
    if (gameMap.getTileAt((int)std::round(targetX), y) != TileType::Wall) {
        realX = targetX; x = (int)std::round(realX);
    }
    if (hp <= 0) { hp = 0; alive = false; combatLog = name + " 已倒下！ "; }
}

void Player::setMoveIntent(int dir) {
    moveIntent = dir;
    if (dir != 0) facingDirection = dir;
}

void Player::processJump(bool jumpPressed, bool jumpHeld) {
    if (jumpPressed) {
        if (isGrounded) { velocityY = jumpForce; isGrounded = false; jumpCount = 1; }
        else if (jumpCount < 2) { velocityY = jumpForce * 0.85f; jumpCount = 2; combatLog = "二段跳！ "; }
    }
    if (!jumpHeld && velocityY < 0) velocityY *= 0.5f;
}

void Player::startDash() {
    if (!isDashing && dashCooldownTimer <= 0.0f) {
        isDashing = true; dashTimer = dashDuration; dashCooldownTimer = dashCooldown;
        dashDirection = (facingDirection != 0) ? facingDirection : 1;
        velocityY = 0.0f;
    }
}

void Player::update(const Map& gameMap, float dt) {
    if (!alive) return;
    if (dashCooldownTimer > 0.0f) dashCooldownTimer -= dt;
    if (flickerTimer > 0) flickerTimer--;

    // 1. 物理速度计算
    if (isDashing) {
        dashTimer -= dt;
        if (dashTimer <= 0.0f) { isDashing = false; velocityX = 0.0f; }
        else { velocityX = dashDirection * dashSpeed; velocityY = 0.0f; }
    }
    else {
        float currentFriction = isGrounded ? groundFriction : airDrag;
        float currentMaxSpeed = isRunningMode ? maxRunSpeed : (maxRunSpeed * 0.6f);
        if (moveIntent != 0) {
            velocityX += runAccel * moveIntent * dt;
            if (velocityX > currentMaxSpeed) velocityX = currentMaxSpeed;
            if (velocityX < -currentMaxSpeed) velocityX = -currentMaxSpeed;
        }
        else {
            if (velocityX > 0) { velocityX -= currentFriction * dt; if (velocityX < 0) velocityX = 0; }
            else if (velocityX < 0) { velocityX += currentFriction * dt; if (velocityX > 0) velocityX = 0; }
        }
        // 【核心修复一】：永远应用重力，无论状态！
        velocityY += gravity * dt;
        if (velocityY > maxFallSpeed) velocityY = maxFallSpeed;
    }

    // 2. X轴碰撞判定
    float nextRealX = realX + velocityX * dt;
    float boxLeft = nextRealX + (1.0f - hbWidth) / 2.0f;
    float boxRight = nextRealX + (1.0f + hbWidth) / 2.0f;
    float boxTop = realY + 1.0f - hbHeight + 0.1f;
    float boxBottom = realY + 0.9f;

    if (velocityX > 0) {
        if (gameMap.getTileAt((int)boxRight, (int)boxTop) == TileType::Wall || gameMap.getTileAt((int)boxRight, (int)boxBottom) == TileType::Wall) {
            nextRealX = std::floor(boxRight) - (1.0f + hbWidth) / 2.0f - 0.01f; velocityX = 0.0f;
        }
    }
    else if (velocityX < 0) {
        if (gameMap.getTileAt((int)boxLeft, (int)boxTop) == TileType::Wall || gameMap.getTileAt((int)boxLeft, (int)boxBottom) == TileType::Wall) {
            nextRealX = std::floor(boxLeft) + 1.0f - (1.0f - hbWidth) / 2.0f + 0.01f; velocityX = 0.0f;
        }
    }
    realX = nextRealX;

    // 3. Y轴碰撞与精确接地判定
    float nextRealY = realY + velocityY * dt;
    Hitbox box = getHitbox();
    float inset = 0.05f;
    float pLeft = box.left + inset;
    float pRight = box.right - inset;

    isGrounded = false; // 每帧默认清空，由下面物理严格证明

    // 【核心修复二】：检测向下碰撞时，必须包含 velocityY >= 0 (即静止站立状态也要测脚底！)
    if (velocityY >= 0.0f) {
        int currentBottomTile = (int)(realY + 1.0f);
        int nextBottomTile = (int)(nextRealY + 1.0f);
        bool hitGround = false; int groundTileY = nextBottomTile;

        for (int ty = currentBottomTile; ty <= nextBottomTile; ++ty) {
            TileType leftFoot = gameMap.getTileAt((int)pLeft, ty);
            TileType rightFoot = gameMap.getTileAt((int)pRight, ty);
            if ((leftFoot == TileType::Wall || leftFoot == TileType::Platform) || (rightFoot == TileType::Wall || rightFoot == TileType::Platform)) {
                hitGround = true; groundTileY = ty; break;
            }
        }
        if (hitGround) {
            nextRealY = (float)groundTileY - 1.0f;
            velocityY = 0.0f; isGrounded = true; jumpCount = 0;
        }
    }
    else if (velocityY < 0) {
        int currentTopTile = (int)(realY + 1.0f - hbHeight);
        int nextTopTile = (int)(nextRealY + 1.0f - hbHeight);
        bool hitCeil = false; int ceilTileY = nextTopTile;

        for (int ty = currentTopTile; ty >= nextTopTile; --ty) {
            TileType leftHead = gameMap.getTileAt((int)pLeft, ty);
            TileType rightHead = gameMap.getTileAt((int)pRight, ty);
            if (leftHead == TileType::Wall || rightHead == TileType::Wall) { hitCeil = true; ceilTileY = ty; break; }
        }
        if (hitCeil) { nextRealY = (float)ceilTileY + 1.0f - (1.0f - hbHeight) + 0.01f; velocityY = 0.0f; }
    }
    realY = nextRealY; x = (int)std::round(realX); y = (int)std::round(realY);
}

AttackResult Player::attack(std::vector<Enemy>& enemies, const Map& gameMap, bool downPressed) {
    AttackResult res;
    Hitbox myBox = getHitbox();

    if (!isGrounded && downPressed) {
        Hitbox pogoBox = { myBox.left, myBox.right, myBox.bottom, myBox.bottom + 1.5f };
        for (auto& target : enemies) {
            if (!target.isAlive()) continue;
            if (pogoBox.intersects(target.getHitbox())) {
                target.takeDamage(baseDamage, x, gameMap);
                res.pogoSuccess = true; res.hitSomething = true;
                combatLog = "【下劈弹刀！】命中目标！ ";
                if (!target.isAlive()) res.totalXp += target.getExpReward();
            }
        }
        TileType leftFoot = gameMap.getTileAt((int)pogoBox.left, (int)pogoBox.bottom);
        TileType rightFoot = gameMap.getTileAt((int)pogoBox.right, (int)pogoBox.bottom);
        if (leftFoot == TileType::SpikeUp || rightFoot == TileType::SpikeUp) { res.pogoSuccess = true; combatLog = "【下劈弹刀！】借助地刺弹起！ "; }
        if (res.pogoSuccess) { velocityY = jumpForce * 0.9f; jumpCount = 1; }
    }
    else {
        Hitbox slashBox;
        if (facingDirection == 1) slashBox = { myBox.right, myBox.right + 1.5f, myBox.top, myBox.bottom };
        else slashBox = { myBox.left - 1.5f, myBox.left, myBox.top, myBox.bottom };

        bool hitSomething = false;
        for (auto& target : enemies) {
            if (!target.isAlive()) continue;
            if (slashBox.intersects(target.getHitbox())) {
                target.takeDamage(baseDamage, x, gameMap);
                res.hitSomething = true; hitSomething = true; combatLog = "发起平砍！ ";
                if (!target.isAlive()) res.totalXp += target.getExpReward();
            }
        }
        if (!hitSomething) combatLog = "挥空了... ";
    }
    return res;
}

void Player::addExp(int amount) {
    currentExp += amount;
    while (currentExp >= expToNextLevel) {
        level++; currentExp -= expToNextLevel; expToNextLevel = (int)(expToNextLevel * 1.5);
        maxHp += 20; hp = maxHp; baseDamage += 5;
        combatLog = "★★★ 升级！当前等级:  " + std::to_string(level) + " ★★★ ";
    }
}