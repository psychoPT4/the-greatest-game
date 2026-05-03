#include "Role.h"
#include <iostream>
#include <cmath>
#include <algorithm> 

extern std::string combatLog;

Role::Role(std::string n, int startX, int startY, int maxHealth, int dmg)
    : name(n), x(startX), y(startY), realX((float)startX), realY((float)startY),
    hbWidth(1.0f), hbHeight(1.0f),
    hp(maxHealth), maxHp(maxHealth), baseDamage(dmg),
    isGrounded(false), velocityX(0.0f), velocityY(0.0f),
    facingDirection(1), alive(true), flickerTimer(0) {
}

void Role::takeDamage(int amount, int attackerX, const Map& gameMap) {
    if (!alive || flickerTimer > 0) return;
    hp -= amount;
    flickerTimer = (name == "骑士") ? 50 : 5;

    int kDir = (x > attackerX) ? 1 : -1;
    float targetX = realX + kDir * 2.0f;
    if (gameMap.getTileAt((int)round(targetX), y) != TileType::Wall) {
        realX = targetX;
        x = (int)round(realX);
    }

    if (hp <= 0) {
        hp = 0; alive = false;
        combatLog = name + " 已倒下！";
    }
}

// ==========================================
// Player
// ==========================================
Player::Player(int startX, int startY)
    : Role("骑士", startX, startY, 100, 25),
    level(1), currentExp(0), expToNextLevel(100), mana(100), jumpCount(0), moveIntent(0),
    maxRunSpeed(15.0f), runAccel(150.0f), groundFriction(200.0f), airDrag(80.0f),
    gravity(85.0f), jumpForce(-26.0f), maxFallSpeed(30.0f),
    isDashing(false), dashSpeed(40.0f), dashDuration(0.2f), dashTimer(0.0f),
    dashCooldown(0.6f), dashCooldownTimer(0.0f), dashDirection(1)
{
    hbWidth = 0.5f; hbHeight = 0.8f;
}

void Player::setMoveIntent(int dir) {
    moveIntent = dir;
    if (dir != 0) facingDirection = dir;
}

void Player::processJump(bool jumpPressed, bool jumpHeld) {
    if (jumpPressed) {
        if (isGrounded) {
            velocityY = jumpForce; isGrounded = false; jumpCount = 1;
        }
        else if (jumpCount < 2) {
            velocityY = jumpForce * 0.85f; jumpCount = 2; combatLog = "二段跳！";
        }
    }
    if (!jumpHeld && velocityY < 0) {
        velocityY *= 0.5f;
    }
}

void Player::startDash() {
    // 修复：补全了确实的大括号，并修正了 facingDirection 的命名
    if (!isDashing && dashCooldownTimer <= 0.0f) {
        isDashing = true;
        dashTimer = dashDuration;
        dashCooldownTimer = dashCooldown;
        dashDirection = (facingDirection != 0) ? facingDirection : 1;
        velocityY = 0.0f;
    }
}

void Player::update(const Map& gameMap, float dt) {
    if (!alive) return;

    if (dashCooldownTimer > 0.0f) {
        dashCooldownTimer -= dt;
    }
    if (flickerTimer > 0) flickerTimer--;

    TileType under = gameMap.getTileAt(x, y + 1);
    if (isGrounded && under != TileType::Wall && under != TileType::Platform) isGrounded = false;

    // --- 核心物理层分离 ---
    if (isDashing) {
        dashTimer -= dt;
        if (dashTimer <= 0.0f) {
            isDashing = false;
            velocityX = 0.0f;
        }
        else {
            velocityX = dashDirection * dashSpeed;
            velocityY = 0.0f; // Dash 期间强行剥夺重力
        }
    }
    else {
        // 常规物理系统：加速、摩擦力、空气阻力、重力
        float currentFriction = isGrounded ? groundFriction : airDrag;
        if (moveIntent != 0) {
            velocityX += runAccel * moveIntent * dt;
            velocityX = std::clamp(velocityX, -maxRunSpeed, maxRunSpeed);
        }
        else {
            if (velocityX > 0) { velocityX -= currentFriction * dt; if (velocityX < 0) velocityX = 0; }
            else if (velocityX < 0) { velocityX += currentFriction * dt; if (velocityX > 0) velocityX = 0; }
        }

        if (!isGrounded) {
            velocityY += gravity * dt;
            if (velocityY > maxFallSpeed) velocityY = maxFallSpeed;
        }
    }

    // --- AABB 碰撞检测 (冲刺和走路共用这套碰撞，防止穿墙) ---
    float nextRealX = realX + velocityX * dt;
    float boxLeft = nextRealX + (1.0f - hbWidth) / 2.0f;
    float boxRight = nextRealX + (1.0f + hbWidth) / 2.0f;
    float boxTop = realY + 1.0f - hbHeight + 0.1f;
    float boxBottom = realY + 0.9f;

    if (velocityX > 0) {
        if (gameMap.getTileAt((int)boxRight, (int)boxTop) == TileType::Wall || gameMap.getTileAt((int)boxRight, (int)boxBottom) == TileType::Wall) {
            nextRealX = floor(boxRight) - (1.0f + hbWidth) / 2.0f - 0.01f;
            velocityX = 0.0f;
        }
    }
    else if (velocityX < 0) {
        if (gameMap.getTileAt((int)boxLeft, (int)boxTop) == TileType::Wall || gameMap.getTileAt((int)boxLeft, (int)boxBottom) == TileType::Wall) {
            nextRealX = floor(boxLeft) + 1.0f - (1.0f - hbWidth) / 2.0f + 0.01f;
            velocityX = 0.0f;
        }
    }
    realX = nextRealX;

    float nextRealY = realY + velocityY * dt;
    Hitbox box = getHitbox();
    float inset = 0.05f;
    float pLeft = box.left + inset;
    float pRight = box.right - inset;
    isGrounded = false;

    if (velocityY > 0) {
        int currentBottomTile = (int)(realY + 1.0f);
        int nextBottomTile = (int)(nextRealY + 1.0f);
        bool hitGround = false;
        int groundTileY = nextBottomTile;

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
    realY = nextRealY;
    x = (int)round(realX); y = (int)round(realY);
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
                res.pogoSuccess = true;
                combatLog = "【下劈弹刀！】命中目标！";
                if (!target.isAlive()) res.totalXp += target.getExpReward();
            }
        }
        TileType leftFoot = gameMap.getTileAt((int)pogoBox.left, (int)pogoBox.bottom);
        TileType rightFoot = gameMap.getTileAt((int)pogoBox.right, (int)pogoBox.bottom);
        if (leftFoot == TileType::SpikeUp || rightFoot == TileType::SpikeUp) {
            res.pogoSuccess = true; combatLog = "【下劈弹刀！】借助地刺弹起！";
        }
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
                hitSomething = true; combatLog = "发起平砍！";
                if (!target.isAlive()) res.totalXp += target.getExpReward();
            }
        }
        if (!hitSomething) combatLog = "挥空了...";
    }
    return res;
}

