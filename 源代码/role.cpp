#include "../头文件/Role.h"
#include <iostream>
#include <cmath> // 用于 round 等数学运算

// ==========================================
// 1. 基类 Role 实现
// ==========================================
// 构造函数：严格对齐头文件中的 7 个参数
Role::Role(std::string n, int startX, int startY, int maxHp, int dmg, float rx, float ry)
    : name(n), x(startX), y(startY), hp(maxHp), maxHp(maxHp), baseDamage(dmg), realX(rx), realY(ry) {}

void Role::takeDamage(int amount) {
    hp -= amount;
    if (hp < 0) hp = 0;
}

// ==========================================
// 2. 派生类 Player 实现
// ==========================================
// 骑士构造：初始化时将 int 坐标同步给影子坐标 float
Player::Player(int startX, int startY)
    : Role("骑士", startX, startY, 100, 15, (float)startX, (float)startY),
      level(1), currentExp(0), expToNextLevel(100),
      specialDamage(30), mana(100),
      velocityY(0.0f), gravity(0.15f), jumpForce(-1.0f), isGrounded(false) {}

void Player::move(int dx, int dy, const Map& gameMap) {
    int targetX = x + dx;
    int targetY = y + dy;
    TileType targetTile = gameMap.getTileAt(targetX, targetY);

    // 水平碰撞检测
    if (targetTile == TileType::Empty || targetTile == TileType::Platform) {
        x = targetX;
        realX = (float)x; // 同步影子坐标
    }
}

void Player::jump() {
    if (isGrounded) {
        velocityY = jumpForce;
        isGrounded = false;
    }
}

void Player::update(const Map& gameMap) {
    // A. 悬崖检测：站在边缘时如果脚下变空，立即转为坠落状态
    if (isGrounded) {
        if (gameMap.getTileAt(x, y + 1) == TileType::Empty) {
            isGrounded = false;
        }
    }

    // B. 重力计算：仅在空中时累加
    if (!isGrounded) {
        velocityY += gravity;
        if (velocityY > 2.0f) velocityY = 2.0f; // 终端速度限制，防止穿模
    }

    // C. 核心修正：使用影子坐标进行高精度累加，消灭顶点停顿感
    realY += velocityY;
    
    // 渲染坐标 y 采用影子坐标的四舍五入，确保平滑
    int nextY = static_cast<int>(round(realY));

    // D. 垂直碰撞检测
if (velocityY > 0) { // 下落中
    TileType landTile = gameMap.getTileAt(x, nextY);
    
    // 【核心修复点】：增加 (y < nextY) 判断
    // 只有当你现在的高度(y)比目标格(nextY)高，才能算是“从上往下掉在台子上”
    if ((landTile == TileType::Wall || landTile == TileType::Platform) && y < nextY) {
        y = nextY - 1;       // 站在方块上方
        realY = (float)y;    
        velocityY = 0.0f;
        isGrounded = true;
    } 
    // 如果你在平台下方或者和平台等高，下落时不能踩上去，只能继续掉落（或者被判定为撞墙）
    else {
        y = nextY;
        isGrounded = false;
    }
} 
    else if (velocityY < 0) { // 跳跃中
        TileType hitTile = gameMap.getTileAt(x, nextY);
        if (hitTile == TileType::Wall) {
            y = nextY + 1;       // 撞到天花板弹回
            realY = (float)y;
            velocityY = 0.0f;
        } else {
            y = nextY;
            isGrounded = false;
        }
    }
}

// 战斗与成长逻辑
void Player::gainExp(int amount) {
    currentExp += amount;
    if (currentExp >= expToNextLevel) levelUp();
}

void Player::levelUp() {
    level++;
    currentExp = 0;
    expToNextLevel *= 1.5;
    maxHp += 20;
    hp = maxHp;
    baseDamage += 5;
}

void Player::normalAttack(Role& target) {
    target.takeDamage(baseDamage);
}

void Player::specialAttack(Role& target) {
    if (mana >= 30) {
        mana -= 30;
        target.takeDamage(specialDamage);
    }
}

// ==========================================
// 3. 派生类 Enemy 实现
// ==========================================
Enemy::Enemy(int startX, int startY, int expGive)
    : Role("爬虫", startX, startY, 30, 8, (float)startX, (float)startY),
      moveDirection(1), expReward(expGive) {}

void Enemy::update(const Map& gameMap) {
    // 基础 AI：每隔几帧移动一次，防止在 20FPS 下速度过快
    static int frame = 0;
    if (++frame % 8 == 0) {
        int nextX = x + moveDirection;
        // 碰到墙壁或悬崖边缘就转身
        if (gameMap.getTileAt(nextX, y) == TileType::Wall || 
            gameMap.getTileAt(nextX, y + 1) == TileType::Empty) {
            moveDirection *= -1;
        } else {
            x = nextX;
            realX = (float)x;
        }
    }
}