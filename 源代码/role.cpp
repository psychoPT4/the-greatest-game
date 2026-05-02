#include "../头文件/Role.h"
#include <iostream>
#include <cmath>
#include <algorithm> // 用于 std::clamp

extern std::string combatLog;

Role::Role(std::string n, int startX, int startY, int maxHp, int dmg)
    : name(n), x(startX), y(startY), hp(maxHp), maxHp(maxHp), baseDamage(dmg), 
      alive(true), flickerTimer(0), realX((float)startX), realY((float)startY) {}

void Role::takeDamage(int amount, int attackerX, const Map& gameMap) {
    // 【核心修复】：如果已经死了，或者正处于“无敌闪烁”状态，则免疫此次伤害
    if (!alive || flickerTimer > 0) return; 
    
    hp -= amount;
    
    // 设置无敌帧时长：60 帧（大约 1 到 1.5 秒无敌时间，配合闪烁效果）
    flickerTimer = 60; 
    
    // 击退逻辑 (后退 2 格)
    int kDir = (x > attackerX) ? 1 : -1;
    float targetX = realX + kDir * 2.0f; 
    if (gameMap.getTileAt((int)round(targetX), y) != TileType::Wall) {
        realX = targetX; 
        x = (int)round(realX);
    }
    
    if (hp <= 0) { 
        hp = 0; 
        alive = false; 
        combatLog = name + " 已倒下！"; 
    }
}

// ==========================================
// Player: 空洞骑士动力学模型
// ==========================================
Player::Player(int startX, int startY)
    : Role("骑士", startX, startY, 100, 25), 
      level(1), currentExp(0), expToNextLevel(100), mana(100), facingDirection(1), jumpCount(0), 
      isGrounded(false), moveIntent(0),
      velocityX(0.0f), velocityY(0.0f),
      // 物理常数 (基于 deltaTime 缩放)
      maxRunSpeed(15.0f),  // 最大横向速度 (格/秒)
      runAccel(150.0f),    // 加速度：0.1秒达到满速
      groundFriction(200.0f), // 地面摩擦力：极高，实现松手即停
      airDrag(80.0f),      // 空气阻力：较低，保证空中手感
      gravity(85.0f),      // 重力加速度
      jumpForce(-26.0f),   // 起跳爆发力
      maxFallSpeed(30.0f)  // 终端下落速度
{}

void Player::setMoveIntent(int dir) {
    moveIntent = dir;
    if (dir != 0) facingDirection = dir;
}

void Player::processJump(bool jumpPressed, bool jumpHeld) {
    // 初次起跳
    if (jumpPressed) {
        if (isGrounded) { 
            velocityY = jumpForce; isGrounded = false; jumpCount = 1; 
        } else if (jumpCount < 2) { 
            velocityY = jumpForce * 0.85f; jumpCount = 2; combatLog = "二段跳！";
        }
    }
    // 【空洞骑士核心】：松开跳跃键，向上速度瞬间衰减，实现精准跳跃控制
    if (!jumpHeld && velocityY < 0) {
        velocityY *= 0.5f; // 瞬间砍掉一半向上的动能
    }
}

void Player::update(const Map& gameMap, float dt) {
    if (!alive) return;
    if (flickerTimer > 0) flickerTimer--;

    // 1. 状态判定
    TileType under = gameMap.getTileAt(x, y + 1);
    if (isGrounded && under != TileType::Wall && under != TileType::Platform) {
        isGrounded = false;
    }

    // 2. 水平动力学 (X轴)
    float currentFriction = isGrounded ? groundFriction : airDrag;
    
    if (moveIntent != 0) {
        // 主动加速
        velocityX += runAccel * moveIntent * dt;
        // 速度钳制
        velocityX = std::clamp(velocityX, -maxRunSpeed, maxRunSpeed);
    } else {
        // 松手后的阻尼衰减
        if (velocityX > 0) {
            velocityX -= currentFriction * dt;
            if (velocityX < 0) velocityX = 0;
        } else if (velocityX < 0) {
            velocityX += currentFriction * dt;
            if (velocityX > 0) velocityX = 0;
        }
    }

    // 更新 X 坐标
    float nextRealX = realX + velocityX * dt;
    int nextXInt = (int)round(nextRealX);
    if (gameMap.getTileAt(nextXInt, y) != TileType::Wall) {
        realX = nextRealX; x = nextXInt;
    } else {
        velocityX = 0.0f; // 撞墙瞬间失去所有横向动能
    }

    // 3. 垂直动力学 (Y轴)
    if (!isGrounded) {
        velocityY += gravity * dt;
        if (velocityY > maxFallSpeed) velocityY = maxFallSpeed;
    }
    
    realY += velocityY * dt;
    int nextY = (int)round(realY);

    if (velocityY > 0) { // 下落碰撞检测
        bool hit = false;
        for (int i = y + 1; i <= nextY; ++i) {
            TileType t = gameMap.getTileAt(x, i);
            if ((t == TileType::Wall || t == TileType::Platform) && y < i) {
                y = i - 1; realY = (float)y; velocityY = 0.0f; 
                isGrounded = true; jumpCount = 0; hit = true; break;
            }
        }
        if (!hit) y = (nextY < 15) ? nextY : 14;
    } 
    else if (velocityY < 0) { // 上升撞头检测
        TileType head = gameMap.getTileAt(x, nextY);
        if (head == TileType::Wall) {
            velocityY = 0.0f; realY = (float)y; // 撞天花板动能清零
        } else {
            y = (nextY >= 0) ? nextY : 0;
        }
    }
}

