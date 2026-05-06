#pragma execution_character_set("utf-8")
#include "Enemy.h"
#include <cmath>
#include "raylib.h"

extern std::string combatLog;

Enemy::Enemy(int startX, int startY, int type)
    : name(type == 0 ? "Crawler" : "Flyer"), x(startX), y(startY),
    realX((float)startX), realY((float)startY), spawnX(startX), spawnY((float)startY),
    enemyType(type),
    alive(true), flickerTimer(0), moveDirection(1), attackCooldown(0),
    crawlerSpeed(1.5f), currentFrame(0), animTimer(0.0f) {

    hp = (enemyType == 0) ? 30 : 20;
    maxHp = hp;
    baseDamage = 25;

    // 🌟 核心修改 1：现在不管是爬虫还是飞虫，大家都是 4 帧动作了！
    totalFrames = 4;

    // 🌟 核心修改 2：碰撞箱比例升级！
    if (enemyType == 0) {
        // 爬虫 (1:1的身材，把高度拉满到 0.8)
        hbWidth = 0.8f;
        hbHeight = 0.8f;
    }
    else {
        // 飞虫保持不变
        hbWidth = 0.8f;
        hbHeight = 0.5f;
    }

    currentFrame = GetRandomValue(0, totalFrames - 1);
}

bool Enemy::takeDamage(int damage, int sourceX, const Map& gameMap) {
    // 🌟 如果怪物已经死了，或者还在无敌时间，返回 false（表示这刀砍在残影上了）
    if (flickerTimer > 0 || !alive) return false;

    hp -= damage;
    if (hp <= 0) {
        alive = false; combatLog = "击杀目标！ ";
    }
    else {
        // 🌟 核心修复：把 5 改成 15！
        // 既然我们的剑刃判定变持久了，怪物的无敌帧也要增加到 0.25 秒，防止被同一刀扣两次血！
        flickerTimer = 15;
        int knockbackDir = (x > sourceX) ? 1 : -1;
        TileType backWall = gameMap.getTileAt(x + knockbackDir, y);
        if (backWall != TileType::Wall) {
            realX += knockbackDir * 0.5f;
            x = (int)std::round(realX);
        }
    }
    return true; // 🌟 返回 true，告诉主角：你确实砍到真肉了，赶紧爆火花！
}

