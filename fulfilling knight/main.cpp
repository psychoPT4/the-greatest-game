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

using namespace std;

enum GameState { MENU, PLAYING, PAUSED, SETTINGS, GAME_OVER };
GameState currentState = MENU;

struct SporeParticle {
    Vector2 pos;
    Vector2 velocity;
    float radius;
    unsigned char alpha;
};

struct HitParticle {
    Vector2 pos;
    Vector2 velocity;
    float life;
    float maxLife;
    Color color;
    float size;
};

struct DashTrail {
    Texture2D tex;
    int frameIndex;
    int totalFrames;
    float worldX;
    float worldY;
    bool facingLeft;
    float life;
    float maxLife;
};

// 🌟 新增：独立存活的命中十字爆特效
struct HitImpactVFX {
    float x;
    float y;
    int frame;
    float timer;
    float rotation;
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
    bgms.push_back(LoadMusicStream("bgm0.mp3"));
    bgms.push_back(LoadMusicStream("bgm1.mp3"));
    bgms.push_back(LoadMusicStream("bgm2.mp3"));
    for (auto& m : bgms) SetMusicVolume(m, 0.4f);
    SetMusicVolume(bgms[0], 0.5f);
    SetMusicVolume(bgms[1], 0.6f);

    int currentBgmIndex = 0;
    PlayMusicStream(bgms[currentBgmIndex]);

    Sound sfxSwing = LoadSound("swing.wav");
    Sound sfxHit = LoadSound("hit.wav");
    Sound sfxJump = LoadSound("jump.wav");
    Sound sfxLand = LoadSound("land.wav");
    Sound sfxDash = LoadSound("dash.wav");
    Sound sfxWalk = LoadSound("walk.wav");
    Sound sfxRun = LoadSound("run.wav");
    Sound sfxCast = LoadSound("cast_spell.wav");
    Sound sfxHeal = LoadSound("heal.wav");

    int codepointCount = (127 - 32) + (0x9FA5 - 0x4E00 + 1);
    int* codepoints = new int[codepointCount];
    int index = 0;
    for (int i = 32; i < 127; i++) codepoints[index++] = i;
    for (int i = 0x4E00; i <= 0x9FA5; i++) codepoints[index++] = i;
    Font myFont = LoadFontEx("font.ttf", 32, codepoints, codepointCount);
    delete[] codepoints;

    Texture2D bgTex = LoadTexture("bg.png");

    // ==========================================
    // 🌟 精确散图阵列 (骑士全新资产)
    // ==========================================
    vector<Texture2D> knightStand;
    for (int i = 0; i < 2; i++) knightStand.push_back(LoadTexture(TextFormat("stand%d.png", i)));

    vector<Texture2D> knightWalk;
    for (int i = 0; i < 4; i++) knightWalk.push_back(LoadTexture(TextFormat("walk%d.png", i)));

    vector<Texture2D> knightRun;
    for (int i = 0; i < 5; i++) knightRun.push_back(LoadTexture(TextFormat("run%d.png", i)));

    vector<Texture2D> knightAttack; // 单段平砍 (3帧)
    for (int i = 0; i < 3; i++) knightAttack.push_back(LoadTexture(TextFormat("attack%d.png", i)));

    vector<Texture2D> knightUpAttack; // 上挑 (3帧)
    for (int i = 0; i < 3; i++) knightUpAttack.push_back(LoadTexture(TextFormat("upattack%d.png", i)));

    vector<Texture2D> knightPogo; // 下劈 (4帧)
    for (int i = 0; i < 4; i++) knightPogo.push_back(LoadTexture(TextFormat("pogo%d.png", i)));

    // 🌟 空中动态序列
    vector<Texture2D> knightJump; // 起跳瞬间 (2帧)
    for (int i = 0; i < 2; i++) knightJump.push_back(LoadTexture(TextFormat("jump%d.png", i)));

    vector<Texture2D> knightUp; // 空中上升 (1帧)
    knightUp.push_back(LoadTexture("up.png"));

    vector<Texture2D> knightFall; // 空中下落 (1帧)
    knightFall.push_back(LoadTexture("fall.png"));

    vector<Texture2D> knightLand; // 落地缓冲 (2帧)
    for (int i = 0; i < 2; i++) knightLand.push_back(LoadTexture(TextFormat("land%d.png", i)));

    // 法术动画 (沿用单图加载入 vector)
    vector<Texture2D> knightHeal;
    knightHeal.push_back(LoadTexture("heal.png"));
    vector<Texture2D> knightCast;
    knightCast.push_back(LoadTexture("cast_spell.png"));

    Texture2D crawlerTex[4];
    for (int i = 0; i < 4; i++) crawlerTex[i] = LoadTexture(("crawler" + to_string(i) + ".png").c_str());
    Texture2D flyerTex = LoadTexture("flyer.png");
    Texture2D wallTex = LoadTexture("wall.png");
    Texture2D platformTex = LoadTexture("platform.png");
    Texture2D spikeTex = LoadTexture("spike.png");
    Texture2D bgFar = LoadTexture("bg_far.png");

    RenderTexture2D target = LoadRenderTexture(1280, 720);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

