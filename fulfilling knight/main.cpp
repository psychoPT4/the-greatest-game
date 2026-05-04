#pragma execution_character_set("utf-8")
#include <iostream>
#include <string>
#include <vector>
#include <math.h>
#include "map.h"
#include "Player.h"
#include "Enemy.h"
#include "Camera.h"
#include "raylib.h" 

using namespace std;

enum GameState { MENU, PLAYING, GAME_OVER };
GameState currentState = MENU;

// 菜单孢子粒子
struct SporeParticle {
    Vector2 pos;
    Vector2 velocity;
    float radius;
    unsigned char alpha;
};

// 【新增】战斗受击火花粒子
struct HitParticle {
    Vector2 pos;       // 物理逻辑坐标
    Vector2 velocity;  // 弹射速度
    float life;        // 当前剩余寿命
    float maxLife;     // 初始总寿命
    Color color;       // 粒子颜色
    float size;        // 粒子大小
};
struct DashTrail {
    Texture2D tex;      // 记录残影生成时，主角用的是哪张贴图
    int frameIndex;     // 记录当时切到了第几帧
    int totalFrames;    // 总帧数
    float worldX;       // 必须记录世界坐标，否则残影会跟着屏幕一起跑！
    float worldY;
    bool facingLeft;    // 记录当时的朝向
    float life;         // 剩余寿命
    float maxLife;      // 初始总寿命
};

string combatLog = "Adventure continues...";
float safeX, safeY;
const int TILE_SIZE = 48;

static void DrawTexExact(Texture2D tex, float x, float y, float w, float h, Color tint = WHITE, bool flipX = false) {
    Rectangle src = { 0, 0, flipX ? -(float)tex.width : (float)tex.width, (float)tex.height };
    Rectangle dest = { x, y, w, h };
    DrawTexturePro(tex, src, dest, { 0, 0 }, 0.0f, tint);
}

static void DrawSpriteFrame(Texture2D tex, int currentFrame, int totalFrames, float x, float y, float w, float h, Color tint = WHITE, bool flipX = false) {
    float frameWidth = (float)tex.width / totalFrames;
    Rectangle src = { currentFrame * frameWidth, 0, flipX ? -frameWidth : frameWidth, (float)tex.height };
    Rectangle dest = { x, y, w, h };
    DrawTexturePro(tex, src, dest, { 0, 0 }, 0.0f, tint);
}

