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

    // 【重力与地板吸附系统】
    float bottomY = realY + 1.0f;
    int gridX = (int)(realX + 0.5f);
    TileType underTile = gameMap.getTileAt(gridX, (int)(bottomY + 0.05f));

    if (underTile == TileType::Wall || underTile == TileType::Platform) {
        realY = std::floor(bottomY) - 1.0f; // 紧贴地面
    }
    else {
        realY += 12.0f * dt; // 掉落
    }

    // ==========================================
    // 🧠 状态机 (0:巡逻, 1:前摇, 2:狂暴追击, 3:撞墙冷却)
    // ==========================================
    float distToPlayerX = std::abs(player.getRealX() - realX);
    float distToPlayerY = std::abs(player.getRealY() - realY);
    bool inSight = (distToPlayerX <= 6.0f && distToPlayerY <= 2.0f);

    float currentSpeed = 0.0f;

    if (aiState == 0) { // 🚶 巡逻状态
        currentSpeed = crawlerSpeed;
        if (inSight) {
            aiState = 1;               // 发现目标，进入前摇！
            stateTimer = 0.5f;         // 蓄力发呆 0.5 秒
            moveDirection = (player.getRealX() > realX) ? 1 : -1; // 死死盯住玩家方向
        }
        else {
            // 正常的出生点巡逻折返
            if (moveDirection == 1 && realX >= spawnX + 3.0f) moveDirection = -1;
            else if (moveDirection == -1 && realX <= spawnX - 3.0f) moveDirection = 1;
        }
    }
    else if (aiState == 1) { // 💢 前摇蓄力状态
        currentSpeed = 0.0f; // 停在原地，压迫感拉满
        stateTimer -= dt;
        if (stateTimer <= 0.0f) {
            aiState = 2; // 蓄力结束，开始冲锋！
        }
    }
    else if (aiState == 2) { // 🩸 狂暴冲锋状态
        currentSpeed = crawlerSpeed * 2.5f;
        moveDirection = (player.getRealX() > realX) ? 1 : -1;
        if (!inSight) aiState = 0; // 玩家跑远了，放弃追击，恢复巡逻
    }
    else if (aiState == 3) { // 🌀 迷茫冷却状态 (撞墙后触发)
        currentSpeed = crawlerSpeed * 0.5f; // 慢悠悠地反向走
        stateTimer -= dt;
        if (stateTimer <= 0.0f) aiState = 0; // 冷却结束，恢复正常巡逻
    }

    // ==========================================
    // 🛡️ 探路天线与碰撞检测
    // ==========================================
    float probeX = realX + moveDirection * (hbWidth / 2.0f + 0.2f);
    int probeGridX = (int)probeX;
    int probeGridY = (int)(realY + 0.5f);
    int probeFloorY = (int)(realY + 1.1f);

    TileType nextWall = gameMap.getTileAt(probeGridX, probeGridY);
    TileType nextFloor = gameMap.getTileAt(probeGridX, probeFloorY);

    if (nextWall == TileType::Wall || nextFloor == TileType::Void || nextFloor == TileType::SpikeUp) {
        moveDirection *= -1; // 强制转身
        realX += (moveDirection == 1 ? 0.05f : -0.05f); // 防抖动推离

        // 🌟【消灭鬼畜的核心修复】：如果是在追击玩家时撞墙，说明被地形卡住了！
        // 直接进入状态 3 (冷却)，放弃思考 1.5 秒，它就不会再回头鬼畜了！
        if (aiState == 2) {
            aiState = 3;
            stateTimer = 1.5f;
        }
    }
    else {
        realX += moveDirection * currentSpeed * dt;
    }

    x = (int)std::round(realX);
    y = (int)std::round(realY);

    // ==========================================
    // 🎬 动画与伤害判定
    // ==========================================
    animTimer += dt;
    // 状态决定动画速度：狂暴时很快，前摇抽搐时极快，平时正常
    float frameDuration = 0.15f;
    if (aiState == 2) frameDuration = 0.06f;
    else if (aiState == 1) frameDuration = 0.04f; // 前摇时腿捣得飞快，原地抽搐蓄力

    if (animTimer >= frameDuration) {
        currentFrame = (currentFrame + 1) % totalFrames;
        animTimer = 0.0f;
    }

    if (getHitbox().intersects(player.getHitbox()) && attackCooldown <= 0) {
        player.takeDamage(baseDamage, x, gameMap);
        attackCooldown = 30;
    }
}