#pragma execution_character_set("utf-8")
#include "Enemy.h"
#include <cmath>
#include "raylib.h"

extern std::string combatLog;

Enemy::Enemy(int startX, int startY, int type)
// 🌟 核心修复 1：打通实体真实名称映射，确保主程序正确识别 Boss
    : name(type == 0 ? "Crawler" : (type == 1 ? "Flyer" : "False Knight")), x(startX), y(startY),
    realX((float)startX), realY((float)startY), spawnX(startX), spawnY((float)startY),
    enemyType(type),
    alive(true), flickerTimer(0), moveDirection(1), attackCooldown(0),
    crawlerSpeed(1.5f), currentFrame(0), animTimer(0.0f) {

    if (enemyType == 0) hp = 30;
    else if (enemyType == 1) hp = 20;
    else if (enemyType == 2) hp = 450; // 🌟 假骑士巨量生命值

    maxHp = hp;
    baseDamage = (enemyType == 2) ? 25 : 25;

    if (enemyType == 0) {
        totalFrames = 4; hbWidth = 0.8f; hbHeight = 0.8f;
    }
    else if (enemyType == 1) {
        totalFrames = 4; hbWidth = 0.8f; hbHeight = 0.5f;
    }
    else if (enemyType == 2) {
        // 🌟 假骑士完整属性与初始状态分配
        totalFrames = 1;
        hbWidth = 2.5f;
        hbHeight = 3.0f;
        bossState = 0;
        stateTimer = 1.0f;
        isEnraged = false;
        crawlerSpeed = 16.0f; // 空中飞跃的极速移动系数
    }

    currentFrame = (totalFrames > 1) ? GetRandomValue(0, totalFrames - 1) : 0;
}