    Texture2D slashTex[4];
    for (int i = 0; i < 4; i++) slashTex[i] = LoadTexture(("slash" + to_string(i) + ".png").c_str());

    Texture2D hitImpactTex[4];
    for (int i = 0; i < 4; i++) hitImpactTex[i] = LoadTexture(("hit_impact" + to_string(i) + ".png").c_str());

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
    vector<HitImpactVFX> activeImpacts;

    float hitStopTimer = 0.0f;
    float camShakeX = 0.0f;
    float camShakeY = 0.0f;

    bool isAttacking = false;
    bool isPogoAttacking = false;
    bool isUpAttacking = false;
    int lockedAttackDir = 1;
    int attackFrame = 0;
    float attackAnimTimer = 0.0f;
    int currentAttackType = 0; // 0=平砍, 1=下劈, 2=上挑
    float castVisualTimer = 0.0f;

    // Animation & input state
    float stepTimer = 0.4f;
    bool lastJump = false;
    bool wasGroundedLastFrame = true;
    bool facingLeft = false;
    float frameTimer = 0.0f;
    int currentFrame = 0;
    bool isFacingLeftNow = false;
    float dashTrailTimer = 0.0f;
    float runTrailTimer = 0.0f;
    float bgmVolume = 0.5f;
    float sfxVolume = 1.0f;

    // 🌟 新增：跳跃与落地的过渡动画计时器
    float jumpAnimTimer = 0.0f;
    float landAnimTimer = 0.0f;
    vector<Texture2D>* lastFrameArray = nullptr;