void Enemy::update(const Map& gameMap, Player& player, float dt) {
    if (!alive) return;
    if (flickerTimer > 0) flickerTimer--;
    if (attackCooldown > 0) attackCooldown--;

    float distToPlayerX = std::abs(player.getRealX() - realX);
    float distToPlayerY = std::abs(player.getRealY() - realY);

    // ==========================================
        // 🦋 飞虫逻辑 (Type 1)
        // ==========================================
    if (enemyType == 1) {
        // 🌟 索敌雷达放大：X轴 10格内，Y轴 5格内均可发现玩家
        float distToPlayerX = std::abs(player.getRealX() - realX);
        float distToPlayerY = std::abs(player.getRealY() - realY);
        bool inSight = (distToPlayerX <= 10.0f && distToPlayerY <= 5.0f);

        if (aiState == 0) { // 🚶 巡逻状态
            // 无视重力，上下正弦波浮动
            realY = spawnY + sin(GetTime() * 3.0f + spawnX) * 0.5f;

            if (inSight) {
                aiState = 1;               // 发现目标，进入前摇！
                stateTimer = 0.3f;         // 惊恐停顿 0.3 秒 (尖叫)
            }
            else {
                // 缓慢巡逻
                realX += moveDirection * crawlerSpeed * 0.5f * dt;
                if (std::abs(realX - spawnX) > 4.0f) moveDirection *= -1; // 飞出 4 格折返
            }
        }
        else if (aiState == 1) { // 💢 发现目标，僵直前摇
            // 原地颤抖，营造压迫感
            realX += (GetRandomValue(-1, 1) * 0.02f);
            stateTimer -= dt;
            if (stateTimer <= 0.0f) {
                aiState = 2; // 前摇结束，开始追击！
            }
        }
        else if (aiState == 2) { // 🩸 狂暴追击状态 (X和Y轴全方位追踪)
            // 丢失目标 (玩家跑出 14 格远)
            if (distToPlayerX > 14.0f || distToPlayerY > 10.0f) {
                aiState = 3; // 放弃追击，准备回家
            }
            else {
                // X轴冲刺飞向玩家
                moveDirection = (player.getRealX() > realX) ? 1 : -1;
                realX += moveDirection * crawlerSpeed * 1.8f * dt;

                // 🌟 核心修复：Y轴追踪，飞虫要俯冲下来咬人！
                float targetY = player.getRealY() - 0.2f; // 瞄准骑士头部稍高一点
                if (realY < targetY) realY += crawlerSpeed * 1.5f * dt;
                else if (realY > targetY) realY -= crawlerSpeed * 1.5f * dt;
            }
        }
        else if (aiState == 3) { // 🌀 失去目标，飞回出生点
            moveDirection = (spawnX > realX) ? 1 : -1;
            realX += moveDirection * crawlerSpeed * 1.0f * dt;

            if (realY < spawnY) realY += crawlerSpeed * 1.0f * dt;
            else if (realY > spawnY) realY -= crawlerSpeed * 1.0f * dt;

            // 🌟 核心修复：加个极小误差判定，回到家后完美恢复巡逻
            if (std::abs(realX - spawnX) < 0.5f && std::abs(realY - spawnY) < 0.5f) {
                aiState = 0;
            }

            // 如果在回家路上又看到玩家，马上回头继续追！
            if (inSight) {
                aiState = 1;
                stateTimer = 0.2f;
            }
        }

        // ==========================================
        // 🧱 墙壁碰撞检测 (防止飞虫卡进泥土里)
        // ==========================================
        float probeX = realX + moveDirection * (hbWidth / 2.0f + 0.1f);
        if (gameMap.getTileAt((int)probeX, (int)(realY + 0.5f)) == TileType::Wall) {
            if (aiState == 0 || aiState == 3) moveDirection *= -1; // 巡逻/回家时撞墙转身
            else realX -= moveDirection * crawlerSpeed * 1.8f * dt; // 追击时被墙挡住 (推离墙体)
        }

        // 更新坐标与动画
        x = (int)std::round(realX);
        y = (int)std::round(realY);

        animTimer += dt;
        // 狂暴追击时，翅膀扑腾的速度也会加快！
        float frameDuration = (aiState == 2) ? 0.06f : 0.10f;
        if (animTimer >= frameDuration) {
            currentFrame = (currentFrame + 1) % totalFrames;
            animTimer = 0.0f;
        }
    }
    else {
        // ==========================================
        // 🪲 爬虫逻辑 (Type 0)
        // ==========================================
        float bottomY = realY + 1.0f;
        int gridX = (int)(realX + 0.5f);
        TileType underTile = gameMap.getTileAt(gridX, (int)(bottomY + 0.05f));

        if (underTile == TileType::Wall || underTile == TileType::Platform) {
            realY = std::floor(bottomY) - 1.0f;
        }
        else {
            realY += 12.0f * dt;
        }

        bool inSight = (distToPlayerX <= 6.0f && distToPlayerY <= 2.0f);
        float currentSpeed = 0.0f;

        if (aiState == 0) {
            currentSpeed = crawlerSpeed;
            if (inSight) {
                aiState = 1;
                stateTimer = 0.5f;
                moveDirection = (player.getRealX() > realX) ? 1 : -1;
            }
            else {
                if (moveDirection == 1 && realX >= spawnX + 3.0f) moveDirection = -1;
                else if (moveDirection == -1 && realX <= spawnX - 3.0f) moveDirection = 1;
            }
        }
        else if (aiState == 1) {
            currentSpeed = 0.0f;
            stateTimer -= dt;
            if (stateTimer <= 0.0f) aiState = 2;
        }
        else if (aiState == 2) {
            currentSpeed = crawlerSpeed * 2.5f;
            moveDirection = (player.getRealX() > realX) ? 1 : -1;
            if (!inSight) aiState = 0;
        }
        else if (aiState == 3) {
            currentSpeed = crawlerSpeed * 0.5f;
            stateTimer -= dt;
            if (stateTimer <= 0.0f) aiState = 0;
        }

        float probeX = realX + moveDirection * (hbWidth / 2.0f + 0.2f);
        int probeGridX = (int)probeX;
        int probeGridY = (int)(realY + 0.5f);
        int probeFloorY = (int)(realY + 1.1f);

        TileType nextWall = gameMap.getTileAt(probeGridX, probeGridY);
        TileType nextFloor = gameMap.getTileAt(probeGridX, probeFloorY);

        if (nextWall == TileType::Wall || nextFloor == TileType::Void || nextFloor == TileType::SpikeUp) {
            moveDirection *= -1;
            realX += (moveDirection == 1 ? 0.05f : -0.05f);
            if (aiState == 2) { aiState = 3; stateTimer = 1.5f; }
        }
        else {
            realX += moveDirection * currentSpeed * dt;
        }

        x = (int)std::round(realX);
        y = (int)std::round(realY);

        animTimer += dt;
        float frameDuration = 0.15f;
        if (aiState == 2) frameDuration = 0.06f;
        else if (aiState == 1) frameDuration = 0.04f;

        if (animTimer >= frameDuration) {
            currentFrame = (currentFrame + 1) % totalFrames;
            animTimer = 0.0f;
        }
    }

    if (getHitbox().intersects(player.getHitbox()) && attackCooldown <= 0) {
        player.takeDamage(baseDamage, x, gameMap);
        attackCooldown = 30;
    }
}