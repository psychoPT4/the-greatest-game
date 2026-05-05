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
#include <fstream>
#include <sstream>
#include <iostream>

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

// 战斗受击火花粒子
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
    InitAudioDevice();
    SetConfigFlags(FLAG_VSYNC_HINT);
    SetTargetFPS(60);

    std::vector<Music> bgms;
    bgms.push_back(LoadMusicStream("bgm0.mp3")); // 下标 0: 主菜单
    bgms.push_back(LoadMusicStream("bgm1.mp3")); // 下标 1: 第一关 (平原)
    bgms.push_back(LoadMusicStream("bgm2.mp3")); // 下标 2: 第二关 (洞穴)
    for (auto& m : bgms) SetMusicVolume(m, 0.4f);
    SetMusicVolume(bgms[0], 0.5f); // 菜单音量稍微大一点
    SetMusicVolume(bgms[1], 0.6f);

    int currentBgmIndex = 0; // 当前正在播放的音轨下标
    PlayMusicStream(bgms[currentBgmIndex]); // 游戏启动，先放菜单音乐

    Sound sfxSwing = LoadSound("swing.wav");
    Sound sfxHit = LoadSound("hit.wav");
    Sound sfxJump = LoadSound("jump.wav");
    Sound sfxLand = LoadSound("land.wav");
    Sound sfxDash = LoadSound("dash.wav");
    Sound sfxWalk = LoadSound("walk.wav");
    Sound sfxRun = LoadSound("run.wav");

    RenderTexture2D target = LoadRenderTexture(1280, 720);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

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
    Texture2D texAttack = LoadTexture("knight_attack.png");
    Texture2D crawlerTex = LoadTexture("crawler.png");
    Texture2D wallTex = LoadTexture("wall.png");
    Texture2D platformTex = LoadTexture("platform.png");
    Texture2D spikeTex = LoadTexture("spike.png");
    Texture2D texJumpUp = LoadTexture("knight_jump_up.png");
    Texture2D texJumpMid = LoadTexture("knight_jump_mid.png");
    Texture2D texFall = LoadTexture("knight_fall.png");
    Texture2D texLand = LoadTexture("knight_land.png");
    Texture2D texPogo = LoadTexture("knight_pogo.png");
    Texture2D bgFar = LoadTexture("bg_far.png");

    float bgScrollSpeed = 0.15f;
    int currentLevel = 1;
    bool mapLoaded = false;
    GameCamera cam(screenWidth / TILE_SIZE, screenHeight / TILE_SIZE);

    Player player(0, 0);
    Map gameMap;
    vector<Enemy> enemies;
    static int attackCd = 0;

    Rectangle btnStart = { screenWidth / 2.0f - 100, screenHeight / 2.0f, 200, 50 };
    Rectangle btnSettings = { screenWidth / 2.0f - 100, screenHeight / 2.0f + 70, 200, 50 };
    Rectangle btnExit = { screenWidth / 2.0f - 100, screenHeight / 2.0f + 140, 200, 50 };

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

    bool isAttacking = false;
    bool isPogoAttacking = false;
    int attackFrame = 0;
    float attackAnimTimer = 0.0f;
    bool damageDealtThisSwing = false;
    const int ATTACK_TOTAL_FRAMES = 5;
    const int POGO_TOTAL_FRAMES = 2;
    const float ATTACK_SPEED = 0.05f;

    // ==========================================
    // 游戏主循环
    // ==========================================
    while (!WindowShouldClose()) {

        UpdateMusicStream(bgms[currentBgmIndex]);
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;

        // --- 2. 逻辑更新区 ---
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
                StopMusicStream(bgms[currentBgmIndex]);
                currentBgmIndex = currentLevel; // 直接把关卡号当成切歌指令！
                PlayMusicStream(bgms[currentBgmIndex]);
            }
            if (CheckCollisionPointRec(mousePos, btnExit) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                break;
            }
            if (IsKeyPressed(KEY_ENTER)) {
                currentState = PLAYING; mapLoaded = false;
                StopMusicStream(bgms[currentBgmIndex]);
                currentBgmIndex = currentLevel; // 直接把关卡号当成切歌指令！
                PlayMusicStream(bgms[currentBgmIndex]);
            }
        }
        else if (currentState == PLAYING) {
            if (!mapLoaded) {
                if (!gameMap.loadLevel(currentLevel)) break;
                SpawnPoint pSpawn = gameMap.getPlayerSpawn();
                player.setRealPos((float)pSpawn.x, (float)pSpawn.y);
                safeX = (float)pSpawn.x; safeY = (float)pSpawn.y;

                enemies.clear();
                for (const auto& s : gameMap.getEnemySpawns()) {
                    if (s.type == 8) enemies.emplace_back(s.x, s.y, 40);
                }
                mapLoaded = true;
            }

            // 【完美的魔法分离】：传入按键状态，让主角自己思考
            MagicAction mAct = player.processMagic(IsKeyDown(KEY_I), IsKeyReleased(KEY_I), dt);

            if (mAct == CAST_SPELL) {
                PlaySound(sfxSwing); // 如果你有施法音效最好，这里暂借挥剑音效
                camShakeX = 0.3f;    // 后坐力
                combatLog = "释放灵魂冲击波！";
            }
            else if (mAct == HEALED) {
                PlaySound(sfxLand);  // 暂借落地音效当回血音效
                camShakeY = 0.1f;    // 能量灌注的微小震动
                combatLog = "凝聚灵魂，回复生命！";
            }

            // 在物理更新区调用：
            player.updateProjectiles(enemies, gameMap, dt);

            // 按下 L 键冲刺，改用 tryDash：
            if (IsKeyPressed(KEY_L)) {
                if (player.tryDash()) PlaySound(sfxDash);
            }

            int moveDir = (IsKeyDown(KEY_D) - IsKeyDown(KEY_A));
            static float stepTimer = 0.4f;
            if (player.getIsGrounded() && moveDir != 0 && !player.getIsDashing() && !isAttacking && !isPogoAttacking) {
                stepTimer += dt;
                float currentStepInterval = IsKeyDown(KEY_LEFT_SHIFT) ? 0.28f : 0.45f;
                if (stepTimer >= currentStepInterval) {
                    PlaySound(IsKeyDown(KEY_LEFT_SHIFT) ? sfxRun : sfxWalk);
                    stepTimer = 0.0f;
                }
            }
            else {
                stepTimer = 0.4f;
            }
            player.setMoveIntent(moveDir);
            player.setRunningMode(IsKeyDown(KEY_LEFT_SHIFT));

            bool jump = IsKeyDown(KEY_K);
            static bool lastJump = false;
            if (jump && !lastJump) {
                if (player.getIsGrounded() || player.getJumpCount() < 2) {
                    PlaySound(sfxJump);
                }
            }
            player.processJump(jump && !lastJump, jump, IsKeyDown(KEY_S));
            lastJump = jump;

            if (IsKeyDown(KEY_J) && !isAttacking && !isPogoAttacking && attackCd <= 0 && !player.getIsDashing()) {
                PlaySound(sfxSwing);
                if (!player.getIsGrounded() && IsKeyDown(KEY_S)) {
                    isPogoAttacking = true;
                }
                else {
                    isAttacking = true;
                }
                attackFrame = 0;
                attackAnimTimer = 0.0f;
                damageDealtThisSwing = false;
            }

            if (isAttacking || isPogoAttacking) {
                if (hitStopTimer <= 0.0f) {
                    attackAnimTimer += dt;
                    float currentAnimSpeed = isPogoAttacking ? 0.12f : ATTACK_SPEED;

                    if (attackAnimTimer >= currentAnimSpeed) {
                        attackFrame++;
                        attackAnimTimer = 0.0f;
                    }
                }

                int triggerFrame = isPogoAttacking ? 1 : 2;

                if (attackFrame == triggerFrame && !damageDealtThisSwing) {
                    AttackResult result = player.attack(enemies, gameMap, IsKeyDown(KEY_S));
                    if (result.totalXp > 0) player.addExp(result.totalXp);

                    if (result.hitSomething || result.pogoSuccess) {
                        PlaySound(sfxHit);
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

            if (hitStopTimer > 0.0f) {
                hitStopTimer -= dt;
                camShakeX = GetRandomValue(-15, 15) / 100.0f;
                camShakeY = GetRandomValue(-15, 15) / 100.0f;
            }
            else {
                camShakeX = 0.0f;
                camShakeY = 0.0f;

                player.update(gameMap, dt);
                static bool wasGroundedLastFrame = true;
                bool isGroundedNow = player.getIsGrounded();

                if (!wasGroundedLastFrame && isGroundedNow) {
                    PlaySound(sfxLand);
                    camShakeY = 0.05f;
                }
                wasGroundedLastFrame = isGroundedNow;
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
                for (auto it = dashTrails.begin(); it != dashTrails.end(); ) {
                    it->life -= dt;
                    if (it->life <= 0) it = dashTrails.erase(it);
                    else ++it;
                }
            }

            Hitbox pBox = player.getHitbox();
            TileType footL = gameMap.getTileAt((int)pBox.left, (int)pBox.bottom);
            TileType footR = gameMap.getTileAt((int)pBox.right, (int)pBox.bottom);

            if (footL == TileType::Void || footR == TileType::Void || player.getY() >= gameMap.getHeight() - 1) {
                if (!player.isFlickering()) {
                    player.takeDamage(player.getMaxHp() / 4, player.getX(), gameMap);
                    player.setRealPos(safeX, safeY);
                    combatLog = "Fell into the void!";
                }
            }
            else if (footL == TileType::SpikeUp || footR == TileType::SpikeUp) {
                if (!player.isFlickering()) {
                    player.takeDamage(20, player.getX(), gameMap);
                    combatLog = "跌入地刺！ ";
                    hitStopTimer = 0.1f;
                    camShakeX = 0.2f;
                    camShakeY = 0.2f;
                }
            }

            cam.update(player.getRealX(), player.getRealY(), gameMap.getWidth(), gameMap.getHeight());

            SpawnPoint goal = gameMap.getGoalPoint();
            if (player.getX() == goal.x && player.getY() == goal.y) {
                currentLevel++;
                mapLoaded = false;
                // 【新增】：过关无缝切歌
                StopMusicStream(bgms[currentBgmIndex]);
                // 防止越界保护：如果还没准备 bgm3，就继续循环播最后一首
                currentBgmIndex = (currentLevel < bgms.size()) ? currentLevel : bgms.size() - 1;
                PlayMusicStream(bgms[currentBgmIndex]);
            }

            if (!player.isAlive()) {
                currentState = GAME_OVER;
            }
        } // 结束 PLAYING 逻辑判断

        // ==========================================
        // --- 3. 极其严格的渲染管线 (汉堡包结构) ---
        // ==========================================

        // 【第一层：画到虚拟画布】
        BeginTextureMode(target);
        ClearBackground({ 20, 20, 30, 255 }); // 游戏底色

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

            // 🌟 【绝对的最底层】：画多层视差大背景
            if (!gameMap.getBgLayers().empty()) {
                float camTargetX = player.getRealX();
                for (const auto& layer : gameMap.getBgLayers()) {
                    float parallaxOffsetX = -camTargetX * layer.scrollSpeed;
                    float scaleY = 720.0f / layer.tex.height; // 强行拉伸填满屏幕高度
                    float scaleX = scaleY;
                    float scaledWidth = layer.tex.width * scaleX;
                    float bgWrapX = fmod(parallaxOffsetX, scaledWidth);

                    DrawTextureEx(layer.tex, { bgWrapX, 0 }, 0.0f, scaleX, WHITE);
                    DrawTextureEx(layer.tex, { bgWrapX + scaledWidth, 0 }, 0.0f, scaleX, WHITE);
                    DrawTextureEx(layer.tex, { bgWrapX - scaledWidth, 0 }, 0.0f, scaleX, WHITE);
                }
            }

            // 🌟 【倒数第二层】：画物理世界（地块、地刺、平台）
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
                        TileType tileAbove = gameMap.getTileAt(worldX, worldY - 1);

                        // 【核心修复】：只要头顶不是墙，且头顶不是地刺，才长草！
                        if (tileAbove != TileType::Wall && tileAbove != TileType::SpikeUp) {
                            Texture2D tex = gameMap.getTexGroundTop(); // 草地
                            if (tex.id != 0) DrawTexExact(tex, renderX, renderY, TILE_SIZE, TILE_SIZE);
                            else DrawRectangle(renderX, renderY, TILE_SIZE, TILE_SIZE, GREEN);
                        }
                        else {
                            Texture2D tex = gameMap.getTexGroundDeep(); // 泥土
                            if (tex.id != 0) DrawTexExact(tex, renderX, renderY, TILE_SIZE, TILE_SIZE);
                            else DrawRectangle(renderX, renderY, TILE_SIZE, TILE_SIZE, DARKBROWN);
                        }
                    }
                    else if (t == TileType::Platform) {
                        if (platformTex.id != 0) DrawTexExact(platformTex, renderX, renderY, TILE_SIZE, TILE_SIZE / 4.0f);
                        else DrawRectangle(renderX, renderY, TILE_SIZE, TILE_SIZE / 4.0f, GRAY);
                    }
                    else if (t == TileType::SpikeUp) {
                        // 【核心修复】：删掉垫底的泥土，只画你带有留白的地刺贴图！
                        if (spikeTex.id != 0) DrawTexExact(spikeTex, renderX, renderY, TILE_SIZE, TILE_SIZE);
                        else DrawTriangle({ renderX + TILE_SIZE / 2.0f, renderY }, { renderX, renderY + TILE_SIZE }, { renderX + TILE_SIZE, renderY + TILE_SIZE }, RED);
                    }
                    else if (t == TileType::Void) {
                        // 放弃纯色块，改用从“深紫/透明”到“全黑”的垂直渐变
                        // 这样看起来就像地表之下的一片虚无迷雾
                        Color voidTop = { 20, 10, 30, 100 }; // 半透明深紫色
                        Color voidBottom = { 0, 0, 0, 255 };  // 纯黑
                        DrawRectangleGradientV(renderX, renderY, TILE_SIZE, TILE_SIZE, voidTop, voidBottom);
                    }
                    SpawnPoint goal = gameMap.getGoalPoint();
                    if (worldX == goal.x && worldY == goal.y) {
                        float pulse = (sin(GetTime() * 5.0f) + 1.0f) * 0.5f;
                        // 绘制一个带光晕的核心
                        Color portalColor = { 255, 255, 150, (unsigned char)(150 + 105 * pulse) };
                        float radius = (0.35f + 0.1f * pulse) * TILE_SIZE; // 半径随呼吸跳动

                        DrawCircle(renderX + TILE_SIZE / 2.0f, renderY + TILE_SIZE / 2.0f, radius, portalColor);
                        // 外围加一圈淡淡的亮边线
                        DrawCircleLines(renderX + TILE_SIZE / 2.0f, renderY + TILE_SIZE / 2.0f, radius + 2, YELLOW);
                    }
                }
            }
            for (const auto& p : player.getProjectiles()) {
                float pRenderX = (p.x - cam.x + camShakeX) * TILE_SIZE;
                float pRenderY = (p.y - cam.y + camShakeY) * TILE_SIZE;
                DrawCircle(pRenderX, pRenderY, 0.6f * TILE_SIZE, { 100, 150, 255, 150 });
                DrawCircle(pRenderX, pRenderY, 0.3f * TILE_SIZE, WHITE);
                float tailLength = 1.5f * TILE_SIZE;
                float tailStart = p.facingDir == 1 ? pRenderX - tailLength : pRenderX;
                DrawRectangle(tailStart, pRenderY - 10, tailLength, 20, { 200, 220, 255, 100 });
            }
            // 画敌人
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

            // 决定主角用哪张贴图
            float pRenderX = (player.getRealX() - cam.x - 0.25f + camShakeX) * TILE_SIZE;
            float pRenderY = (player.getRealY() - cam.y + camShakeY) * TILE_SIZE;
            static bool facingLeft = false;
            if (!player.getIsDashing() && !isAttacking) {
                if (IsKeyDown(KEY_A)) facingLeft = true;
                else if (IsKeyDown(KEY_D)) facingLeft = false;
            }

            static float frameTimer = 0.0f;
            static int currentFrame = 0;
            frameTimer += GetFrameTime();

            Texture2D activeTex = texStand;
            int overrideFrame = -1;
            int totalFrames = 2;

            if (isPogoAttacking) { activeTex = texPogo; totalFrames = POGO_TOTAL_FRAMES; overrideFrame = attackFrame; }
            else if (isAttacking) { activeTex = texAttack; totalFrames = ATTACK_TOTAL_FRAMES; overrideFrame = attackFrame; }
            else if (player.getIsDashing()) { activeTex = texRun; totalFrames = 5; }
            else if (player.getIsFocusing()) { activeTex = texJumpMid; totalFrames = 1; overrideFrame = 0; } // 蓄力时借用空中蜷缩的姿态
            else if (!player.getIsGrounded()) {
                float vy = player.getVelocityY();
                if (vy < -5.0f) activeTex = texJumpUp;
                else if (vy < 5.0f) activeTex = texJumpMid;
                else activeTex = texFall;
                totalFrames = 1;
                overrideFrame = 0;
            }
            else if (IsKeyDown(KEY_A) || IsKeyDown(KEY_D)) {
                if (IsKeyDown(KEY_LEFT_SHIFT)) { activeTex = texRun; totalFrames = 5; }
                else { activeTex = texWalk; totalFrames = 3; }
            }
            else { activeTex = texStand; totalFrames = 2; }

            static unsigned int lastTexId = activeTex.id;
            if (activeTex.id != lastTexId) {
                currentFrame = 0;
                frameTimer = 0.0f;
                lastTexId = activeTex.id;
            }

           float currentFrameTime = 0.12f;
            if (activeTex.id == texStand.id) currentFrameTime = 0.40f;
            else if (activeTex.id == texRun.id) currentFrameTime = 0.08f;
            else if (activeTex.id == texWalk.id) currentFrameTime = 0.16f;

            if (overrideFrame == -1) {
                if (frameTimer >= currentFrameTime) {
                    currentFrame = (currentFrame + 1) % totalFrames;
                    frameTimer = 0.0f;
                }
            }

            int frameToDraw = (overrideFrame != -1) ? overrideFrame : currentFrame;
            if (frameToDraw >= totalFrames) frameToDraw = 0;
            if (player.getIsFocusing()) {
                // 画一个正在收缩的能量场
                float focusPulse = (sin(GetTime() * 15.0f) + 1.0f) * 0.5f;
                DrawCircleLines(pRenderX + TILE_SIZE / 2, pRenderY + TILE_SIZE / 2, TILE_SIZE * focusPulse, { 255, 255, 255, 150 });
            }
            // 残影逻辑生成
            if (player.getIsDashing()) {
                static float dashTrailTimer = 0.0f;
                dashTrailTimer += GetFrameTime();
                if (dashTrailTimer >= 0.04f) {
                    dashTrailTimer = 0.0f;
                    dashTrails.push_back({ activeTex, frameToDraw, totalFrames, player.getRealX(), player.getRealY(), facingLeft, 0.35f, 0.35f });
                }
            }
            else if (activeTex.id == texRun.id && !isAttacking && !isPogoAttacking) {
                static float runTrailTimer = 0.0f;
                runTrailTimer += GetFrameTime();
                if (runTrailTimer >= 0.06f) {
                    runTrailTimer = 0.0f;
                    dashTrails.push_back({ activeTex, frameToDraw, totalFrames, player.getRealX(), player.getRealY(), facingLeft, 0.25f, 0.4f });
                }
            }

            // 画残影
            for (const auto& trail : dashTrails) {
                float tRenderX = (trail.worldX - cam.x - 0.25f + camShakeX) * TILE_SIZE;
                float tRenderY = (trail.worldY - cam.y + camShakeY) * TILE_SIZE;
                unsigned char alpha = (unsigned char)(200 * (trail.life / trail.maxLife));
                Color trailColor = { 50, 150, 255, alpha };
                DrawSpriteFrame(trail.tex, trail.frameIndex, trail.totalFrames, tRenderX, tRenderY, TILE_SIZE, TILE_SIZE, trailColor, trail.facingLeft);
            }

            // 画本体
            if (activeTex.id != 0) DrawSpriteFrame(activeTex, frameToDraw, totalFrames, pRenderX, pRenderY, TILE_SIZE, TILE_SIZE, player.isFlickering() ? RED : WHITE, facingLeft);
            else DrawRectangle(pRenderX, pRenderY, 0.5f * TILE_SIZE, 0.8f * TILE_SIZE, player.isFlickering() ? SKYBLUE : BLUE);
            if (player.getHealFlashTimer() > 0.0f) {
                // 利用透明度衰减，制造出瞬间照亮整个屏幕的“神圣白光”
                unsigned char flashAlpha = (unsigned char)(255.0f * (player.getHealFlashTimer() / 0.3f));
                DrawRectangle(0, 0, screenWidth, screenHeight, { 255, 255, 255, flashAlpha });
            }
            // 画粒子
            for (const auto& p : hitParticles) {
                float renderX = (p.pos.x - cam.x + camShakeX) * TILE_SIZE;
                float renderY = (p.pos.y - cam.y + camShakeY) * TILE_SIZE;
                unsigned char alpha = (unsigned char)(255 * (p.life / p.maxLife));
                Color renderColor = { p.color.r, p.color.g, p.color.b, alpha };
                DrawRectangle(renderX, renderY, p.size * TILE_SIZE, p.size * TILE_SIZE, renderColor);
            }

            // ==========================================
                        // 终极现代化状态栏 UI (HUD)
                        // ==========================================
            float uiX = 20.0f;
            float uiY = 20.0f;

            // 1. 生命值 (HP) - 红色经典
            DrawRectangle(uiX, uiY, 200, 20, { 50, 20, 20, 200 });
            float hpRatio = fmax(0.0f, (float)player.getHp() / player.getMaxHp());
            DrawRectangle(uiX, uiY, 200 * hpRatio, 20, RED);
            DrawRectangleLines(uiX, uiY, 200, 20, WHITE);
            DrawTextEx(myFont, TextFormat("HP: %d/%d", player.getHp(), player.getMaxHp()), { uiX + 45, uiY + 2 }, 16, 1.0f, WHITE);

            // 2. 灵魂槽 (Mana) - 纯白魔法
            DrawRectangle(uiX, uiY + 25, 150, 12, { 20, 20, 30, 200 });
            float manaRatio = fmax(0.0f, (float)player.getMana() / player.getMaxMana());
            DrawRectangle(uiX, uiY + 25, 150 * manaRatio, 12, WHITE);
            DrawRectangleLines(uiX, uiY + 25, 150, 12, LIGHTGRAY);
            DrawTextEx(myFont, TextFormat("MP: %d/%d", player.getMana(), player.getMaxMana()), { uiX + 160, uiY + 23 }, 16, 1.0f, LIGHTGRAY);

            // 3. 体力条 (Stamina) - 耀眼金黄
            DrawRectangle(uiX, uiY + 42, 120, 8, { 40, 30, 10, 200 });
            float staRatio = fmax(0.0f, player.getStamina() / player.getMaxStamina());
            DrawRectangle(uiX, uiY + 42, 120 * staRatio, 8, YELLOW);
            DrawRectangleLines(uiX, uiY + 42, 120, 8, GRAY);

            // 4. 经验条 (XP) - 翠绿成长
            DrawRectangle(uiX, uiY + 55, 200, 6, { 10, 30, 10, 200 });
            float xpRatio = fmax(0.0f, (float)player.getExp() / player.getExpToNext());
            DrawRectangle(uiX, uiY + 55, 200 * xpRatio, 6, GREEN);
            DrawRectangleLines(uiX, uiY + 55, 200, 6, DARKGRAY);
            DrawTextEx(myFont, TextFormat("LVL %d", player.getLevel()), { uiX + 210, uiY + 50 }, 16, 1.0f, GOLD);

            // 战斗日志避开长条 UI
            DrawTextEx(myFont, combatLog.c_str(), { uiX, uiY + 75 }, 20, 1.0f, WHITE);
        }
        else if (currentState == GAME_OVER) {
            DrawTextEx(myFont, "GAME OVER", { 280, 150 }, 40, 1.0f, RED);
            DrawTextEx(myFont, "The knight has fallen...", { 280, 220 }, 20, 1.0f, LIGHTGRAY);
        }

        EndTextureMode(); // 闭合虚拟画布
        // 【第二层：把虚拟画布贴到真实屏幕上】
        BeginDrawing();
        ClearBackground(BLACK); // 屏幕黑边

        float scale = fmin((float)GetScreenWidth() / 1280.0f, (float)GetScreenHeight() / 720.0f);
        float offsetX = (GetScreenWidth() - (1280.0f * scale)) * 0.5f;
        float offsetY = (GetScreenHeight() - (720.0f * scale)) * 0.5f;

        Rectangle sourceRec = { 0.0f, 0.0f, (float)target.texture.width, (float)-target.texture.height };
        Rectangle destRec = { offsetX, offsetY, 1280.0f * scale, 720.0f * scale };

        DrawTexturePro(target.texture, sourceRec, destRec, { 0, 0 }, 0.0f, WHITE);

        EndDrawing(); // 闭合真实屏幕！必须在 while 循环里面！

    } // while 循环结束

    // ==========================================
    // 资源释放区
    // ==========================================
    for (auto& m : bgms) UnloadMusicStream(m);
    UnloadRenderTexture(target);
    UnloadSound(sfxSwing);
    UnloadSound(sfxHit);
    UnloadSound(sfxJump);
    UnloadSound(sfxDash);
    UnloadSound(sfxWalk);
    UnloadSound(sfxRun);
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
    UnloadTexture(bgFar);

    CloseAudioDevice();
    CloseWindow();
    return 0;
} 