int main() {
    const int screenWidth = 1280;
    const int screenHeight = 720;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "Fulfilling Knight - Graphic Era");
    SetConfigFlags(FLAG_VSYNC_HINT); // 【顺手修复一闪一闪】：开启垂直同步，强行锁帧对齐显示器刷新率！
    SetTargetFPS(60);

    // 【新增】：创建 1280x720 的绝对虚拟画布
    RenderTexture2D target = LoadRenderTexture(1280, 720);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT); // 保持像素画锐利
    // ==========================================
    // 资源加载区
    // ==========================================
    int codepointCount = (127 - 32) + (0x9FA5 - 0x4E00 + 1);
    int* codepoints = new int[codepointCount];
    int index = 0;
    for (int i = 32; i < 127; i++) codepoints[index++] = i;
    for (int i = 0x4E00; i <= 0x9FA5; i++) codepoints[index++] = i;
    Font myFont = LoadFontEx("font.ttf", 32, codepoints, codepointCount);
    delete[] codepoints;

    Texture2D bgTex = LoadTexture("bg.png");
    Texture2D texStand = LoadTexture("knight_stand.png");
    Texture2D texWalk = LoadTexture("knight_walk.png");
    Texture2D texRun = LoadTexture("knight_run.png");
    Texture2D texAttack = LoadTexture("knight_attack.png"); // 攻击序列帧
    Texture2D crawlerTex = LoadTexture("crawler.png");
    Texture2D wallTex = LoadTexture("wall.png");
    Texture2D platformTex = LoadTexture("platform.png");
    Texture2D spikeTex = LoadTexture("spike.png");
    Texture2D texJumpUp = LoadTexture("knight_jump_up.png");
    Texture2D texJumpMid = LoadTexture("knight_jump_mid.png");
    Texture2D texFall = LoadTexture("knight_fall.png");
    Texture2D texLand = LoadTexture("knight_land.png");
    Texture2D texPogo = LoadTexture("knight_pogo.png");

    int currentLevel = 1;
    bool mapLoaded = false;
    GameCamera cam(screenWidth / TILE_SIZE, screenHeight / TILE_SIZE);

    Player player(0, 0);
    Map gameMap(100, 30);
    vector<Enemy> enemies;
    static int attackCd = 0;

    Rectangle btnStart = { screenWidth / 2.0f - 100, screenHeight / 2.0f, 200, 50 };
    Rectangle btnSettings = { screenWidth / 2.0f - 100, screenHeight / 2.0f + 70, 200, 50 };
    Rectangle btnExit = { screenWidth / 2.0f - 100, screenHeight / 2.0f + 140, 200, 50 };

    // ==========================================
    // 物理系统与粒子弹匣
    // ==========================================
    vector<SporeParticle> particles;
    for (int i = 0; i < 150; i++) {
        particles.push_back({
            { (float)GetRandomValue(0, screenWidth), (float)GetRandomValue(0, screenHeight) },
            { (float)GetRandomValue(-15, 15) / 10.0f, (float)GetRandomValue(-30, -5) / 10.0f },
            (float)GetRandomValue(1, 4),
            (unsigned char)GetRandomValue(50, 200)
            });
    }
    vector<DashTrail> dashTrails;
    vector<HitParticle> hitParticles;
    float hitStopTimer = 0.0f;
    float camShakeX = 0.0f;
    float camShakeY = 0.0f;

    // 独立攻击状态机变量
    bool isAttacking = false;
    bool isPogoAttacking = false; // 【新增】：独立记录是否正在下劈
    int attackFrame = 0;
    float attackAnimTimer = 0.0f;
    bool damageDealtThisSwing = false;
    const int ATTACK_TOTAL_FRAMES = 5;
    const int POGO_TOTAL_FRAMES = 2;   // 【新增】：下劈 2 帧
    const float ATTACK_SPEED = 0.05f;

    // ==========================================
    // 游戏主循环
    // ==========================================
    while (!WindowShouldClose()) {

        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;

        if (currentState == MENU) {
            Vector2 mousePos = GetMousePosition();
            for (auto& p : particles) {
                p.pos.x += p.velocity.x; p.pos.y += p.velocity.y;
                float dx = p.pos.x - mousePos.x; float dy = p.pos.y - mousePos.y;
                float dist = sqrt(dx * dx + dy * dy);
                if (dist < 120.0f && dist > 0.1f) {
                    float force = (120.0f - dist) / 120.0f;
                    p.pos.x += (dx / dist) * force * 5.0f; p.pos.y += (dy / dist) * force * 5.0f;
                }
                if (p.pos.y < -10) p.pos.y = screenHeight + 10;
                if (p.pos.x < -10) p.pos.x = screenWidth + 10;
                if (p.pos.x > screenWidth + 10) p.pos.x = -10;
            }

            if (CheckCollisionPointRec(mousePos, btnStart) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                currentState = PLAYING; mapLoaded = false;
            }
            if (CheckCollisionPointRec(mousePos, btnExit) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                break;
            }
        }
        else if (currentState == PLAYING) {
            if (!mapLoaded) {
                if (!gameMap.loadFromCSV("map" + to_string(currentLevel) + ".csv")) break;
                SpawnPoint pSpawn = gameMap.getPlayerSpawn();
                player.setRealPos((float)pSpawn.x, (float)pSpawn.y);
                safeX = (float)pSpawn.x; safeY = (float)pSpawn.y;

                enemies.clear();
                for (const auto& s : gameMap.getEnemySpawns()) {
                    if (s.type == 8) enemies.emplace_back(s.x, s.y, 40);
                }
                mapLoaded = true;
            }

            if (IsKeyPressed(KEY_Q)) break;
            if (IsKeyPressed(KEY_L)) player.startDash();

            int moveDir = (IsKeyDown(KEY_D) - IsKeyDown(KEY_A));
            player.setMoveIntent(moveDir);
            player.setRunningMode(IsKeyDown(KEY_LEFT_SHIFT));

            bool jump = IsKeyDown(KEY_K);
            static bool lastJump = false;
            player.processJump(jump && !lastJump, jump, IsKeyDown(KEY_S));
            lastJump = jump;
// 1. 触发攻击意图（平砍与下劈分流）
            if (IsKeyDown(KEY_J) && !isAttacking && !isPogoAttacking && attackCd <= 0 && !player.getIsDashing()) {
                // 【核心判断】：在空中且按住了 S 键，触发下劈！
                if (!player.getIsGrounded() && IsKeyDown(KEY_S)) {
                    isPogoAttacking = true;
                }
                else {
                    isAttacking = true; // 否则触发普通平砍
                }

                attackFrame = 0;
                attackAnimTimer = 0.0f;
                damageDealtThisSwing = false;
            }

            // 2. 独立推进攻击时间轴 (兼容两种攻击)
            if (isAttacking || isPogoAttacking) {
                if (hitStopTimer <= 0.0f) {
                    attackAnimTimer += dt;
                    // 下劈只有两帧，为了看清楚动作，我们可以稍微把它的帧速度放慢一点点（比如 0.08 秒）
                    float currentAnimSpeed = isPogoAttacking ? 0.12f : ATTACK_SPEED;

                    if (attackAnimTimer >= currentAnimSpeed) {
                        attackFrame++;
                        attackAnimTimer = 0.0f;
                    }
                }

                // 【核心碰撞同步】：平砍在第 3 帧（下标 2）出刀，而下劈在第 2 帧（下标 1）出刀！
                int triggerFrame = isPogoAttacking ? 1 : 2;

                if (attackFrame == triggerFrame && !damageDealtThisSwing) {
                    AttackResult result = player.attack(enemies, gameMap, IsKeyDown(KEY_S));
                    if (result.totalXp > 0) player.addExp(result.totalXp);

                    if (result.hitSomething || result.pogoSuccess) {
                        hitStopTimer = 0.08f;

                        float hitX = player.getRealX();
                        float hitY = player.getRealY();
                        if (result.pogoSuccess) {
                            hitY += 1.0f;
                        }
                        else {
                            hitY += 0.5f;
                            hitX += IsKeyDown(KEY_A) ? -0.6f : 0.6f;
                        }

                        int sparkCount = GetRandomValue(45, 65);
                        for (int i = 0; i < sparkCount; i++) {
                            HitParticle p = {};
                            p.pos = { hitX, hitY };
                            p.velocity.x = GetRandomValue(-50, 50) / 1.0f;
                            p.velocity.y = GetRandomValue(-40, 5) / 1.0f;
                            p.maxLife = GetRandomValue(15, 35) / 100.0f;
                            p.life = p.maxLife;
                            p.size = GetRandomValue(4, 20) / 100.0f;

                            int colorRoll = GetRandomValue(0, 10);
                            if (colorRoll < 2) p.color = ORANGE;
                            else if (colorRoll < 6) p.color = YELLOW;
                            else p.color = WHITE;

                            hitParticles.push_back(p);
                        }
                    }
                    damageDealtThisSwing = true;
                }
                int maxFrames = isPogoAttacking ? POGO_TOTAL_FRAMES : ATTACK_TOTAL_FRAMES;
                if (attackFrame >= maxFrames) {
                    isAttacking = false;
                    isPogoAttacking = false;
                    attackCd = 12;
                }
            }
            else {
                if (attackCd > 0) attackCd--;
            }

            // --- 时停系统与物理接管 ---
            if (hitStopTimer > 0.0f) {
                hitStopTimer -= dt;
                camShakeX = GetRandomValue(-15, 15) / 100.0f;
                camShakeY = GetRandomValue(-15, 15) / 100.0f;
            }
            else {
                camShakeX = 0.0f;
                camShakeY = 0.0f;

                player.update(gameMap, dt);

                if (gameMap.getTileAt(player.getX(), player.getY() + 1) == TileType::Wall) {
                    safeX = player.getRealX(); safeY = player.getRealY();
                }

                for (auto& e : enemies) e.update(gameMap, player, dt);

                for (auto it = hitParticles.begin(); it != hitParticles.end(); ) {
                    it->velocity.y += 40.0f * dt;
                    it->pos.x += it->velocity.x * dt;
                    it->pos.y += it->velocity.y * dt;
                    it->life -= dt;
                    if (it->life <= 0) it = hitParticles.erase(it);
                    else ++it;
                }
                // 【就是这里！】：把截图里那段残影衰减贴在下面
                for (auto it = dashTrails.begin(); it != dashTrails.end(); ) {
                    it->life -= dt;
                    if (it->life <= 0) it = dashTrails.erase(it);
                    else ++it;
                }
            }

            Hitbox pBox = player.getHitbox();
            TileType footL = gameMap.getTileAt((int)pBox.left, (int)pBox.bottom);
            TileType footR = gameMap.getTileAt((int)pBox.right, (int)pBox.bottom);

            if (footL == TileType::Void || footR == TileType::Void || player.getY() >= 29) {
                if (!player.isFlickering()) {
                    player.takeDamage(player.getMaxHp() / 4, player.getX(), gameMap);
                    player.setRealPos(safeX, safeY);
                    combatLog = "Fell into the void!";
                }
            }
            else if (footL == TileType::SpikeUp || footR == TileType::SpikeUp) {
                if (!player.isFlickering()) {
                    player.takeDamage(20, player.getX(), gameMap); // 扣血并产生无敌帧
                    combatLog = "跌入地刺！ ";
                    hitStopTimer = 0.1f;
                    camShakeX = 0.2f;
                    camShakeY = 0.2f;
                }
            }

            cam.update(player.getRealX(), player.getRealY(), 100, 30);

            SpawnPoint goal = gameMap.getGoalPoint();
            if (player.getX() == goal.x && player.getY() == goal.y) {
                currentLevel++;
                mapLoaded = false;
            }

            if (!player.isAlive()) {
                currentState = GAME_OVER;
            }
        }

        // ==========================================
        // 渲染绘制层
        // ==========================================
        BeginTextureMode(target);
        ClearBackground(BLACK);
        if (currentState == MENU) {
            Vector2 mousePos = GetMousePosition();
            float parallaxX = (screenWidth / 2.0f - mousePos.x) * 0.03f;
            float parallaxY = (screenHeight / 2.0f - mousePos.y) * 0.03f;

            if (bgTex.id != 0) {
                float scaleX = (screenWidth + 100.0f) / bgTex.width;
                float scaleY = (screenHeight + 100.0f) / bgTex.height;
                float scale = (scaleX > scaleY) ? scaleX : scaleY;
                DrawTextureEx(bgTex, { -50.0f + parallaxX, -50.0f + parallaxY }, 0.0f, scale, WHITE);
            }
            else {
                Color topColor = { (unsigned char)(20 + parallaxX * 0.5), 20, 30, 255 };
                Color botColor = { 5, 5, (unsigned char)(15 + parallaxY * 0.5), 255 };
                DrawRectangleGradientV(0, 0, screenWidth, screenHeight, topColor, botColor);
            }

            DrawTextEx(myFont, "Fulfilling Knight", { (float)(screenWidth / 2 - 220), (float)(screenHeight / 2 - 150) }, 60, 1.0f, LIGHTGRAY);

            for (const auto& p : particles) {
                Color sporeColor = { 135, 206, 235, p.alpha };
                DrawCircleV(p.pos, p.radius, sporeColor);
            }

            bool hoverStart = CheckCollisionPointRec(mousePos, btnStart);
            DrawRectangleRec(btnStart, hoverStart ? DARKGRAY : BLANK);
            DrawRectangleLinesEx(btnStart, 2, hoverStart ? WHITE : GRAY);
            DrawTextEx(myFont, "START GAME", { (float)(btnStart.x + 40), (float)(btnStart.y + 15) }, 20, 1.0f, hoverStart ? WHITE : GRAY);

            bool hoverSettings = CheckCollisionPointRec(mousePos, btnSettings);
            DrawRectangleRec(btnSettings, hoverSettings ? DARKGRAY : BLANK);
            DrawRectangleLinesEx(btnSettings, 2, hoverSettings ? WHITE : GRAY);
            DrawTextEx(myFont, "SETTINGS", { (float)(btnSettings.x + 55), (float)(btnSettings.y + 15) }, 20, 1.0f, hoverSettings ? WHITE : GRAY);

            bool hoverExit = CheckCollisionPointRec(mousePos, btnExit);
            DrawRectangleRec(btnExit, hoverExit ? DARKGRAY : BLANK);
            DrawRectangleLinesEx(btnExit, 2, hoverExit ? WHITE : GRAY);
            DrawTextEx(myFont, "EXIT", { (float)(btnExit.x + 80), (float)(btnExit.y + 15) }, 20, 1.0f, hoverExit ? WHITE : GRAY);
        }
        else if (currentState == PLAYING) {
            if (bgTex.id != 0) {
                float scale = (float)screenWidth / bgTex.width;
                DrawTextureEx(bgTex, { -cam.x * 2.0f, -cam.y * 2.0f }, 0.0f, scale, DARKGRAY);
            }

            int viewW = screenWidth / TILE_SIZE + 2;
            int viewH = screenHeight / TILE_SIZE + 2;

            for (int sy = 0; sy < viewH; ++sy) {
                for (int sx = 0; sx < viewW; ++sx) {
                    int worldX = (int)cam.x + sx;
                    int worldY = (int)cam.y + sy;

                    TileType t = gameMap.getTileAt(worldX, worldY);
                    float renderX = (worldX - cam.x + camShakeX) * TILE_SIZE;
                    float renderY = (worldY - cam.y + camShakeY) * TILE_SIZE;

                    if (t == TileType::Wall) {
                        if (wallTex.id != 0) DrawTexExact(wallTex, renderX, renderY, TILE_SIZE, TILE_SIZE);
                        else DrawRectangle(renderX, renderY, TILE_SIZE, TILE_SIZE, DARKGRAY);
                    }
                    else if (t == TileType::Platform) {
                        if (platformTex.id != 0) DrawTexExact(platformTex, renderX, renderY, TILE_SIZE, TILE_SIZE / 4.0f);
                        else DrawRectangle(renderX, renderY, TILE_SIZE, TILE_SIZE / 4.0f, GRAY);
                    }
                    else if (t == TileType::SpikeUp) {
                        if (spikeTex.id != 0) DrawTexExact(spikeTex, renderX, renderY, TILE_SIZE, TILE_SIZE);
                        else DrawTriangle({ renderX + TILE_SIZE / 2.0f, renderY }, { renderX, renderY + TILE_SIZE }, { renderX + TILE_SIZE, renderY + TILE_SIZE }, RED);
                    }
                    else if (t == TileType::Void) {
                        DrawRectangle(renderX, renderY, TILE_SIZE, TILE_SIZE, { 20, 10, 30, 200 });
                    }

                    SpawnPoint goal = gameMap.getGoalPoint();
                    if (worldX == goal.x && worldY == goal.y) DrawCircle(renderX + TILE_SIZE / 2.0f, renderY + TILE_SIZE / 2.0f, TILE_SIZE / 2.0f, YELLOW);
                }
            }

            for (const auto& e : enemies) {
                if (e.isAlive()) {
                    float renderX = (e.getRealX() - cam.x - 0.10f + camShakeX) * TILE_SIZE;
                    float renderY = (e.getRealY() - cam.y + 0.40f + camShakeY) * TILE_SIZE;
                    bool eFacingLeft = (e.getVelocityX() < 0);
                    if (crawlerTex.id != 0) {
                        DrawSpriteFrame(crawlerTex, e.getCurrentFrame(), e.getTotalFrames(), renderX, renderY, 1.0f * TILE_SIZE, 0.6f * TILE_SIZE, e.isFlickering() ? RED : WHITE, eFacingLeft);
                    }
                    else {
                        DrawRectangle(renderX, renderY, 0.8f * TILE_SIZE, 0.5f * TILE_SIZE, e.isFlickering() ? ORANGE : RED);
                    }
                }
            }

            // 绘制玩家骑士（强化的渲染状态机）
            float pRenderX = (player.getRealX() - cam.x - 0.25f + camShakeX) * TILE_SIZE;
            float pRenderY = (player.getRealY() - cam.y + camShakeY) * TILE_SIZE;

            static bool facingLeft = false;
            // 冲刺和攻击时，强行锁死身体朝向
            if (!player.getIsDashing() && !isAttacking) {
                if (IsKeyDown(KEY_A)) facingLeft = true;
                else if (IsKeyDown(KEY_D)) facingLeft = false;
            }

            static float frameTimer = 0.0f;
            static int currentFrame = 0;
            const float FRAME_UPDATE_TIME = 0.12f;
            frameTimer += GetFrameTime();

            // ==========================================
                        // 重构后的渲染状态机 (彻底消灭越界幽灵帧)
                        // ==========================================
            Texture2D activeTex = texStand;
            int overrideFrame = -1; // -1 代表顺其自然，使用底层的 currentFrame

            int totalFrames = 2;
            // 优先级 1.1：下劈攻击 (最高优先级)
            if (isPogoAttacking) {
                activeTex = texPogo;
                totalFrames = POGO_TOTAL_FRAMES;
                overrideFrame = attackFrame;
            }
            // 优先级 1.2：普通攻击
            else if (isAttacking) {
                activeTex = texAttack;
                totalFrames = ATTACK_TOTAL_FRAMES;
                overrideFrame = attackFrame;
            }
            // 优先级 2：冲刺中
            else if (player.getIsDashing()) {
                activeTex = texRun;
                totalFrames = 5;
            }
            // 优先级 3：空中 (物理分层动画)
            else if (!player.getIsGrounded()) {
                float vy = player.getVelocityY();
                if (vy < -5.0f) activeTex = texJumpUp;
                else if (vy < 5.0f) activeTex = texJumpMid;
                else activeTex = texFall;
                totalFrames = 1;
                overrideFrame = 0; // 空中姿态死锁在第 0 帧
            }
            // 优先级 4：地面移动
            else if (IsKeyDown(KEY_A) || IsKeyDown(KEY_D)) {
                if (IsKeyDown(KEY_LEFT_SHIFT)) {
                    activeTex = texRun;
                    totalFrames = 5;
                }
                else {
                    activeTex = texWalk;
                    totalFrames = 3;
                }
            }
            // 优先级 5：待机
            else {
                activeTex = texStand;
                totalFrames = 2; // 【注】：确认你的 stand 图片包含两帧
            }

            // --- 换图安全锁：一旦贴图改变，立刻重置常规帧数 ---
            static unsigned int lastTexId = activeTex.id;
            if (activeTex.id != lastTexId) {
                currentFrame = 0;
                frameTimer = 0.0f;
                lastTexId = activeTex.id;
            }

            // --- 动态赋予动画时间流速 ---
            float currentFrameTime = 0.12f;
            if (activeTex.id == texStand.id) currentFrameTime = 0.40f;
            else if (activeTex.id == texRun.id) currentFrameTime = 0.08f;
            else if (activeTex.id == texWalk.id) currentFrameTime = 0.16f;

            // --- 推进常规帧数 ---
            if (overrideFrame == -1) {
                frameTimer += GetFrameTime();
                if (frameTimer >= currentFrameTime) {
                    currentFrame = (currentFrame + 1) % totalFrames;
                    frameTimer = 0.0f;
                }
            }

            // --- 终极绘制抉择与越界保护 ---
            int frameToDraw = (overrideFrame != -1) ? overrideFrame : currentFrame;
            if (frameToDraw >= totalFrames) frameToDraw = 0;

            // ==========================================
            // 【找回的拼图】：残影的生成逻辑与绘制
            // ==========================================

            // 1. 冲刺强残影
            if (player.getIsDashing()) {
                static float dashTrailTimer = 0.0f;
                dashTrailTimer += GetFrameTime();
                if (dashTrailTimer >= 0.04f) {
                    dashTrailTimer = 0.0f;
                    DashTrail trail = { activeTex, frameToDraw, totalFrames, player.getRealX(), player.getRealY(), facingLeft, 0.35f, 0.35f };
                    dashTrails.push_back(trail);
                }
            }
            // 2. 奔跑弱残影 (排除了普通攻击和下劈)
            else if (activeTex.id == texRun.id && !isAttacking && !isPogoAttacking) {
                static float runTrailTimer = 0.0f;
                runTrailTimer += GetFrameTime();
                if (runTrailTimer >= 0.06f) {
                    runTrailTimer = 0.0f;
                    DashTrail trail = { activeTex, frameToDraw, totalFrames, player.getRealX(), player.getRealY(), facingLeft, 0.25f, 0.4f };
                    dashTrails.push_back(trail);
                }
            }

            // 3. 画出所有残影（压在实体下面）
            for (const auto& trail : dashTrails) {
                float tRenderX = (trail.worldX - cam.x - 0.25f + camShakeX) * TILE_SIZE;
                float tRenderY = (trail.worldY - cam.y + camShakeY) * TILE_SIZE;
                unsigned char alpha = (unsigned char)(200 * (trail.life / trail.maxLife));
                Color trailColor = { 50, 150, 255, alpha };
                DrawSpriteFrame(trail.tex, trail.frameIndex, trail.totalFrames, tRenderX, tRenderY, TILE_SIZE, TILE_SIZE, trailColor, trail.facingLeft);
            }

            // ==========================================
            // --- 最终绘制 (主角本体) ---
            // ==========================================
            if (activeTex.id != 0) {
                DrawSpriteFrame(activeTex, frameToDraw, totalFrames, pRenderX, pRenderY, TILE_SIZE, TILE_SIZE, player.isFlickering() ? RED : WHITE, facingLeft);
            }
            else {
                DrawRectangle(pRenderX, pRenderY, 0.5f * TILE_SIZE, 0.8f * TILE_SIZE, player.isFlickering() ? SKYBLUE : BLUE);
            }
            // 绘制火花特效
            for (const auto& p : hitParticles) {
                float renderX = (p.pos.x - cam.x + camShakeX) * TILE_SIZE;
                float renderY = (p.pos.y - cam.y + camShakeY) * TILE_SIZE;
                unsigned char alpha = (unsigned char)(255 * (p.life / p.maxLife));
                Color renderColor = { p.color.r, p.color.g, p.color.b, alpha };
                DrawRectangle(renderX, renderY, p.size * TILE_SIZE, p.size * TILE_SIZE, renderColor);
            }

            DrawTextEx(myFont, TextFormat("HP: %d/%d  LVL: %d  XP: %d/%d", player.getHp(), player.getMaxHp(), player.getLevel(), player.getExp(), player.getExpToNext()), { 10, 10 }, 20, 1.0f, GREEN);
            DrawTextEx(myFont, combatLog.c_str(), { 10, 40 }, 20, 1.0f, WHITE);
        }
        else if (currentState == GAME_OVER) {
            DrawTextEx(myFont, "GAME OVER", { 280, 150 }, 40, 1.0f, RED);
            DrawTextEx(myFont, "The knight has fallen...", { 280, 220 }, 20, 1.0f, LIGHTGRAY);
        }
        EndTextureMode();

        // ==========================================
        // 第二阶段：将虚拟画布投射到真实屏幕上 (自动居中、等比缩放)
        // ==========================================
        BeginDrawing();
        ClearBackground(BLACK); // 屏幕外围的黑边填充

        // 计算最大可用的缩放比例
        float scale = fmin((float)GetScreenWidth() / 1280.0f, (float)GetScreenHeight() / 720.0f);

        // 计算居中偏移量
        float offsetX = (GetScreenWidth() - (1280.0f * scale)) * 0.5f;
        float offsetY = (GetScreenHeight() - (720.0f * scale)) * 0.5f;

        // 将虚拟画布贴到屏幕上 (注意 Raylib 的 RenderTexture 在 Y 轴是倒置的，所以高度要给负数)
        Rectangle sourceRec = { 0.0f, 0.0f, (float)target.texture.width, (float)-target.texture.height };
        Rectangle destRec = { offsetX, offsetY, 1280.0f * scale, 720.0f * scale };

        DrawTexturePro(target.texture, sourceRec, destRec, { 0, 0 }, 0.0f, WHITE);

        EndDrawing();
    }

    // ==========================================
    // 资源释放区
    // ==========================================
    UnloadFont(myFont);
    UnloadTexture(bgTex);
    UnloadTexture(texStand);
    UnloadTexture(texWalk);
    UnloadTexture(texRun);
    UnloadTexture(texAttack);
    UnloadTexture(crawlerTex);
    UnloadTexture(wallTex);
    UnloadTexture(platformTex);
    UnloadTexture(spikeTex);

    CloseWindow();
    return 0;
}