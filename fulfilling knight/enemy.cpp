#pragma execution_character_set("utf-8")
#include "Enemy.h"
#include <cmath>
#include "raylib.h"

extern std::string combatLog;

Enemy::Enemy(int startX, int startY, int type)
    : name(type == 0 ? "Crawler" : "Flyer"), x(startX), y(startY),
    realX((float)startX), realY((float)startY), spawnX(startX), spawnY((float)startY),
    hbWidth(0.8f), hbHeight(0.5f), enemyType(type),
    alive(true), flickerTimer(0), moveDirection(1), attackCooldown(0),
    crawlerSpeed(1.5f), currentFrame(0), animTimer(0.0f), totalFrames(5) {

    hp = (enemyType == 0) ? 30 : 20;
    maxHp = hp;
    baseDamage = 25; // 撞击统一 25 点伤害

    totalFrames = (enemyType == 0) ? 5 : 4;
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
            realX += knockbackDir * 0.5f;
            x = (int)std::round(realX);
        }
    }
}

void Enemy::update(const Map& gameMap, Player& player, float dt) {
    if (!alive) return;
    if (flickerTimer > 0) flickerTimer--;
    if (attackCooldown > 0) attackCooldown--;

    float distToPlayerX = std::abs(player.getRealX() - realX);
    float distToPlayerY = std::abs(player.getRealY() - realY);

    if (enemyType == 1) {
        // ==========================================
        // 🦋 飞虫逻辑 (Type 1)
        // ==========================================
        realY = spawnY + sin(GetTime() * 3.0f + spawnX) * 0.5f;

        if (distToPlayerX <= 8.0f && distToPlayerY <= 4.0f) {
            moveDirection = (player.getRealX() > realX) ? 1 : -1;
            realX += moveDirection * crawlerSpeed * 1.5f * dt;
        }
        else {
            realX += moveDirection * crawlerSpeed * 0.5f * dt;
            if (std::abs(realX - spawnX) > 4.0f) moveDirection *= -1;
        }

        x = (int)std::round(realX);
        y = (int)std::round(realY);

        animTimer += dt;
        if (animTimer >= 0.10f) {
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