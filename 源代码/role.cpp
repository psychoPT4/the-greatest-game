#include "../头文件/Role.h" // 注意：根据你的实际文件夹结构，可能需要改成 "Role.h"
#include <iostream>

// ==========================================
// 基类 Role 实现
// ==========================================
Role::Role(std::string n, int startX, int startY, int maxHp, int dmg) 
    : name(n), x(startX), y(startY), hp(maxHp), maxHp(maxHp), baseDamage(dmg) {}

void Role::takeDamage(int amount) {
    if (isDead()) return; 
    
    hp -= amount;
    if (hp < 0) hp = 0; 
    
    std::cout << "[战斗] " << name << " 受到了 " << amount << " 点伤害！(剩余HP: " << hp << "/" << maxHp << ")" << std::endl;
    
    if (isDead()) {
        std::cout << ">>> " << name << " 被击败了！ <<<" << std::endl;
    }
}

// ==========================================
// 派生类 Player 实现
// ==========================================
Player::Player(int startX, int startY) 
    : Role("骑士", startX, startY, 50, 10), 
      level(1), currentExp(0), expToNextLevel(100), 
      specialDamage(25), mana(100),
      velocityY(0.0f), gravity(0.5f), jumpForce(-2.0f), isGrounded(false) {}

void Player::move(int dx, int dy, const Map& gameMap) {
    int targetX = x + dx;
    int targetY = y + dy;

    TileType targetTile = gameMap.getTileAt(targetX, targetY);

    if (targetTile == TileType::Empty || targetTile == TileType::Platform) {
        x = targetX;
        y = targetY;
        // 在终端打印太多信息会眼花，所以把移动打印先去掉了，你可以根据需要加回来
    } else if (targetTile == TileType::Wall) {
        // 撞墙了，不改变坐标
    }
}

// 核心重力与碰撞更新
void Player::update(const Map& gameMap) {
    if (!isGrounded) {
        velocityY += gravity; 
        if (velocityY > 3.0f) velocityY = 3.0f; // 防穿模
    }

    int nextY = y + (int)velocityY; 

    // 向下掉落
    if (velocityY > 0) {
        TileType targetTile = gameMap.getTileAt(x, nextY);
        if (targetTile == TileType::Wall || targetTile == TileType::Platform) {
            y = nextY - 1;       
            velocityY = 0.0f;    
            isGrounded = true;   
        } else {
            y = nextY;
            isGrounded = false;
        }
    } 
    // 向上跳跃
    else if (velocityY < 0) {
        TileType targetTile = gameMap.getTileAt(x, nextY);
        if (targetTile == TileType::Wall) {
            y = nextY + 1;       
            velocityY = 0.0f;    
        } else {
            y = nextY;
            isGrounded = false;
        }
    }
}

void Player::jump() {
    if (isGrounded) {
        velocityY = jumpForce; 
        isGrounded = false;
        std::cout << name << " 起跳！" << std::endl;
    }
}

void Player::gainExp(int amount) {
    currentExp += amount;
    std::cout << "[系统] " << name << " 获得了 " << amount << " 点经验值。(" << currentExp << "/" << expToNextLevel << ")" << std::endl;
    
    while (currentExp >= expToNextLevel) {
        levelUp();
    }
}

void Player::levelUp() {
    currentExp -= expToNextLevel; 
    level++;
    expToNextLevel = int(expToNextLevel * 1.5); 
    
    maxHp += 10;
    hp = maxHp; 
    baseDamage += 5;
    specialDamage += 10;
    
    std::cout << "✨✨✨ 升级啦！" << name << " 达到了等级 " << level << "！ ✨✨✨" << std::endl;
}

void Player::normalAttack(Role& target) {
    std::cout << name << " 挥舞圣剑，对 " << target.getName() << " 发起普通攻击！" << std::endl;
    target.takeDamage(baseDamage);
}

void Player::specialAttack(Role& target) {
    if (mana >= 33) { 
        mana -= 33;
        std::cout << name << " 释放报恩之魂，对 " << target.getName() << " 造成巨大伤害！(剩余灵魂: " << mana << ")" << std::endl;
        target.takeDamage(specialDamage);
    } else {
        std::cout << name << " 灵魂不足，无法释放特殊技能！" << std::endl;
    }
}

// ==========================================
// 派生类 Enemy 实现
// ==========================================
Enemy::Enemy(int startX, int startY, int expGive) 
    : Role("爬虫", startX, startY, 20, 5), moveDirection(1), expReward(expGive) {}

void Enemy::update(const Map& gameMap) {
    // 简易AI：水平来回移动
    x += moveDirection;
    moveDirection *= -1;
}