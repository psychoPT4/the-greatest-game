#pragma execution_character_set("utf-8")
#include "Player.h"
#include "Enemy.h"
#include <cmath>
#include <algorithm>

extern std::string combatLog;
Player::Player(int startX, int startY)
    : name("Knight"), x(startX), y(startY), realX((float)startX), realY((float)startY),
    hbWidth(0.5f), hbHeight(0.8f), hp(100), maxHp(100), baseDamage(25),
    isGrounded(false), velocityX(0.0f), velocityY(0.0f), facingDirection(1),
    alive(true), flickerTimer(0),
    level(1), currentExp(0), expToNextLevel(100), mana(0),maxMana(99),jumpCount(0), moveIntent(0),
    maxRunSpeed(12.0f), runAccel(120.0f), groundFriction(160.0f), airDrag(80.0f),
    gravity(85.0f), jumpForce(-26.0f), maxFallSpeed(30.0f),
    isDashing(false), dashSpeed(40.0f), dashDuration(0.2f), dashTimer(0.0f),
    dashCooldown(0.6f), dashCooldownTimer(0.0f), ignorePlatformTimer(0.0f), dashDirection(1), isRunningMode(false) {
    stamina = 100.0f;
    maxStamina = 100.0f;
}

void Player::setRealPos(float nx, float ny) {
    realX = nx; realY = ny;
    x = (int)std::round(realX); y = (int)std::round(realY);
    velocityX = 0.0f; velocityY = 0.0f;
    flickerTimer = 30;
}

void Player::takeDamage(int damage, int sourceX, const Map& gameMap) {
    // 1. 基础拦截：死了或者正在无敌闪烁，直接无视
    if (!alive || flickerTimer > 0) return;

    // 2. 【状态机核心修复】：强行打断冲刺！
    // 动作游戏铁律：一旦挨打（哪怕你原本想设定冲刺无敌，但在地刺这种绝对伤害面前也必须破防），必须强行解除冲刺状态！
    if (isDashing) {
        isDashing = false;
        combatLog = "冲刺被打断！ ";
    }

    // 3. 扣血与赋予无敌帧 (保留你原本的 50 帧)
    hp -= damage;
    flickerTimer = 50;

    // 4. 【新增】：受击向上弹起 (Hurt Bounce)
    isGrounded = false;
    velocityY = -8.0f; // 给一个向上的初速度，把骑士扎飞！

    // 5. 【保留你原本的】：X 轴微小击退防穿墙逻辑
    int kDir = (x > sourceX) ? 1 : -1;
    float targetX = realX + kDir * 1.0f; // 这里的 1.0f 是击退距离

    if (gameMap.getTileAt((int)std::round(targetX), y) != TileType::Wall) {
        realX = targetX;
        x = (int)std::round(realX);
    }

    // 6. 【保留你原本的】：死亡判定结算
    if (hp <= 0) {
        hp = 0;
        alive = false;
        combatLog = name + " 已倒下！ ";
    }
}

void Player::setMoveIntent(int dir) {
    moveIntent = dir;
    if (dir != 0) facingDirection = dir;
}

void Player::processJump(bool jumpPressed, bool jumpHeld, bool downHeld) {
    if (jumpPressed && (isGrounded || jumpCount < 2)) {
        if (stamina >= 15.0f) {
            stamina -= 15.0f;
            velocityY = jumpForce;
            jumpCount++;
            isGrounded = false;
        }
    }
    if (jumpPressed) {
        // 【新增机制】：按下跳跃键时，如果按住了 S 键且在地面上，触发下落
        if (downHeld && isGrounded) {
            ignorePlatformTimer = 0.2f; // 给予 0.2 秒的“穿透时间”
            isGrounded = false;
            velocityY = 10.0f; // 给一个微小的向下初速度，瞬间打破吸附
            return; // 直接返回，不要再触发常规跳跃了
        }

        if (isGrounded) { velocityY = jumpForce; isGrounded = false; jumpCount = 1; }
        else if (jumpCount < 2) { velocityY = jumpForce * 0.85f; jumpCount = 2; combatLog = "二段跳！ "; }
    }
    if (!jumpHeld && velocityY < 0) velocityY *= 0.5f;
}

