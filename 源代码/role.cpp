#include "../头文件/Role.h"
#include <iostream>
#include <cmath>

extern std::string combatLog; 

Role::Role(std::string n, int startX, int startY, int maxHp, int dmg)
    : name(n), x(startX), y(startY), hp(maxHp), maxHp(maxHp), baseDamage(dmg), 
      alive(true), flickerTimer(0), realX((float)startX), realY((float)startY) {}

void Role::takeDamage(int amount, int attackerX, const Map& gameMap) {
    if (!alive) return;
    hp -= amount;
    flickerTimer = 5; 
    int kDir = (x > attackerX) ? 1 : -1;
    if (gameMap.getTileAt(x + kDir, y) != TileType::Wall) {
        x += kDir; realX = (float)x;
    }
    if (hp <= 0) { hp = 0; alive = false; }
}

// ---------------------------------------------------------
// 骑士 (Player) - 独立坐标轴判定版
// ---------------------------------------------------------
Player::Player(int startX, int startY)
    : Role("骑士", startX, startY, 100, 25), level(1), currentExp(0), expToNextLevel(100), 
      mana(100), facingDirection(1), jumpCount(0), 
      velocityY(0.0f), 
      gravity(0.10f),   // 降低重力，让下落更平滑
      jumpForce(-0.8f), // 增强初速度，跳跃更有力
      isGrounded(false) {}

void Player::move(int dx, int dy, const Map& gameMap) {
    if (!alive) return;
    if (dx != 0) facingDirection = dx;

    // 【独立 X 轴判定】：不考虑 y，只要侧面没墙就能走
    int targetX = x + dx;
    if (gameMap.getTileAt(targetX, y) != TileType::Wall) {
        x = targetX;
        realX = (float)x;
    }
}

void Player::jump() {
    // 【高响应】：直接修改速度，不等待逻辑帧
    if (isGrounded) { 
        velocityY = jumpForce; 
        isGrounded = false; 
        jumpCount = 1; 
    } else if (jumpCount < 2) { 
        velocityY = jumpForce * 0.85f; 
        jumpCount = 2; 
        combatLog = "二段跳！";
    }
}

void Player::update(const Map& gameMap) {
    if (!alive) return;
    if (flickerTimer > 0) flickerTimer--;

    // 1. 落地预检
    TileType under = gameMap.getTileAt(x, y + 1);
    if (isGrounded && under != TileType::Wall && under != TileType::Platform) {
        isGrounded = false;
    }

    // 2. 应用重力
    if (!isGrounded) {
        velocityY += gravity;
        if (velocityY > 1.5f) velocityY = 1.5f; 
    }
    
    // 3. 【独立 Y 轴判定】：使用路径扫描防止钻地
    realY += velocityY;
    int nextY = (int)round(realY);

    if (velocityY > 0) { // 下落
        bool hit = false;
        for (int i = y + 1; i <= nextY; ++i) {
            TileType t = gameMap.getTileAt(x, i);
            if ((t == TileType::Wall || t == TileType::Platform) && y < i) {
                y = i - 1; 
                realY = (float)y; 
                velocityY = 0.0f; 
                isGrounded = true; 
                jumpCount = 0; 
                hit = true;
                break;
            }
        }
        if (!hit) y = (nextY < 15) ? nextY : 14;
    } 
    else if (velocityY < 0) { // 上升
        TileType head = gameMap.getTileAt(x, nextY);
        if (head == TileType::Wall) {
            velocityY = 0.1f; realY = (float)y; 
        } else {
            y = (nextY >= 0) ? nextY : 0;
        }
    }
}

void Player::attack(Role& target, const Map& gameMap) {
    if (!target.isAlive()) return;
    if (abs(target.getX() - x) <= 2 && (target.getX() - x) * facingDirection >= 0) {
        target.takeDamage(baseDamage, x, gameMap);
        combatLog = "你发起了攻击！";
    }
}

// ---------------------------------------------------------
// 爬虫 (Enemy) - 物理统一版
// ---------------------------------------------------------
Enemy::Enemy(int startX, int startY, int expGive)
    : Role("爬虫", startX, startY, 40, 12), moveDirection(1), expReward(expGive), attackCooldown(0) {}

void Enemy::update(const Map& gameMap, Player& player) {
    if (!alive) return;
    if (flickerTimer > 0) flickerTimer--;
    if (attackCooldown > 0) attackCooldown--;

    // 怪物重力检测
    if (gameMap.getTileAt(x, y + 1) == TileType::Empty) {
        y++; realY = (float)y;
    } else {
        static int f = 0;
        if (++f % 10 == 0) {
            int nx = x + moveDirection;
            if (gameMap.getTileAt(nx, y) == TileType::Wall || gameMap.getTileAt(nx, y + 1) == TileType::Empty) {
                moveDirection *= -1;
            } else { x = nx; }
        }
    }

    if (abs(player.getX() - x) <= 1 && abs(player.getY() - y) <= 1 && attackCooldown <= 0) {
        player.takeDamage(baseDamage, x, gameMap);
        attackCooldown = 20;
        combatLog = "你被碰撞了！HP -12";
    }
}