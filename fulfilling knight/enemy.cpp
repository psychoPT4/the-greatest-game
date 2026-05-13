#pragma execution_character_set("utf-8")
#include "Enemy.h"
#include <cmath>
#include "raylib.h"

extern std::string combatLog;

Enemy::Enemy(int startX, int startY, int type)
// 🌟 核心映射：确保名字严格赋为 "False Knight"，引导 main.cpp 正确分发超大贴图渲染
    : name((type == 0) ? "Crawler" : ((type == 1) ? "Flyer" : "False Knight")),
    x(startX), y(startY),
    realX((float)startX), realY((float)startY), spawnX(startX), spawnY((float)startY),
    enemyType((type == 12) ? 2 : type), // 完美兼容直接传入 CSV 图块代码 12 的场景
    alive(true), flickerTimer(0), moveDirection(1), attackCooldown(0),
    crawlerSpeed(1.5f), currentFrame(0), animTimer(0.0f) {

    if (enemyType == 0) hp = 30;
    else if (enemyType == 1) hp = 20;
    else hp = 450; // 🌟 假骑士拥有 450 点巨量生命值

    maxHp = hp;
    baseDamage = (enemyType == 2) ? 25 : 25;

    if (enemyType == 0) {
        totalFrames = 4; hbWidth = 0.8f; hbHeight = 0.8f;
    }
    else if (enemyType == 1) {
        totalFrames = 4; hbWidth = 0.8f; hbHeight = 0.5f;
    }
    else {
        // 🌟 假骑士底层状态装载
        enemyType = 2; // 强制锁死内部归属
        totalFrames = 1;
        hbWidth = 2.5f;
        hbHeight = 3.0f;
        bossState = 0;
        stateTimer = 1.0f;
        isEnraged = false;
        crawlerSpeed = 16.0f; // 大跳飞跃的水平极速系数
        animTimer = 0.0f;     // 安全复用闲置的 animTimer 作为独立垂直速度 bossVy
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
        // 受击卡顿反馈：Boss在待机思考时受击稍微略增顿帧，大幅提升刀肉相撞的打击感
        if (enemyType == 2 && bossState == 0) {
            stateTimer += 0.05f;
        }

        // 沉重的巨兽 Boss 拥有绝对霸体，不被普通平A推退
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
            if (inSight) { aiState = 1; stateTimer = 0.3f; }
            else {
                realX += moveDirection * crawlerSpeed * 0.5f * dt;
                if (std::abs(realX - spawnX) > 4.0f) moveDirection *= -1;
            }
        }
        else if (aiState == 1) {
            realX += (GetRandomValue(-1, 1) * 0.02f);
            stateTimer -= dt;
            if (stateTimer <= 0.0f) aiState = 2;
        }
        else if (aiState == 2) {
            if (distToPlayerX > 14.0f || distToPlayerY > 10.0f) aiState = 3;
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

            if (std::abs(realX - spawnX) < 0.5f && std::abs(realY - spawnY) < 0.5f) aiState = 0;
            if (inSight) { aiState = 1; stateTimer = 0.2f; }
        }

        float probeX = realX + moveDirection * (hbWidth / 2.0f + 0.1f);
        if (gameMap.getTileAt((int)probeX, (int)(realY + 0.5f)) == TileType::Wall) {
            if (aiState == 0 || aiState == 3) moveDirection *= -1;
            else realX -= moveDirection * crawlerSpeed * 1.8f * dt;
        }

        x = (int)std::round(realX); y = (int)std::round(realY);
        animTimer += dt;
        float frameDuration = (aiState == 2) ? 0.06f : 0.10f;
        if (animTimer >= frameDuration) { currentFrame = (currentFrame + 1) % totalFrames; animTimer = 0.0f; }
    }
    // ==========================================
    // 🪲 爬虫逻辑 (Type 0) - 严密防跌落机制
    // ==========================================
    else if (enemyType == 0) {
        float bottomY = realY + 1.0f;
        int gridX = (int)(realX + 0.5f);
        TileType underTile = gameMap.getTileAt(gridX, (int)(bottomY + 0.05f));

        if (underTile == TileType::Wall || underTile == TileType::Platform) {
            realY = std::floor(bottomY) - 1.0f;
        }
        else realY += 12.0f * dt;

        bool inSight = (distToPlayerX <= 6.0f && distToPlayerY <= 2.0f);
        float currentSpeed = 0.0f;

        if (aiState == 0) {
            currentSpeed = crawlerSpeed;
            if (inSight) { aiState = 1; stateTimer = 0.5f; moveDirection = (player.getRealX() > realX) ? 1 : -1; }
            else {
                if (moveDirection == 1 && realX >= spawnX + 3.0f) moveDirection = -1;
                else if (moveDirection == -1 && realX <= spawnX - 3.0f) moveDirection = 1;
            }
        }
        else if (aiState == 1) { currentSpeed = 0.0f; stateTimer -= dt; if (stateTimer <= 0.0f) aiState = 2; }
        else if (aiState == 2) { currentSpeed = crawlerSpeed * 2.5f; moveDirection = (player.getRealX() > realX) ? 1 : -1; if (!inSight) aiState = 0; }
        else if (aiState == 3) { currentSpeed = crawlerSpeed * 0.5f; stateTimer -= dt; if (stateTimer <= 0.0f) aiState = 0; }

        float probeX = realX + moveDirection * (hbWidth / 2.0f + 0.2f);
        int probeGridX = (int)probeX;
        int probeGridY = (int)(realY + 0.5f);
        int probeFloorY = (int)(realY + 1.1f);

        TileType nextWall = gameMap.getTileAt(probeGridX, probeGridY);
        TileType nextFloor = gameMap.getTileAt(probeGridX, probeFloorY);
        bool isCliffAhead = (nextFloor != TileType::Wall && nextFloor != TileType::Platform);

        if (nextWall == TileType::Wall || nextFloor == TileType::Void || nextFloor == TileType::SpikeUp || isCliffAhead) {
            moveDirection *= -1;
            realX += (moveDirection == 1 ? 0.08f : -0.08f);
            if (aiState == 2) { aiState = 3; stateTimer = 1.5f; }
        }
        else realX += moveDirection * currentSpeed * dt;

        x = (int)std::round(realX); y = (int)std::round(realY);
        animTimer += dt;
        float frameDuration = (aiState == 2) ? 0.06f : 0.15f;
        if (aiState == 1) frameDuration = 0.04f;
        if (animTimer >= frameDuration) { currentFrame = (currentFrame + 1) % totalFrames; animTimer = 0.0f; }
    }
    // ==========================================
    // 🛡️ 假骑士 Boss AI 终极破局状态机 (Type 2)
    // ==========================================
    else if (enemyType == 2) {
        // 🌟 1. 独立更新贴地滚动的冲击波
        for (auto it = shockwaves.begin(); it != shockwaves.end(); ) {
            it->x += (it->speed * it->direction) * dt;
            it->lifeTimer -= dt;

            // 探测上方空气层避免刚出生误触地表自融
            if (it->lifeTimer <= 0 || gameMap.getTileAt((int)it->x, (int)(it->y - 0.5f)) == TileType::Wall) {
                it = shockwaves.erase(it);
            }
            else {
                if (it->getHitbox().intersects(player.getHitbox())) {
                    player.takeDamage(25, (int)it->x, gameMap);
                }
                ++it;
            }
        }

        // 🌟 2. 真实物理重力抛物线驱动
        float& bossVy = animTimer; // 借用闲置变量作为垂直速度中枢
        bossVy += 45.0f * dt;
        if (bossVy > 25.0f) bossVy = 25.0f;

        float nextY = realY + bossVy * dt;
        int gridX = (int)std::round(realX);
        TileType underFoot = gameMap.getTileAt(gridX, (int)(nextY + 0.05f));
        bool isBossGrounded = (underFoot == TileType::Wall || underFoot == TileType::Platform);

        // 落地着陆处理
        if (bossVy > 0.0f && isBossGrounded) {
            realY = std::floor(nextY);
            bossVy = 0.0f;

            // 捕获大跳落地信号
            if (bossState == 2) {
                flickerTimer = 10; // 落地强力后坐力触发大震屏

                // 连招链：只有后撤大跳 (aiState == 1) 落地才接大招放冲击波
                if (aiState == 1) {
                    bossState = 3; // 切入举锤前摇
                    stateTimer = 0.5f;
                    maceSwingDir = (player.getRealX() > realX) ? 1 : -1;
                }
                else {
                    // 常规向前大跳压杀落地 -> 给玩家提供极佳的背刺反击硬直窗口
                    bossState = 0;
                    stateTimer = 1.0f;
                }
            }
        }
        else {
            realY = nextY;
        }

        if (hp < maxHp / 2 && !isEnraged) {
            isEnraged = true;
        }

        // 🌟 3. 高智商拉扯与换位核心 AI
        int facingToPlayer = (player.getRealX() > realX) ? 1 : -1;

        if (bossState == 0) { // 🚶 威压逼近博弈 (boss_stand)
            stateTimer -= dt;
            moveDirection = facingToPlayer;

            // 距离较远时沉重迈步压迫
            if (distToPlayerX > 3.5f) {
                float targetWalkX = realX + facingToPlayer * (isEnraged ? 4.5f : 3.0f) * dt;
                if (gameMap.getTileAt((int)(targetWalkX + facingToPlayer * 1.5f), (int)(realY - 0.5f)) != TileType::Wall) {
                    realX = targetWalkX;
                }
            }

            if (stateTimer <= 0.0f) {
                // 🌟【绝密修复：雷达视距暴增】：大跳空中滞空长达0.8秒，覆盖足足 11~14 格！
                // 必须严格检查背后 12 格之外是否有实心墙壁阻挡
                float checkBehindX = realX - facingToPlayer * 12.0f;
                bool spaceBehind = (checkBehindX > 1.0f && checkBehindX < (gameMap.getWidth() - 2.0f)) &&
                    (gameMap.getTileAt((int)checkBehindX, (int)(realY - 0.5f)) != TileType::Wall);

                // 只有身后跑道足够长，且玩家贴脸时，才允许后撤大跳
                if (distToPlayerX < 6.5f && spaceBehind && GetRandomValue(0, 10) <= 6) {
                    bossState = 1;
                    stateTimer = 0.35f;
                    aiState = 1;        // 标记为后撤大招跳
                }
                else {
                    // 🌟 墙角破局：背后没空间被逼进死角时，优先大跳跨越玩家头顶（换位压杀）！
                    // 彻底摆脱被卡死在边界摩擦的尴尬局面
                    if (distToPlayerX > 4.5f && GetRandomValue(0, 2) == 0) {
                        bossState = 3; // 偶尔选择原地直接挥锤砸波
                        stateTimer = 0.6f;
                        maceSwingDir = facingToPlayer;
                    }
                    else {
                        bossState = 1;      // 准备刚猛向前起跳
                        stateTimer = 0.35f;
                        aiState = 0;        // 标记为常规向前换位大跳
                    }
                }
            }
        }
        else if (bossState == 1) { // 🧎 下蹲蓄力准备起飞 (boss_jump)
            stateTimer -= dt;
            moveDirection = facingToPlayer;
            if (stateTimer <= 0.0f) {
                bossState = 2;
                bossVy = -18.5f; // 刚猛起跳初速

                // 空行动能指派：后撤跳反向，换位跳正向
                moveDirection = (aiState == 1) ? -facingToPlayer : facingToPlayer;
            }
        }
        else if (bossState == 2) { // 🦅 掠空压杀 (boss_air)
            float targetAirX = realX + moveDirection * (isEnraged ? 18.0f : 14.0f) * dt;
            // 严防穿墙
            if (gameMap.getTileAt((int)(targetAirX + moveDirection * 1.5f), (int)(realY - 1.0f)) != TileType::Wall) {
                realX = targetAirX;
            }
            else {
                moveDirection = 0; // 撞墙立刻空中急刹停车
            }
        }
        else if (bossState == 3) { // 锤 举锤后仰前摇 (boss_swing)
            stateTimer -= dt;
            moveDirection = maceSwingDir;

            if (stateTimer <= 0.0f) {
                bossState = 4;
                stateTimer = 0.5f;
                flickerTimer = 12; // 深度颤动触发主程序大震屏

                Shockwave wave;
                wave.x = realX + maceSwingDir * 1.5f;
                wave.y = realY;
                wave.speed = isEnraged ? 15.0f : 11.0f;
                wave.direction = maceSwingDir;
                wave.lifeTimer = 3.5f;
                wave.height = 1.8f; // 死死锁定 1.8 格峰值
                shockwaves.push_back(wave);
            }
        }
        else if (bossState == 4) { // 💥 砸地大后摇 (boss_hit)
            stateTimer -= dt;
            moveDirection = maceSwingDir;
            if (stateTimer <= 0.0f) {
                bossState = 0;
                stateTimer = 1.0f; // 恢复思考中枢
            }
        }

        x = (int)std::round(realX); y = (int)std::round(realY);
    }

    // 实体碰撞判定
    if (getHitbox().intersects(player.getHitbox()) && attackCooldown <= 0) {
        player.takeDamage(baseDamage, x, gameMap);
        attackCooldown = 30;
    }
}