bool Player::tryDash() {
    // 严密防线：体力必须够 30，不能已经在冲刺中，且必须冷却完毕！
    if (stamina >= 30.0f && !isDashing && dashCooldownTimer <= 0.0f) {

        // 1. 经济系统扣除
        stamina -= 30.0f;

        // 2. 状态机切换 (这就是原本 startDash 干的活，现在完美融合进来)
        isDashing = true;
        dashTimer = dashDuration;         // 重置冲刺持续时间
        dashCooldownTimer = dashCooldown; // 重置冲刺 CD
        dashDirection = facingDirection;  // 瞬间锁死冲刺方向

        return true; // 告诉 main 函数：冲刺成功，可以播放音效了！
    }

    return false; // 体力不够或 CD 没转好，拒绝冲刺
}

void Player::update(const Map& gameMap, float dt) {
    if (!alive) return;
    if (dashCooldownTimer > 0.0f) dashCooldownTimer -= dt;
    if (ignorePlatformTimer > 0.0f) ignorePlatformTimer -= dt;
    if (flickerTimer > 0) flickerTimer--;

    // 1. 物理速度计算
    if (isDashing) {
        dashTimer -= dt;
        if (dashTimer <= 0.0f) { isDashing = false; velocityX = 0.0f; }
        else { velocityX = dashDirection * dashSpeed; velocityY = 0.0f; }
    }
    else {
        float currentFriction = isGrounded ? groundFriction : airDrag;
        float currentMaxSpeed = isRunningMode ? maxRunSpeed : (maxRunSpeed * 0.6f);
        if (moveIntent != 0) {
            velocityX += runAccel * moveIntent * dt;
            if (velocityX > currentMaxSpeed) velocityX = currentMaxSpeed;
            if (velocityX < -currentMaxSpeed) velocityX = -currentMaxSpeed;
        }
        else {
            if (velocityX > 0) { velocityX -= currentFriction * dt; if (velocityX < 0) velocityX = 0; }
            else if (velocityX < 0) { velocityX += currentFriction * dt; if (velocityX > 0) velocityX = 0; }
        }
        // 【核心修复一】：永远应用重力，无论状态！
        velocityY += gravity * dt;
        if (velocityY > maxFallSpeed) velocityY = maxFallSpeed;
    }
    if (isDashing) {
        // 冲刺中不恢复
    }
    else if (isRunningMode && isGrounded && (IsKeyDown(KEY_A) || IsKeyDown(KEY_D))) {
        // 跑步时持续消耗！(比如每秒耗 15 点)
        stamina -= 15.0f * dt;
        if (stamina < 0.0f) stamina = 0.0f;
    }
    else {
        // 没跑没冲，缓慢恢复 (比如每秒回 25 点)
        stamina += 25.0f * dt;
        if (stamina > maxStamina) stamina = maxStamina;
    }
    // 2. X轴碰撞判定
    float nextRealX = realX + velocityX * dt;
    float boxLeft = nextRealX + (1.0f - hbWidth) / 2.0f;
    float boxRight = nextRealX + (1.0f + hbWidth) / 2.0f;
    float boxTop = realY + 1.0f - hbHeight + 0.1f;
    float boxBottom = realY + 0.9f;

    if (velocityX > 0) {
        if (gameMap.getTileAt((int)boxRight, (int)boxTop) == TileType::Wall || gameMap.getTileAt((int)boxRight, (int)boxBottom) == TileType::Wall) {
            nextRealX = std::floor(boxRight) - (1.0f + hbWidth) / 2.0f - 0.01f; velocityX = 0.0f;
        }
    }
    else if (velocityX < 0) {
        if (gameMap.getTileAt((int)boxLeft, (int)boxTop) == TileType::Wall || gameMap.getTileAt((int)boxLeft, (int)boxBottom) == TileType::Wall) {
            nextRealX = std::floor(boxLeft) + 1.0f - (1.0f - hbWidth) / 2.0f + 0.01f; velocityX = 0.0f;
        }
    }
    realX = nextRealX;

    // 3. Y轴碰撞与精确接地判定
    float nextRealY = realY + velocityY * dt;
    Hitbox box = getHitbox();
    float inset = 0.05f;
    float pLeft = box.left + inset;
    float pRight = box.right - inset;

    isGrounded = false; // 每帧默认清空，由下面物理严格证明

    // 【核心修复二】：检测向下碰撞时，必须包含 velocityY >= 0 (即静止站立状态也要测脚底！)
    if (velocityY >= 0.0f) {
        int currentBottomTile = (int)(realY + 1.0f);
        int nextBottomTile = (int)(nextRealY + 1.0f);
        bool hitGround = false; int groundTileY = nextBottomTile;

        for (int ty = currentBottomTile; ty <= nextBottomTile; ++ty) {
            TileType leftFoot = gameMap.getTileAt((int)pLeft, ty);
            TileType rightFoot = gameMap.getTileAt((int)pRight, ty);

            // 【核心逻辑】：实体墙永远会挡住你
            if (leftFoot == TileType::Wall || rightFoot == TileType::Wall) {
                hitGround = true; groundTileY = ty; break;
            }
            // 【核心逻辑】：只有在“非穿透”状态下，平台才会被视为地面
            if (ignorePlatformTimer <= 0.0f) {
                if (leftFoot == TileType::Platform || rightFoot == TileType::Platform) {
                    hitGround = true; groundTileY = ty; break;
                }
            }
        }
        if (hitGround) {
            nextRealY = (float)groundTileY - 1.0f;
            velocityY = 0.0f; isGrounded = true; jumpCount = 0;
        }
    }
    else if (velocityY < 0) {
        int currentTopTile = (int)(realY + 1.0f - hbHeight);
        int nextTopTile = (int)(nextRealY + 1.0f - hbHeight);
        bool hitCeil = false; int ceilTileY = nextTopTile;

        for (int ty = currentTopTile; ty >= nextTopTile; --ty) {
            TileType leftHead = gameMap.getTileAt((int)pLeft, ty);
            TileType rightHead = gameMap.getTileAt((int)pRight, ty);
            if (leftHead == TileType::Wall || rightHead == TileType::Wall) { hitCeil = true; ceilTileY = ty; break; }
        }
        if (hitCeil) { nextRealY = (float)ceilTileY + 1.0f - (1.0f - hbHeight) + 0.01f; velocityY = 0.0f; }
    }
    realY = nextRealY; x = (int)std::round(realX); y = (int)std::round(realY);
}

