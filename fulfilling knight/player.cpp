#pragma execution_character_set("utf-8")
#include "Player.h"
#include "Enemy.h"
#include <cmath>
#include <algorithm>

extern std::string combatLog;

Player::Player(int startX, int startY)
    : name("Knight"), x(startX), y(startY), realX((float)startX), realY((float)startY),
    hbWidth(0.5f), hbHeight(0.8f), hp(100), maxHp(100), baseDamage(10), // 伤害锁定 10
    isGrounded(false), velocityX(0.0f), velocityY(0.0f), facingDirection(1),
    alive(true), flickerTimer(0),
    mana(0), maxMana(99), jumpCount(0), moveIntent(0),
    maxRunSpeed(12.0f), runAccel(120.0f), groundFriction(160.0f), airDrag(80.0f),
    gravity(85.0f), jumpForce(-26.0f), maxFallSpeed(30.0f),
    isDashing(false), dashSpeed(40.0f), dashDuration(0.2f), dashTimer(0.0f),
    dashCooldown(0.6f), dashCooldownTimer(0.0f), ignorePlatformTimer(0.0f), dashDirection(1), isRunningMode(false) {
    stamina = 100.0f;
    maxStamina = 100.0f;
}

void Player::setRealPos(float nx, float ny) {
    realX = nx; realY = ny;
    x = (int)std::round(realX); y = (int)std::round(realY);
    velocityX = 0.0f; velocityY = 0.0f;
    flickerTimer = 30;
}

void Player::takeDamage(int damage, int sourceX, const Map& gameMap) {
    if (!alive || flickerTimer > 0) return;

    if (isDashing) {
        isDashing = false;
        combatLog = "冲刺被打断！ ";
    }

    hp -= damage;
    flickerTimer = 50;

    isGrounded = false;
    velocityY = -8.0f;

    int kDir = (x > sourceX) ? 1 : -1;
    float targetX = realX + kDir * 1.0f;

    if (gameMap.getTileAt((int)std::round(targetX), y) != TileType::Wall) {
        realX = targetX;
        x = (int)std::round(realX);
    }

    if (hp <= 0) {
        hp = 0;
        alive = false;
        combatLog = name + " 已倒下！ ";
    }
}

void Player::setMoveIntent(int dir) {
    moveIntent = dir;
    if (dir != 0) facingDirection = dir;
}

void Player::processJump(bool jumpPressed, bool jumpHeld, bool downHeld) {
    if (jumpPressed && (isGrounded || jumpCount < 2)) {
        if (stamina >= 15.0f) {
            stamina -= 15.0f;
            velocityY = jumpForce;
            jumpCount++;
            isGrounded = false;
        }
    }
    if (jumpPressed) {
        if (downHeld && isGrounded) {
            ignorePlatformTimer = 0.2f;
            isGrounded = false;
            velocityY = 10.0f;
            return;
        }

        if (isGrounded) { velocityY = jumpForce; isGrounded = false; jumpCount = 1; }
        else if (jumpCount < 2) { velocityY = jumpForce * 0.85f; jumpCount = 2; combatLog = "二段跳！ "; }
    }
    if (!jumpHeld && velocityY < 0) velocityY *= 0.5f;
}

bool Player::tryDash() {
    if (stamina >= 30.0f && !isDashing && dashCooldownTimer <= 0.0f) {
        stamina -= 30.0f;
        isDashing = true;
        dashTimer = dashDuration;
        dashCooldownTimer = dashCooldown;
        dashDirection = facingDirection;
        return true;
    }
    return false;
}