bool Player::attack(std::vector<Enemy>& enemies, const Map& gameMap, bool downPressed) {
    bool pogoSuccess = false;
    
    // ==========================================
    // 1. 空洞骑士神技：下劈 (在空中 + 按下方向键)
    // ==========================================
    if (!isGrounded && downPressed) {
        // A. 检测是否下劈到了怪物
        for (auto& target : enemies) {
            if (!target.isAlive()) continue;
            // 判定条件：怪物在脚下 1~2 格距离内，且水平基本对齐
            if (abs(target.getX() - x) <= 1 && (target.getY() - y) > 0 && (target.getY() - y) <= 2) {
                target.takeDamage(baseDamage, x, gameMap);
                pogoSuccess = true;
                combatLog = "【下劈弹刀！】命中爬虫！";
            }
        }
        
        // B. 检测是否下劈到了地刺 (借助危险环境跳跃)
        TileType tileBelow = gameMap.getTileAt(x, y + 2); 
        if (tileBelow == TileType::SpikeUp) {
            pogoSuccess = true;
            combatLog = "【下劈弹刀！】借助地刺弹起！";
        }
        
        // C. 弹刀成功，重置物理状态
        if (pogoSuccess) {
            velocityY = jumpForce * 0.9f; // 赋予向上的反冲动能
            jumpCount = 1; // 刷新二段跳次数
            return true;
        }
    } 
    // ==========================================
    // 2. 正常平砍 (左右挥剑)
    // ==========================================
    else {
        bool hitSomething = false;
        for (auto& target : enemies) {
            if (!target.isAlive()) continue;
            // 判定条件：水平距离<=2，且方向朝向怪物，垂直误差<=1
            if (abs(target.getY() - y) <= 1 && abs(target.getX() - x) <= 2 && (target.getX() - x) * facingDirection >= 0) {
                target.takeDamage(baseDamage, x, gameMap);
                hitSomething = true;
                combatLog = "发起平砍！";
            }
        }
        if (!hitSomething) combatLog = "挥空了...";
    }
    return false;
}
// ==========================================
// Enemy 逻辑 (同步 dt 接口)
// ==========================================

Enemy::Enemy(int startX, int startY, int expGive)
    : Role("爬虫", startX, startY, 40, 12), moveDirection(1), expReward(expGive), attackCooldown(0), moveTimer(0.0f) {}

void Enemy::update(const Map& gameMap, Player& player, float dt) {
    if (!alive) return;
    if (flickerTimer > 0) flickerTimer--;
    if (attackCooldown > 0) attackCooldown--;

    // 悬空坠落逻辑
    if (gameMap.getTileAt(x, y + 1) == TileType::Empty || gameMap.getTileAt(x, y + 1) == TileType::Void) {
        y++; realY = (float)y;
    } else {
        // 【核心修复】：使用独立计时器累加 dt
        moveTimer += dt;
        if (moveTimer >= 0.15f) { // 每 0.15 秒移动一次
            moveTimer = 0.0f; // 重置专属计时器
            
            int nx = x + moveDirection;
            TileType nextFloor = gameMap.getTileAt(nx, y + 1);
            TileType nextWall = gameMap.getTileAt(nx, y);
            
            // 边缘探测：如果前面是墙，或者前面脚下是空气/深渊，就转身
            if (nextWall == TileType::Wall || nextFloor == TileType::Empty || nextFloor == TileType::Void) {
                moveDirection *= -1;
            } else {
                x = nx; realX = (float)x;
            }
        }
    }

    // 攻击判定
    if (abs(player.getX() - x) <= 1 && abs(player.getY() - y) <= 1 && attackCooldown <= 0) {
        player.takeDamage(baseDamage, x, gameMap);
        attackCooldown = 20;
    }
}