AttackResult Player::attack(std::vector<Enemy>& enemies, const Map& gameMap, bool downPressed) {
    AttackResult res;
    Hitbox myBox = getHitbox();

    if (!isGrounded && downPressed) {
        Hitbox pogoBox = { myBox.left, myBox.right, myBox.bottom, myBox.bottom + 1.5f };
        for (auto& target : enemies) {
            if (!target.isAlive()) continue;
            if (pogoBox.intersects(target.getHitbox())) {
                target.takeDamage(baseDamage, x, gameMap);
                res.pogoSuccess = true; res.hitSomething = true;
                addMana(11);
                combatLog = "【下劈弹刀！】命中目标 ";
                if (!target.isAlive()) res.totalXp += target.getExpReward();
            }
        }
        TileType leftFoot = gameMap.getTileAt((int)pogoBox.left, (int)pogoBox.bottom);
        TileType rightFoot = gameMap.getTileAt((int)pogoBox.right, (int)pogoBox.bottom);
        if (leftFoot == TileType::SpikeUp || rightFoot == TileType::SpikeUp) { res.pogoSuccess = true; combatLog = "【下劈弹刀！】借助地刺弹起 "; }
        if (res.pogoSuccess) { velocityY = jumpForce * 0.9f; jumpCount = 1; }
    }
    else {
        Hitbox slashBox;
        if (facingDirection == 1) slashBox = { myBox.right, myBox.right + 1.5f, myBox.top, myBox.bottom };
        else slashBox = { myBox.left - 1.5f, myBox.left, myBox.top, myBox.bottom };

        bool hitSomething = false;
        for (auto& target : enemies) {
            if (!target.isAlive()) continue;
            if (slashBox.intersects(target.getHitbox())) {
                target.takeDamage(baseDamage, x, gameMap);
                res.hitSomething = true; hitSomething = true; combatLog = "发起平砍 ";
                addMana(11);
                if (!target.isAlive()) res.totalXp += target.getExpReward();
            }
        }
        if (!hitSomething) combatLog = "挥空了... ";
    }
    return res;
}