void Player::update(const Map& gameMap, float dt) {
    if (!alive) return;
    if (dashCooldownTimer > 0.0f) dashCooldownTimer -= dt;
    if (ignorePlatformTimer > 0.0f) ignorePlatformTimer -= dt;
    if (flickerTimer > 0) flickerTimer--;

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
        velocityY += gravity * dt;
        if (velocityY > maxFallSpeed) velocityY = maxFallSpeed;
    }

    if (isDashing) {
    }
    else if (isRunningMode && isGrounded && (IsKeyDown(KEY_A) || IsKeyDown(KEY_D))) {
        stamina -= 15.0f * dt;
        if (stamina < 0.0f) stamina = 0.0f;
    }
    else {
        stamina += 25.0f * dt;
        if (stamina > maxStamina) stamina = maxStamina;
    }

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

    float nextRealY = realY + velocityY * dt;
    Hitbox box = getHitbox();
    float inset = 0.05f;
    float pLeft = box.left + inset;
    float pRight = box.right - inset;

    isGrounded = false;

    if (velocityY >= 0.0f) {
        int currentBottomTile = (int)(realY + 1.0f);
        int nextBottomTile = (int)(nextRealY + 1.0f);
        bool hitGround = false; int groundTileY = nextBottomTile;

        for (int ty = currentBottomTile; ty <= nextBottomTile; ++ty) {
            TileType leftFoot = gameMap.getTileAt((int)pLeft, ty);
            TileType rightFoot = gameMap.getTileAt((int)pRight, ty);

            if (leftFoot == TileType::Wall || rightFoot == TileType::Wall) {
                hitGround = true; groundTileY = ty; break;
            }
            if (ignorePlatformTimer <= 0.0f) {
                if (leftFoot == TileType::Platform || rightFoot == TileType::Platform) {
                    hitGround = true; groundTileY = ty; break;
                }
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

AttackResult Player::attack(std::vector<Enemy>& enemies, const Map& gameMap, int attackType, int lockedDir) {
    AttackResult res;
    Hitbox myBox = getHitbox();
    Hitbox strikeBox;

    // 1. 生成锁定方向的碰撞盒
    if (attackType == 1 && !isGrounded) {
        // 【下劈】
        strikeBox = { myBox.left, myBox.right, myBox.bottom, myBox.bottom + 1.5f };
    }
    else if (attackType == 2) {
        // 🌟 【上挑】：在头顶生成判定框
        strikeBox = { myBox.left - 0.5f, myBox.right + 0.5f, myBox.top - 1.5f, myBox.top };
    }
    else {
        // 🌟 【平砍】：严格使用 lockedDir，拒绝转身滑步！
        if (lockedDir == 1) strikeBox = { myBox.right, myBox.right + 1.5f, myBox.top, myBox.bottom };
        else strikeBox = { myBox.left - 1.5f, myBox.left, myBox.top, myBox.bottom };
    }

    // 2. 遍历敌人，造成有效伤害
    for (auto& target : enemies) {
        if (!target.isAlive()) continue;
        if (strikeBox.intersects(target.getHitbox())) {
            // 🌟 只有 takeDamage 返回 true（真的扣血了），才判定为命中！
            if (target.takeDamage(baseDamage, x, gameMap)) {
                res.hitSomething = true;
                if (attackType == 1) res.pogoSuccess = true;
                addMana(11);
                combatLog = "【命中目标！】 ";
            }
        }
    }

    // 下劈弹起地形的逻辑保留
    if (attackType == 1 && !res.pogoSuccess) {
        TileType leftFoot = gameMap.getTileAt((int)strikeBox.left, (int)strikeBox.bottom);
        TileType rightFoot = gameMap.getTileAt((int)strikeBox.right, (int)strikeBox.bottom);
        if (leftFoot == TileType::SpikeUp || rightFoot == TileType::SpikeUp) {
            res.pogoSuccess = true;
            combatLog = "【下劈弹刀！】借助地刺弹起 ";
        }
    }

    if (res.pogoSuccess) { velocityY = jumpForce * 0.9f; jumpCount = 1; }

    return res;
}

MagicAction Player::processMagic(bool magicHeld, bool magicReleased, float dt) {
    MagicAction action = MAGIC_NONE;
    if (healFlashTimer > 0.0f) healFlashTimer -= dt;

    if (magicHeld) {
        focusTimer += dt; // 只要按住，计时器就永远在走

        // 凝聚铁律：必须站在地上，没有移动，没有冲刺，且没有在挨打
        if (isGrounded && velocityX == 0.0f && !isDashing && flickerTimer <= 0) {
            isFocusing = true;
            if (focusTimer >= 1.0f) {
                if (hp < maxHp && consumeMana(33)) {
                    hp += 25; // 恢复四分之一血量 (一个面具)
                    if (hp > maxHp) hp = maxHp;
                    healFlashTimer = 0.3f;
                    action = HEALED;
                }
                focusTimer = 0.0f;
            }
        }
        else {
            isFocusing = false; // 移动打断白光特效，但计时器不归零
        }
    }
    else if (magicReleased) {
        // 解放空中和移动施法
        if (focusTimer > 0.0f && focusTimer < 0.35f) {
            if (consumeMana(33)) {
                Projectile p;
                p.x = realX;
                p.y = realY + 0.3f;
                p.facingDir = facingDirection;
                p.speed = 18.0f;
                p.lifeTimer = 0.6f;
                p.active = true;
                projectiles.push_back(p);
                action = CAST_SPELL;
            }
        }
        isFocusing = false;
        focusTimer = 0.0f;
    }
    return action;
}

void Player::updateProjectiles(std::vector<Enemy>& enemies, const Map& gameMap, float dt) {
    for (auto it = projectiles.begin(); it != projectiles.end(); ) {
        it->x += (it->speed * it->facingDir) * dt;
        it->lifeTimer -= dt;

        for (auto& target : enemies) {
            if (target.isAlive() && it->getHitbox().intersects(target.getHitbox())) {
                if (!target.isFlickering()) {
                    target.takeDamage(40, it->x, gameMap);
                }
            }
        }

        TileType frontTile = gameMap.getTileAt(it->x + (it->facingDir == 1 ? 1.0f : 0.0f), it->y);
        if (frontTile == TileType::Wall || it->lifeTimer <= 0) {
            it = projectiles.erase(it);
        }
        else {
            ++it;
        }
    }
}