bool Enemy::takeDamage(int damage, int sourceX, const Map& gameMap) {
    if (flickerTimer > 0 || !alive) return false;

    hp -= damage;
    if (hp <= 0) {
        alive = false;
        combatLog = (enemyType == 2) ? "击败假骑士！ " : "击杀目标！ ";
    }
    else {
        flickerTimer = 15;
        // 挨打顿帧：Boss在思考待机时受击，稍微增加决策延迟增加受击打击感
        if (enemyType == 2 && bossState == 0) {
            stateTimer += 0.05f;
        }

        // 普通小怪受击后退，沉重的巨兽 Boss 不被普通攻击推退
        if (enemyType != 2) {
            int knockbackDir = (x > sourceX) ? 1 : -1;
            TileType backWall = gameMap.getTileAt(x + knockbackDir, y);
            if (backWall != TileType::Wall) {
                realX += knockbackDir * 0.5f;
                x = (int)std::round(realX);
            }
        }
    }
    return true;
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
        bool inSight = (distToPlayerX <= 10.0f && distToPlayerY <= 5.0f);

        if (aiState == 0) {
            realY = spawnY + sin(GetTime() * 3.0f + spawnX) * 0.5f;

            if (inSight) {
                aiState = 1;
                stateTimer = 0.3f;
            }
            else {
                realX += moveDirection * crawlerSpeed * 0.5f * dt;
                if (std::abs(realX - spawnX) > 4.0f) moveDirection *= -1;
            }
        }
        else if (aiState == 1) {
            realX += (GetRandomValue(-1, 1) * 0.02f);
            stateTimer -= dt;
            if (stateTimer <= 0.0f) {
                aiState = 2;
            }
        }
        else if (aiState == 2) {
            if (distToPlayerX > 14.0f || distToPlayerY > 10.0f) {
                aiState = 3;
            }
            else {
                moveDirection = (player.getRealX() > realX) ? 1 : -1;
                realX += moveDirection * crawlerSpeed * 1.8f * dt;

                float targetY = player.getRealY() - 0.2f;
                if (realY < targetY) realY += crawlerSpeed * 1.5f * dt;
                else if (realY > targetY) realY -= crawlerSpeed * 1.5f * dt;
            }
        }
        else if (aiState == 3) {
            moveDirection = (spawnX > realX) ? 1 : -1;
            realX += moveDirection * crawlerSpeed * 1.0f * dt;

            if (realY < spawnY) realY += crawlerSpeed * 1.0f * dt;
            else if (realY > spawnY) realY -= crawlerSpeed * 1.0f * dt;

            if (std::abs(realX - spawnX) < 0.5f && std::abs(realY - spawnY) < 0.5f) {
                aiState = 0;
            }

            if (inSight) {
                aiState = 1;
                stateTimer = 0.2f;
            }
        }

        float probeX = realX + moveDirection * (hbWidth / 2.0f + 0.1f);
        if (gameMap.getTileAt((int)probeX, (int)(realY + 0.5f)) == TileType::Wall) {
            if (aiState == 0 || aiState == 3) moveDirection *= -1;
            else realX -= moveDirection * crawlerSpeed * 1.8f * dt;
        }

        x = (int)std::round(realX);
        y = (int)std::round(realY);

        animTimer += dt;
        float frameDuration = (aiState == 2) ? 0.06f : 0.10f;
        if (animTimer >= frameDuration) {
            currentFrame = (currentFrame + 1) % totalFrames;
            animTimer = 0.0f;
        }
    }
    else if (enemyType == 0) {
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
    else if (enemyType == 2) {
        // ==========================================
        // 🛡️ 假骑士 Boss AI 状态机复刻 (Type 2)
        // ==========================================
        // 维护独立的地面滚动冲击波
        for (auto it = shockwaves.begin(); it != shockwaves.end(); ) {
            it->x += (it->speed * it->direction) * dt;
            it->lifeTimer -= dt;

            if (it->lifeTimer <= 0 || gameMap.getTileAt((int)it->x, (int)it->y) == TileType::Wall) {
                it = shockwaves.erase(it);
            }
            else {
                if (it->getHitbox().intersects(player.getHitbox())) {
                    player.takeDamage(25, (int)it->x, gameMap);
                }
                ++it;
            }
        }

        // 沉重的物理重力检测
        float bottomY = realY;
        int gridX = (int)std::round(realX);
        TileType underFoot = gameMap.getTileAt(gridX, (int)(bottomY + 0.05f));
        bool isBossGrounded = (underFoot == TileType::Wall || underFoot == TileType::Platform);

        if (isBossGrounded && bossState != 2) {
            realY = std::floor(bottomY);
        }
        else {
            realY += 18.0f * dt;
        }

        if (hp < maxHp / 2 && !isEnraged) {
            isEnraged = true;
        }

        // 核心 AI 状态调度
        if (bossState == 0) { // 待机逼近 (boss_stand)
            stateTimer -= dt;
            moveDirection = (player.getRealX() > realX) ? 1 : -1;

            if (stateTimer <= 0.0f) {
                // 距离大于5格放毁天灭地冲击波，否则直接大跳换位压杀
                if (distToPlayerX > 5.0f) {
                    bossState = 3; // 举锤蓄力
                    stateTimer = 0.8f;
                    maceSwingDir = moveDirection;
                }
                else {
                    bossState = 1; // 准备起跳
                    stateTimer = 0.3f; // 起跳下蹲缓冲
                }
            }
        }
        else if (bossState == 1) { // 下蹲蓄力准备起跳 (boss_jump)
            stateTimer -= dt;
            if (stateTimer <= 0.0f) {
                bossState = 2; // 腾空跳跃
                realY -= 0.5f; // 反冲升空
                moveDirection = (player.getRealX() > realX) ? 1 : -1;
            }
        }
        else if (bossState == 2) { // 腾空飞跃 (boss_air)
            realX += moveDirection * crawlerSpeed * dt;
            // 落地震感反馈
            if (isBossGrounded) {
                bossState = 0;
                stateTimer = 1.2f;
                flickerTimer = 10; // 传递强力着陆后坐力信号触发震屏
            }
        }
        else if (bossState == 3) { // 挥锤蓄力前摇 (boss_swing)
            stateTimer -= dt;
            if (stateTimer <= 0.0f) {
                bossState = 4; // 轰击地面！
                stateTimer = 0.5f;
                flickerTimer = 12; // 强烈暴击感触发主程序捕获大震动

                // 🌟 生成沿地面飞驰的死亡冲击波
                Shockwave wave;
                wave.x = realX + maceSwingDir * 2.0f;
                wave.y = realY;
                wave.speed = isEnraged ? 14.0f : 10.0f;
                wave.direction = maceSwingDir;
                wave.lifeTimer = 3.0f;
                wave.height = 1.8f; // 🌟 峰高 1.8 格，强制长按大跳跨过！
                shockwaves.push_back(wave);
            }
        }
        else if (bossState == 4) { // 保持砸地后摇姿势 (boss_hit)
            stateTimer -= dt;
            if (stateTimer <= 0.0f) {
                bossState = 0;
                stateTimer = 1.5f;
            }
        }

        x = (int)std::round(realX);
        y = (int)std::round(realY);
    }

    if (getHitbox().intersects(player.getHitbox()) && attackCooldown <= 0) {
        player.takeDamage(baseDamage, x, gameMap);
        attackCooldown = 30;
    }
}