void Player::addExp(int amount) {
    currentExp += amount;
    while (currentExp >= expToNextLevel) {
        level++; currentExp -= expToNextLevel; expToNextLevel = (int)(expToNextLevel * 1.5);
        maxHp += 20; hp = maxHp; baseDamage += 5;
        combatLog = "★★★ 升级！当前等级:  " + std::to_string(level) + " ★★★ ";
    }
}
MagicAction Player::processMagic(bool magicHeld, bool magicReleased, float dt) {
    MagicAction action = MAGIC_NONE;
    if (healFlashTimer > 0.0f) healFlashTimer -= dt;

    // 凝聚铁律：必须站在地上，没有移动，没有冲刺，且没有在挨打
    if (magicHeld && isGrounded && velocityX == 0.0f && !isDashing && flickerTimer <= 0) {
        isFocusing = true;
        focusTimer += dt;

        // 蓄力满 1.0 秒，消耗 33 灵魂，回复 20 血量！
        if (focusTimer >= 1.0f) {
            if (hp < maxHp && consumeMana(33)) {
                hp += 20;
                if (hp > maxHp) hp = maxHp;
                healFlashTimer = 0.3f; // 引爆 0.3 秒的白光特效
                action = HEALED;
            }
            focusTimer = 0.0f; // 重置，允许连续按住持续回血
        }
    }
    else {
        // 如果松开按键，且蓄力时间很短（< 0.25秒），判定为“轻按开火”！
        if (isFocusing && magicReleased && focusTimer > 0.0f && focusTimer < 0.25f) {
            if (consumeMana(33)) {
                Projectile p;
                p.x = realX;
                p.y = realY + 0.3f;

                // 🌟 【核心修复】：直接用自带的 facingDirection！绝不向外求！
                p.facingDir = facingDirection;

                p.speed = 18.0f;
                p.lifeTimer = 0.6f;
                p.active = true;
                projectiles.push_back(p);
                action = CAST_SPELL;
            }
        }
        isFocusing = false;
        focusTimer = 0.0f;
    }
    return action;
}

// 在 Player.cpp 中处理自己发出的法术的飞行与碰撞
void Player::updateProjectiles(std::vector<Enemy>& enemies, const Map& gameMap, float dt) {
    for (auto it = projectiles.begin(); it != projectiles.end(); ) {
        it->x += (it->speed * it->facingDir) * dt;
        it->lifeTimer -= dt;

        for (auto& target : enemies) {
            if (target.isAlive() && it->getHitbox().intersects(target.getHitbox())) {
                if (!target.isFlickering()) {
                    target.takeDamage(40, it->x, gameMap);
                    // 这里你可以利用外部传入的粒子数组或者回调来生成火花，或者仅造成伤害
                }
            }
        }

        // 撞墙或寿命终结
        TileType frontTile = gameMap.getTileAt(it->x + (it->facingDir == 1 ? 1.0f : 0.0f), it->y);
        if (frontTile == TileType::Wall || it->lifeTimer <= 0) {
            it = projectiles.erase(it);
        }
        else {
            ++it;
        }
    }
}