    while (!WindowShouldClose()) {

        UpdateMusicStream(bgms[currentBgmIndex]);
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
                StopMusicStream(bgms[currentBgmIndex]);
                currentBgmIndex = currentLevel;
                PlayMusicStream(bgms[currentBgmIndex]);
            }
            if (CheckCollisionPointRec(mousePos, btnSettings) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                currentState = SETTINGS;
            }
            if (CheckCollisionPointRec(mousePos, btnExit) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                break;
            }
            if (IsKeyPressed(KEY_ENTER)) {
                currentState = PLAYING; mapLoaded = false;
                StopMusicStream(bgms[currentBgmIndex]);
                currentBgmIndex = currentLevel;
                PlayMusicStream(bgms[currentBgmIndex]);
            }
        }
        else if (currentState == PLAYING) {
            if (IsKeyPressed(KEY_ESCAPE)) {
                currentState = PAUSED;
            }
            if (!mapLoaded) {
                if (!gameMap.loadLevel(currentLevel)) break;
                SpawnPoint pSpawn = gameMap.getPlayerSpawn();
                player.setRealPos((float)pSpawn.x, (float)pSpawn.y);
                safeX = (float)pSpawn.x; safeY = (float)pSpawn.y;

                enemies.clear();
                for (const auto& s : gameMap.getEnemySpawns()) {
                    if (s.type == 8) enemies.emplace_back(s.x, s.y, 0);
                    else if (s.type == 9) enemies.emplace_back(s.x, s.y, 1);
                }
                mapLoaded = true;

                // Reset animation/input state for new level
                stepTimer = 0.4f;
                lastJump = false;
                wasGroundedLastFrame = true;
                frameTimer = 0.0f;
                currentFrame = 0;
                isFacingLeftNow = false;
                lastFrameArray = nullptr;
                dashTrailTimer = 0.0f;
                runTrailTimer = 0.0f;
                jumpAnimTimer = 0.0f;
                landAnimTimer = 0.0f;
                hitParticles.clear();
                dashTrails.clear();
                activeImpacts.clear();
                isAttacking = false;
                isPogoAttacking = false;
                isUpAttacking = false;
                attackCd = 0;
                castVisualTimer = 0.0f;
            }

            if (castVisualTimer > 0.0f) castVisualTimer -= dt;

            MagicAction mAct = player.processMagic(IsKeyDown(KEY_I), IsKeyReleased(KEY_I), dt);

            if (mAct == CAST_SPELL) {
                PlaySound(sfxCast);
                camShakeX = 0.4f;
                castVisualTimer = 0.25f;
                combatLog = "灵魂冲击！";
            }
            else if (mAct == HEALED) {
                PlaySound(sfxHeal);
                camShakeY = 0.1f;
                combatLog = "回复生命！";
            }

            player.updateProjectiles(enemies, gameMap, dt);

            if (IsKeyPressed(KEY_L)) {
                if (player.tryDash()) PlaySound(sfxDash);
            }

            int moveDir = (IsKeyDown(KEY_D) - IsKeyDown(KEY_A));
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

            // 🌟 触发起跳动画
            bool jump = IsKeyDown(KEY_K);
            if (jump && !lastJump) {
                if (player.getIsGrounded() || player.getJumpCount() < 2) {
                    PlaySound(sfxJump);
                    jumpAnimTimer = 0.2f;
                }
            }
            player.processJump(jump && !lastJump, jump, IsKeyDown(KEY_S));
            lastJump = jump;

            // 🌟 精简版单段动作攻击逻辑
            if (IsKeyPressed(KEY_J) && !isAttacking && !player.getIsDashing() && attackCd <= 0) {
                PlaySound(sfxSwing);
                isAttacking = true;
                lockedAttackDir = player.getFacingDirection(); // 瞬间锁死当前朝向！
                attackFrame = 0;
                attackAnimTimer = 0.0f;

                if (!player.getIsGrounded() && IsKeyDown(KEY_S)) {
                    isPogoAttacking = true; currentAttackType = 1;
                }
                else if (IsKeyDown(KEY_W)) {
                    isUpAttacking = true; currentAttackType = 2;
                }
                else {
                    currentAttackType = 0;
                }
            }

            if (isAttacking) {
                if (hitStopTimer <= 0.0f) {
                    attackAnimTimer += dt;
                    float currentAnimSpeed = isPogoAttacking ? 0.08f : 0.05f;

                    if (attackAnimTimer >= currentAnimSpeed) {
                        attackFrame++;
                        attackAnimTimer = 0.0f;
                    }
                }

                // 根据类型获取总帧数 (平砍3, 下劈4, 上挑3)
                int totalFrames = 3;
                if (currentAttackType == 1) totalFrames = 4;

                // 🌟 所有攻击，第 1 到 2 帧都具有强力判定！
                bool isActiveFrame = (attackFrame >= 1 && attackFrame <= 2);

                if (isActiveFrame) {
                    AttackResult result = player.attack(enemies, gameMap, currentAttackType, lockedAttackDir);

                    if (result.hitSomething || result.pogoSuccess) {
                        PlaySound(sfxHit);
                        hitStopTimer = 0.08f;

                        float hitX = player.getRealX();
                        float hitY = player.getRealY();
                        if (result.pogoSuccess) hitY += 1.0f;
                        else if (currentAttackType == 2) hitY -= 1.0f;
                        else {
                            hitY += 0.5f;
                            hitX += (lockedAttackDir == -1) ? -0.6f : 0.6f;
                        }

                        activeImpacts.push_back({ hitX, hitY, 0, 0.0f, (float)GetRandomValue(0, 360) });

                        int sparkCount = GetRandomValue(45, 65);
                        for (int i = 0; i < sparkCount; i++) {
                            HitParticle p = {};
                            p.pos = { hitX, hitY };
                            p.velocity.x = GetRandomValue(-50, 50) / 1.0f;
                            p.velocity.y = GetRandomValue(-40, 5) / 1.0f;
                            p.maxLife = GetRandomValue(15, 35) / 100.0f; p.life = p.maxLife;
                            p.size = GetRandomValue(4, 20) / 100.0f;
                            p.color = (GetRandomValue(0, 10) < 2) ? ORANGE : WHITE;
                            hitParticles.push_back(p);
                        }
                    }
                }

                if (attackFrame >= totalFrames) {
                    isAttacking = false; isPogoAttacking = false; isUpAttacking = false;
                    attackCd = 10;
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

                // 🌟 触发落地缓冲动画
                bool isGroundedNow = player.getIsGrounded();
                if (!wasGroundedLastFrame && isGroundedNow) {
                    PlaySound(sfxLand);
                    camShakeY = 0.05f;
                    landAnimTimer = 0.2f;
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
                    player.takeDamage(25, player.getX(), gameMap);
                    player.setRealPos(safeX, safeY);
                    combatLog = "吞噬于虚空！";
                }
            }
            else if (footL == TileType::SpikeUp || footR == TileType::SpikeUp) {
                if (!player.isFlickering()) {
                    player.takeDamage(25, player.getX(), gameMap);
                    combatLog = "跌入地刺！ ";
                    hitStopTimer = 0.1f;
                    camShakeX = 0.2f; camShakeY = 0.2f;
                }
            }

            cam.update(player.getRealX(), player.getRealY(), gameMap.getWidth(), gameMap.getHeight());

            SpawnPoint goal = gameMap.getGoalPoint();
            if (player.getX() == goal.x && player.getY() == goal.y) {
                currentLevel++;
                mapLoaded = false;
                StopMusicStream(bgms[currentBgmIndex]);
                currentBgmIndex = (currentLevel < bgms.size()) ? currentLevel : bgms.size() - 1;
                PlayMusicStream(bgms[currentBgmIndex]);
            }

            if (!player.isAlive()) {
                currentState = GAME_OVER;
            }
        }
        else if (currentState == PAUSED) {
            if (IsKeyPressed(KEY_ESCAPE)) {
                currentState = PLAYING;
            }
        }
        else if (currentState == SETTINGS) {
            if (IsKeyPressed(KEY_ESCAPE)) {
                currentState = MENU;
            }

            Vector2 mousePos = GetMousePosition();
            Rectangle btnBack = { screenWidth / 2.0f - 60, screenHeight / 2.0f + 160, 120, 50 };
            if (CheckCollisionPointRec(mousePos, btnBack) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                currentState = MENU;
            }

            Rectangle btnBgmDown = { screenWidth / 2.0f + 60, screenHeight / 2.0f - 40, 40, 40 };
            Rectangle btnBgmUp = { screenWidth / 2.0f + 110, screenHeight / 2.0f - 40, 40, 40 };
            Rectangle btnSfxDown = { screenWidth / 2.0f + 60, screenHeight / 2.0f + 30, 40, 40 };
            Rectangle btnSfxUp = { screenWidth / 2.0f + 110, screenHeight / 2.0f + 30, 40, 40 };

            if (CheckCollisionPointRec(mousePos, btnBgmDown) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                bgmVolume = fmax(0.0f, bgmVolume - 0.1f);
                for (auto& m : bgms) SetMusicVolume(m, bgmVolume * 0.8f);
            }
            if (CheckCollisionPointRec(mousePos, btnBgmUp) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                bgmVolume = fmin(1.0f, bgmVolume + 0.1f);
                for (auto& m : bgms) SetMusicVolume(m, bgmVolume * 0.8f);
            }
            if (CheckCollisionPointRec(mousePos, btnSfxDown) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                sfxVolume = fmax(0.0f, sfxVolume - 0.1f);
                SetSoundVolume(sfxSwing, sfxVolume);
                SetSoundVolume(sfxHit, sfxVolume);
                SetSoundVolume(sfxJump, sfxVolume);
                SetSoundVolume(sfxLand, sfxVolume);
                SetSoundVolume(sfxDash, sfxVolume);
                SetSoundVolume(sfxWalk, sfxVolume);
                SetSoundVolume(sfxRun, sfxVolume);
                SetSoundVolume(sfxCast, sfxVolume);
                SetSoundVolume(sfxHeal, sfxVolume);
            }
            if (CheckCollisionPointRec(mousePos, btnSfxUp) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                sfxVolume = fmin(1.0f, sfxVolume + 0.1f);
                SetSoundVolume(sfxSwing, sfxVolume);
                SetSoundVolume(sfxHit, sfxVolume);
                SetSoundVolume(sfxJump, sfxVolume);
                SetSoundVolume(sfxLand, sfxVolume);
                SetSoundVolume(sfxDash, sfxVolume);
                SetSoundVolume(sfxWalk, sfxVolume);
                SetSoundVolume(sfxRun, sfxVolume);
                SetSoundVolume(sfxCast, sfxVolume);
                SetSoundVolume(sfxHeal, sfxVolume);
            }
        }
        else if (currentState == GAME_OVER) {
            if (IsKeyPressed(KEY_ENTER)) {
                currentLevel = 1;
                currentState = PLAYING; mapLoaded = false;
                StopMusicStream(bgms[currentBgmIndex]);
                currentBgmIndex = 0;
                PlayMusicStream(bgms[currentBgmIndex]);
            }
            if (IsKeyPressed(KEY_ESCAPE)) {
                currentState = MENU;
                StopMusicStream(bgms[currentBgmIndex]);
            }
        }

        BeginTextureMode(target);
        ClearBackground({ 20, 20, 30, 255 });

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
        else if (currentState == PLAYING || currentState == PAUSED) {

            if (!gameMap.getBgLayers().empty()) {
                float camTargetX = player.getRealX();
                for (const auto& layer : gameMap.getBgLayers()) {
                    float parallaxOffsetX = -camTargetX * layer.scrollSpeed;
                    float scaleY = 720.0f / layer.tex.height;
                    float scaleX = scaleY;
                    float scaledWidth = layer.tex.width * scaleX;
                    float bgWrapX = fmod(parallaxOffsetX, scaledWidth);

                    DrawTextureEx(layer.tex, { bgWrapX, 0 }, 0.0f, scaleX, WHITE);
                    DrawTextureEx(layer.tex, { bgWrapX + scaledWidth, 0 }, 0.0f, scaleX, WHITE);
                    DrawTextureEx(layer.tex, { bgWrapX - scaledWidth, 0 }, 0.0f, scaleX, WHITE);
                }
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
                        TileType tileAbove = gameMap.getTileAt(worldX, worldY - 1);

                        if (tileAbove != TileType::Wall && tileAbove != TileType::SpikeUp) {
                            Texture2D tex = gameMap.getTexGroundTop();
                            if (tex.id != 0) DrawTexExact(tex, renderX, renderY, TILE_SIZE, TILE_SIZE);
                            else DrawRectangle(renderX, renderY, TILE_SIZE, TILE_SIZE, GREEN);
                        }
                        else {
                            Texture2D tex = gameMap.getTexGroundDeep();
                            if (tex.id != 0) DrawTexExact(tex, renderX, renderY, TILE_SIZE, TILE_SIZE);
                            else DrawRectangle(renderX, renderY, TILE_SIZE, TILE_SIZE, DARKBROWN);
                        }
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
                        Color voidTop = { 20, 10, 30, 100 };
                        Color voidBottom = { 0, 0, 0, 255 };
                        DrawRectangleGradientV(renderX, renderY, TILE_SIZE, TILE_SIZE, voidTop, voidBottom);
                    }
                    SpawnPoint goal = gameMap.getGoalPoint();
                    if (worldX == goal.x && worldY == goal.y) {
                        float pulse = (sin(GetTime() * 5.0f) + 1.0f) * 0.5f;
                        Color portalColor = { 255, 255, 150, (unsigned char)(150 + 105 * pulse) };
                        float radius = (0.35f + 0.1f * pulse) * TILE_SIZE;

                        DrawCircle(renderX + TILE_SIZE / 2.0f, renderY + TILE_SIZE / 2.0f, radius, portalColor);
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
            for (const auto& e : enemies) {
                if (e.isAlive()) {
                    float renderX = (e.getRealX() - cam.x - 0.10f + camShakeX) * TILE_SIZE;
                    bool eFacingLeft = (e.getVelocityX() < 0);

                    if (e.getName() == "Flyer" && flyerTex.id != 0) {
                        float renderY = (e.getRealY() - cam.y + 0.40f + camShakeY) * TILE_SIZE;
                        DrawSpriteFrame(flyerTex, e.getCurrentFrame(), e.getTotalFrames(), renderX, renderY - 10.0f, 1.0f * TILE_SIZE, 1.0f * TILE_SIZE, e.isFlickering() ? RED : WHITE, eFacingLeft);
                    }
                    else if (e.getName() == "Crawler") {
                        Texture2D cTex = crawlerTex[e.getCurrentFrame()];
                        if (cTex.id != 0) {
                            float renderY = (e.getRealY() - cam.y + 0.10f + camShakeY) * TILE_SIZE;
                            DrawSpriteFrame(cTex, 0, 1, renderX, renderY, 1.0f * TILE_SIZE, 1.0f * TILE_SIZE, e.isFlickering() ? RED : WHITE, eFacingLeft);
                        }
                    }
                    else {
                        float renderY = (e.getRealY() - cam.y + 0.40f + camShakeY) * TILE_SIZE;
                        DrawRectangle(renderX, renderY, 0.8f * TILE_SIZE, 0.8f * TILE_SIZE, e.isFlickering() ? ORANGE : RED);
                    }
                }
            }

            float pRenderX = (player.getRealX() - cam.x - 0.25f + camShakeX) * TILE_SIZE;
            float pRenderY = (player.getRealY() - cam.y + camShakeY) * TILE_SIZE;

            // ==========================================
            // 🎨 骑士全新渲染分发 (Knight Renderer)
            // ==========================================
            if (jumpAnimTimer > 0.0f) jumpAnimTimer -= dt;
            if (landAnimTimer > 0.0f) landAnimTimer -= dt;

            vector<Texture2D>* activeFrameArray = &knightStand;
            bool pFlipX = (player.getFacingDirection() == -1);
            int overrideFrame = -1;
            int totalFrames = 2;
            bool useSpecialSingleFrame = false;

            int moveDir = (IsKeyDown(KEY_D) - IsKeyDown(KEY_A));

            if (player.getIsFocusing()) { activeFrameArray = &knightHeal; totalFrames = 1; useSpecialSingleFrame = true; }
            else if (castVisualTimer > 0.0f) { activeFrameArray = &knightCast; totalFrames = 1; useSpecialSingleFrame = true; }
            else if (isAttacking) {
                if (currentAttackType == 1) { activeFrameArray = &knightPogo; totalFrames = 4; }
                else if (currentAttackType == 2) { activeFrameArray = &knightUpAttack; totalFrames = 3; }
                else { activeFrameArray = &knightAttack; totalFrames = 3; }
                overrideFrame = attackFrame;
            }
            else if (player.getIsDashing()) { activeFrameArray = &knightRun; totalFrames = 5; }
            else if (!player.getIsGrounded()) {
                if (jumpAnimTimer > 0.0f) {
                    activeFrameArray = &knightJump; totalFrames = 2;
                    overrideFrame = (jumpAnimTimer > 0.1f) ? 0 : 1;
                }
                else if (player.getVelocityY() < 0.0f) { activeFrameArray = &knightUp; totalFrames = 1; }
                else { activeFrameArray = &knightFall; totalFrames = 1; }
            }
            else if (landAnimTimer > 0.0f && moveDir == 0) {
                activeFrameArray = &knightLand; totalFrames = 2;
                overrideFrame = (landAnimTimer > 0.1f) ? 0 : 1;
            }
            else if (IsKeyDown(KEY_A) || IsKeyDown(KEY_D)) {
                if (IsKeyDown(KEY_LEFT_SHIFT)) { activeFrameArray = &knightRun; totalFrames = 5; }
                else { activeFrameArray = &knightWalk; totalFrames = 4; }
            }
            else {
                activeFrameArray = &knightStand; totalFrames = 2;
            }

            if (activeFrameArray != lastFrameArray) {
                currentFrame = 0;
                frameTimer = 0.0f;
                lastFrameArray = activeFrameArray;
            }

            float currentFrameTime = 0.12f;
            if (activeFrameArray == &knightStand) currentFrameTime = 0.40f;
            else if (activeFrameArray == &knightRun) currentFrameTime = 0.08f;
            else if (activeFrameArray == &knightWalk) currentFrameTime = 0.16f;

            if (overrideFrame == -1) {
                frameTimer += dt;
                if (frameTimer >= currentFrameTime) {
                    currentFrame = (currentFrame + 1) % totalFrames;
                    frameTimer = 0.0f;
                }
            }

            int frameToDraw = (overrideFrame != -1) ? overrideFrame : currentFrame;
            if (frameToDraw >= activeFrameArray->size()) frameToDraw = activeFrameArray->size() - 1;

            Texture2D curFrameTex = (*activeFrameArray)[frameToDraw];

            if (player.getIsFocusing()) {
                float focusPulse = (sin(GetTime() * 15.0f) + 1.0f) * 0.5f;
                DrawCircleLines(pRenderX + TILE_SIZE / 2, pRenderY + TILE_SIZE / 2, TILE_SIZE * focusPulse, { 255, 255, 255, 150 });
            }

            // 轨迹生成逻辑
            if (player.getIsDashing()) {
                dashTrailTimer += dt;
                if (dashTrailTimer >= 0.04f) {
                    dashTrailTimer = 0.0f;
                    dashTrails.push_back({ curFrameTex, 0, 1, player.getRealX(), player.getRealY(), pFlipX, 0.35f, 0.35f });
                }
            }
            else if (activeFrameArray == &knightRun && !isAttacking) {
                runTrailTimer += dt;
                if (runTrailTimer >= 0.06f) {
                    runTrailTimer = 0.0f;
                    dashTrails.push_back({ curFrameTex, 0, 1, player.getRealX(), player.getRealY(), pFlipX, 0.25f, 0.4f });
                }
            }

            for (const auto& trail : dashTrails) {
                float tRenderX = (trail.worldX - cam.x - 0.25f + camShakeX) * TILE_SIZE;
                float tRenderY = (trail.worldY - cam.y + camShakeY) * TILE_SIZE;
                unsigned char alpha = (unsigned char)(200 * (trail.life / trail.maxLife));
                Color trailColor = { 50, 150, 255, alpha };
                DrawSpriteFrame(trail.tex, 0, 1, tRenderX, tRenderY, TILE_SIZE, TILE_SIZE, trailColor, trail.facingLeft);
            }

            // 本体绘制
            if (curFrameTex.id != 0) {
                if (useSpecialSingleFrame) {
                    float energyShake = 1.0f + (sin(GetTime() * 40.0f) * 0.03f);
                    float renderW = TILE_SIZE * 1.5f * energyShake;
                    float renderH = TILE_SIZE * 1.5f * energyShake;
                    float offsetX = pRenderX - (renderW - TILE_SIZE) / 2.0f;
                    float offsetY = pRenderY - (renderH - TILE_SIZE) / 2.0f;

                    DrawTexExact(curFrameTex, offsetX, offsetY, renderW, renderH, WHITE, pFlipX);

                    Color auraColor = (player.getIsFocusing()) ? Color{ 255, 255, 255, 80 } : Color{ 50, 150, 255, 80 };
                    DrawCircle((int)(pRenderX + TILE_SIZE / 2.0f), (int)(pRenderY + TILE_SIZE / 2.0f), TILE_SIZE * 1.2f, auraColor);
                }
                else {
                    DrawTexExact(curFrameTex, pRenderX, pRenderY, TILE_SIZE, TILE_SIZE, player.isFlickering() ? RED : WHITE, pFlipX);
                }
            }
            else {
                DrawRectangle(pRenderX, pRenderY, 0.5f * TILE_SIZE, 0.8f * TILE_SIZE, player.isFlickering() ? SKYBLUE : BLUE);
            }

            // ==========================================
            // ⚔️ 终极刀光渲染管线 (Slash VFX)
            // ==========================================
            if (isAttacking) {
                int vfxFrame = attackFrame;

                if (vfxFrame >= 0 && vfxFrame < 4) {
                    Texture2D currentSlash = slashTex[vfxFrame];
                    if (currentSlash.id != 0) {
                        BeginBlendMode(BLEND_ADDITIVE);

                        float slashW = TILE_SIZE * 2.5f;
                        float slashH = TILE_SIZE * 1.5f;

                        Rectangle srcRec = { 0.0f, 0.0f, (float)currentSlash.width, (float)currentSlash.height };
                        Rectangle destRec;
                        Vector2 origin = { slashW / 2.0f, slashH / 2.0f };

                        int pDir = player.getFacingDirection();
                        float baseRotation = -90.0f;
                        float finalRotation = baseRotation;

                        if (currentAttackType == 1) { // 下劈
                            destRec = { pRenderX + TILE_SIZE / 2.0f, pRenderY + TILE_SIZE * 1.2f, slashW, slashH };
                            finalRotation += 90.0f;
                            destRec.height *= 0.8f;
                        }
                        else if (currentAttackType == 2) { // 上挑
                            destRec = { pRenderX + TILE_SIZE / 2.0f, pRenderY - TILE_SIZE * 0.5f, slashW, slashH };
                            finalRotation -= 90.0f;
                        }
                        else { // 平砍方向锁
                            float offsetX = lockedAttackDir * TILE_SIZE * 1.2f;
                            destRec = { pRenderX + TILE_SIZE / 2.0f + offsetX, pRenderY + TILE_SIZE / 2.0f, slashW, slashH };

                            if (lockedAttackDir == -1) {
                                srcRec.width *= -1.0f;
                                finalRotation = -baseRotation;
                            }
                        }

                        DrawTexturePro(currentSlash, srcRec, destRec, origin, finalRotation, { 200, 230, 255, 230 });
                        EndBlendMode();
                    }
                }
            }

            // ==========================================
            // ✨ 命中十字爆渲染管线 (Hit Impact VFX)
            // ==========================================
            BeginBlendMode(BLEND_ADDITIVE);
            for (auto it = activeImpacts.begin(); it != activeImpacts.end(); ) {
                it->timer += dt;
                if (it->timer > 0.04f) {
                    it->frame++;
                    it->timer = 0.0f;
                }

                if (it->frame >= 4) {
                    it = activeImpacts.erase(it);
                }
                else {
                    Texture2D tex = hitImpactTex[it->frame];
                    if (tex.id != 0) {
                        float renderX = (it->x - cam.x + camShakeX) * TILE_SIZE;
                        float renderY = (it->y - cam.y + camShakeY) * TILE_SIZE;

                        float sizeW = TILE_SIZE * 2.2f;
                        float sizeH = TILE_SIZE * 2.2f;

                        Rectangle srcRec = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
                        Rectangle destRec = { renderX, renderY, sizeW, sizeH };
                        Vector2 origin = { sizeW / 2.0f, sizeH / 2.0f };

                        DrawTexturePro(tex, srcRec, destRec, origin, it->rotation, { 255, 255, 255, 255 });
                    }
                    ++it;
                }
            }
            EndBlendMode();

            if (player.getHealFlashTimer() > 0.0f) {
                unsigned char flashAlpha = (unsigned char)(255.0f * (player.getHealFlashTimer() / 0.3f));
                DrawRectangle(0, 0, screenWidth, screenHeight, { 255, 255, 255, flashAlpha });
            }

            for (const auto& p : hitParticles) {
                float renderX = (p.pos.x - cam.x + camShakeX) * TILE_SIZE;
                float renderY = (p.pos.y - cam.y + camShakeY) * TILE_SIZE;
                unsigned char alpha = (unsigned char)(255 * (p.life / p.maxLife));
                Color renderColor = { p.color.r, p.color.g, p.color.b, alpha };
                DrawRectangle(renderX, renderY, p.size * TILE_SIZE, p.size * TILE_SIZE, renderColor);
            }

            float uiX = 20.0f;
            float uiY = 20.0f;

            DrawRectangle(uiX, uiY, 200, 20, { 50, 20, 20, 200 });
            float hpRatio = fmax(0.0f, (float)player.getHp() / player.getMaxHp());
            DrawRectangle(uiX, uiY, 200 * hpRatio, 20, RED);
            DrawRectangleLines(uiX, uiY, 200, 20, WHITE);
            DrawTextEx(myFont, TextFormat("HP: %d/%d", player.getHp(), player.getMaxHp()), { uiX + 45, uiY + 2 }, 16, 1.0f, WHITE);

            DrawRectangle(uiX, uiY + 25, 150, 12, { 20, 20, 30, 200 });
            float manaRatio = fmax(0.0f, (float)player.getMana() / player.getMaxMana());
            DrawRectangle(uiX, uiY + 25, 150 * manaRatio, 12, WHITE);
            DrawRectangleLines(uiX, uiY + 25, 150, 12, LIGHTGRAY);
            DrawTextEx(myFont, TextFormat("MP: %d/%d", player.getMana(), player.getMaxMana()), { uiX + 160, uiY + 23 }, 16, 1.0f, LIGHTGRAY);

            DrawRectangle(uiX, uiY + 42, 120, 8, { 40, 30, 10, 200 });
            float staRatio = fmax(0.0f, player.getStamina() / player.getMaxStamina());
            DrawRectangle(uiX, uiY + 42, 120 * staRatio, 8, YELLOW);
            DrawRectangleLines(uiX, uiY + 42, 120, 8, GRAY);

            DrawTextEx(myFont, combatLog.c_str(), { uiX, uiY + 60 }, 20, 1.0f, WHITE);
        }
        else if (currentState == SETTINGS) {
            ClearBackground({ 20, 20, 30, 255 });
            DrawTextEx(myFont, "SETTINGS", { (float)(screenWidth / 2 - 100), (float)(screenHeight / 2 - 150) }, 50, 1.0f, LIGHTGRAY);

            Rectangle btnBack = { screenWidth / 2.0f - 60, screenHeight / 2.0f + 160, 120, 50 };
            Rectangle btnBgmDown = { screenWidth / 2.0f + 60, screenHeight / 2.0f - 40, 40, 40 };
            Rectangle btnBgmUp = { screenWidth / 2.0f + 110, screenHeight / 2.0f - 40, 40, 40 };
            Rectangle btnSfxDown = { screenWidth / 2.0f + 60, screenHeight / 2.0f + 30, 40, 40 };
            Rectangle btnSfxUp = { screenWidth / 2.0f + 110, screenHeight / 2.0f + 30, 40, 40 };

            DrawTextEx(myFont, TextFormat("BGM Volume: %.0f%%", bgmVolume * 100), { (float)(screenWidth / 2 - 160), (float)(screenHeight / 2 - 50) }, 22, 1.0f, WHITE);
            DrawRectangleLinesEx(btnBgmDown, 2, GRAY);
            DrawTextEx(myFont, "-", { btnBgmDown.x + 15, btnBgmDown.y + 8 }, 20, 1.0f, GRAY);
            DrawRectangleLinesEx(btnBgmUp, 2, GRAY);
            DrawTextEx(myFont, "+", { btnBgmUp.x + 15, btnBgmUp.y + 8 }, 20, 1.0f, GRAY);

            DrawTextEx(myFont, TextFormat("SFX Volume: %.0f%%", sfxVolume * 100), { (float)(screenWidth / 2 - 160), (float)(screenHeight / 2 + 20) }, 22, 1.0f, WHITE);
            DrawRectangleLinesEx(btnSfxDown, 2, GRAY);
            DrawTextEx(myFont, "-", { btnSfxDown.x + 15, btnSfxDown.y + 8 }, 20, 1.0f, GRAY);
            DrawRectangleLinesEx(btnSfxUp, 2, GRAY);
            DrawTextEx(myFont, "+", { btnSfxUp.x + 15, btnSfxUp.y + 8 }, 20, 1.0f, GRAY);

            DrawRectangleLinesEx(btnBack, 2, GRAY);
            DrawTextEx(myFont, "BACK", { btnBack.x + 30, btnBack.y + 14 }, 20, 1.0f, GRAY);
        }
        else if (currentState == GAME_OVER) {
            DrawTextEx(myFont, "GAME OVER", { (float)(screenWidth / 2 - 150), 150 }, 50, 1.0f, RED);
            DrawTextEx(myFont, "The knight has fallen...", { (float)(screenWidth / 2 - 180), 230 }, 20, 1.0f, LIGHTGRAY);
            DrawTextEx(myFont, "Press ENTER to restart", { (float)(screenWidth / 2 - 160), 320 }, 22, 1.0f, GRAY);
            DrawTextEx(myFont, "Press ESC to return to menu", { (float)(screenWidth / 2 - 180), 360 }, 22, 1.0f, GRAY);
        }

        // PAUSED overlay
        if (currentState == PAUSED) {
            DrawRectangle(0, 0, screenWidth, screenHeight, { 0, 0, 0, 150 });
            DrawTextEx(myFont, "PAUSED", { (float)(screenWidth / 2 - 80), (float)(screenHeight / 2 - 40) }, 50, 1.0f, WHITE);
            DrawTextEx(myFont, "Press ESC to resume", { (float)(screenWidth / 2 - 130), (float)(screenHeight / 2 + 30) }, 18, 1.0f, GRAY);
        }

        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);

        float scale = fmin((float)GetScreenWidth() / 1280.0f, (float)GetScreenHeight() / 720.0f);
        float offsetX = (GetScreenWidth() - (1280.0f * scale)) * 0.5f;
        float offsetY = (GetScreenHeight() - (720.0f * scale)) * 0.5f;

        Rectangle sourceRec = { 0.0f, 0.0f, (float)target.texture.width, (float)-target.texture.height };
        Rectangle destRec = { offsetX, offsetY, 1280.0f * scale, 720.0f * scale };

        DrawTexturePro(target.texture, sourceRec, destRec, { 0, 0 }, 0.0f, WHITE);

        EndDrawing();
    }

    for (auto& m : bgms) UnloadMusicStream(m);
    UnloadRenderTexture(target);
    UnloadSound(sfxSwing);
    UnloadSound(sfxHit);
    UnloadSound(sfxJump);
    UnloadSound(sfxDash);
    UnloadSound(sfxWalk);
    UnloadSound(sfxRun);
    UnloadSound(sfxCast);
    UnloadSound(sfxHeal);
    UnloadFont(myFont);
    UnloadTexture(bgTex);

    // 🌟 卸载全新的骑士散图帧数组
    auto UnloadTexArray = [](vector<Texture2D>& texs) {
        for (auto& t : texs) if (t.id != 0) UnloadTexture(t);
        texs.clear();
        };

    UnloadTexArray(knightStand);
    UnloadTexArray(knightWalk);
    UnloadTexArray(knightRun);
    UnloadTexArray(knightAttack);
    UnloadTexArray(knightUpAttack);
    UnloadTexArray(knightPogo);
    UnloadTexArray(knightJump);
    UnloadTexArray(knightUp);
    UnloadTexArray(knightFall);
    UnloadTexArray(knightLand);
    UnloadTexArray(knightHeal);
    UnloadTexArray(knightCast);

    // 🌟 卸载 4 帧爬虫散图
    for (int i = 0; i < 4; i++) {
        if (crawlerTex[i].id != 0) UnloadTexture(crawlerTex[i]);
    }
    UnloadTexture(flyerTex);
    UnloadTexture(wallTex);
    UnloadTexture(platformTex);
    UnloadTexture(spikeTex);
    UnloadTexture(bgFar);

    // 🌟 卸载新增特效
    for (int i = 0; i < 4; i++) {
        if (slashTex[i].id != 0) UnloadTexture(slashTex[i]);
        if (hitImpactTex[i].id != 0) UnloadTexture(hitImpactTex[i]);
    }

    CloseAudioDevice();
    CloseWindow();
    return 0;
}