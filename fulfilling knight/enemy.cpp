#pragma execution_character_set("utf-8")
#include "Enemy.h"
#include <cmath>
#include "raylib.h"

extern std::string combatLog;

Enemy::Enemy(int startX, int startY, int expGive)
    : name("Crawler"), x(startX), y(startY), realX((float)startX), realY((float)startY),
    hbWidth(0.8f), hbHeight(0.5f), hp(40), maxHp(40), baseDamage(12),
    alive(true), flickerTimer(0),
    moveDirection(1), expReward(expGive), attackCooldown(0),
    crawlerSpeed(1.5f), spawnX(startX), currentFrame(0), animTimer(0.0f), totalFrames(5) {
    currentFrame = GetRandomValue(0, totalFrames - 1);
}

void Enemy::takeDamage(int damage, int sourceX, const Map& gameMap) {
    if (flickerTimer > 0 || !alive) return;
    hp -= damage;
    if (hp <= 0) {
        alive = false; combatLog = "击杀目标！ ";
    }
    else {
        flickerTimer = 5;
        int knockbackDir = (x > sourceX) ? 1 : -1;
        TileType backWall = gameMap.getTileAt(x + knockbackDir, y);
        if (backWall != TileType::Wall) {
            realX += knockbackDir * 0.5f; // 轻微击退，拒绝瞬间强制覆写
            x = (int)std::round(realX);
        }
    }
}

void Enemy::update(const Map& gameMap, Player& player, float dt) {
    if (!alive) return;
    if (flickerTimer > 0) flickerTimer--;
    if (attackCooldown > 0) attackCooldown--;

    // 【终极修复一：物理引力与地板对齐防浮空】
    float bottomY = realY + 1.0f;
    int gridX = (int)(realX + 0.5f);
    TileType underTile = gameMap.getTileAt(gridX, (int)(bottomY + 0.05f));

    if (underTile == TileType::Wall || underTile == TileType::Platform) {
        realY = std::floor(bottomY) - 1.0f; // 紧贴地面，拒绝浮空
    }
    else {
        realY += 12.0f * dt; // 掉落
    }

    // AI 状态机
    float distToPlayerX = std::abs(player.getRealX() - realX);
    float distToPlayerY = std::abs(player.getRealY() - realY);
    bool isChasing = (distToPlayerX <= 4.0f && distToPlayerY <= 1.5f);

    if (isChasing) {
        moveDirection = (player.getRealX() > realX) ? 1 : -1;
    }
    else {
        if (moveDirection == 1 && realX >= spawnX + 3.0f) moveDirection = -1;
        else if (moveDirection == -1 && realX <= spawnX - 3.0f) moveDirection = 1;
    }

    // 【终极修复二：探路天线与连续物理位移结合】
    float probeX = realX + moveDirection * (hbWidth / 2.0f + 0.2f);
    int probeGridX = (int)probeX;
    int probeGridY = (int)(realY + 0.5f);
    int probeFloorY = (int)(realY + 1.1f);

    TileType nextWall = gameMap.getTileAt(probeGridX, probeGridY);
    TileType nextFloor = gameMap.getTileAt(probeGridX, probeFloorY);

    bool wallSafe = (nextWall == TileType::Empty || nextWall == TileType::Void);
    bool floorSafe = (nextFloor == TileType::Wall || nextFloor == TileType::Platform);

    if (!wallSafe || !floorSafe) {
        if (!isChasing) moveDirection *= -1; // 碰壁/悬崖转身
    }
    else {
        realX += moveDirection * crawlerSpeed * dt; // 连续物理移动，彻底消灭瞬移！
    }

    x = (int)std::round(realX);
    y = (int)std::round(realY);

    // 动画推进
    animTimer += dt;
    if (animTimer >= 0.15f) {
        currentFrame = (currentFrame + 1) % totalFrames;
        animTimer = 0.0f;
    }

    // 伤害探测
    if (getHitbox().intersects(player.getHitbox()) && attackCooldown <= 0) {
        player.takeDamage(baseDamage, x, gameMap);
        attackCooldown = 30;
    }
}