void Player::addExp(int amount) {
    currentExp += amount;
    while (currentExp >= expToNextLevel) {
        level++;
        currentExp -= expToNextLevel;
        expToNextLevel = (int)(expToNextLevel * 1.5);
        maxHp += 20; hp = maxHp;
        baseDamage += 5;
        combatLog = "★★★ 升级！当前等级: " + std::to_string(level) + " ★★★";
    }
}

// ==========================================
// Enemy
// ==========================================
Enemy::Enemy(int startX, int startY, int expGive)
    : Role("爬虫", startX, startY, 40, 12),
    moveDirection(1), expReward(expGive), spawnX(startX), attackCooldown(0), moveTimer(0.0f) {
    hbWidth = 0.8f;
    hbHeight = 0.5f;
}

void Enemy::update(const Map& gameMap, Player& player, float dt) {
    if (!alive) return;
    if (flickerTimer > 0) flickerTimer--;
    if (attackCooldown > 0) attackCooldown--;

    if (gameMap.getTileAt(x, y + 1) == TileType::Empty || gameMap.getTileAt(x, y + 1) == TileType::Void) {
        y++; realY = (float)y;
    }
    else {
        float distToPlayerX = abs(player.getRealX() - realX);
        float distToPlayerY = abs(player.getRealY() - realY);

        bool isChasing = (distToPlayerX <= 4.0f && distToPlayerY <= 1.5f);
        int intentDir = 0;

        if (isChasing) {
            intentDir = (player.getRealX() > realX) ? 1 : -1;
            moveDirection = intentDir;
        }
        else {
            if (moveDirection == 1 && realX >= spawnX + 3) moveDirection = -1;
            else if (moveDirection == -1 && realX <= spawnX - 3) moveDirection = 1;
            intentDir = moveDirection;
        }

        moveTimer += dt;
        if (moveTimer >= 0.15f) {
            moveTimer = 0.0f;
            int nx = x + intentDir;

            TileType nextFloor = gameMap.getTileAt(nx, y + 1);
            TileType nextWall = gameMap.getTileAt(nx, y);

            bool isFloorSafe = (nextFloor == TileType::Wall || nextFloor == TileType::Platform);
            bool isWallSafe = (nextWall == TileType::Empty);

            if (!isFloorSafe || !isWallSafe) {
                if (!isChasing) {
                    moveDirection *= -1;
                }
            }
            else {
                x = nx; realX = (float)x;
            }
        }
    }

    if (getHitbox().intersects(player.getHitbox()) && attackCooldown <= 0) {
        player.takeDamage(baseDamage, x, gameMap);
        attackCooldown = 30;
    }
}

void Enemy::takeDamage(int damage, int sourceX, const Map& gameMap) {
    if (flickerTimer > 0) return;
    hp -= damage;
    if (hp <= 0) {
        alive = false; combatLog = "击杀目标！";
    }
    else {
        flickerTimer = 5;
        int knockbackDir = (x > sourceX) ? 1 : -1;
        TileType backWall = gameMap.getTileAt(x + knockbackDir, y);
        if (backWall != TileType::Wall) {
            x += knockbackDir; realX = (float)x;
        }
    }
}