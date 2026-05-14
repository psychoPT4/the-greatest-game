#pragma execution_character_set("utf-8")
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
#include <vector>
#include <math.h>
#include <cmath>
#include "map.h"
#include "Player.h"
#include "Enemy.h"
#include "Camera.h"
#include "raylib.h" 
#include <fstream>
#include <sstream>
#include <cstdio>

using namespace std;

enum GameState { MENU, PLAYING, PAUSED, SETTINGS, GAME_OVER, VICTORY, HOW_TO_PLAY };
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

struct HitImpactVFX {
    float x;
    float y;
    int frame;
    float timer;
    float rotation;
};

string combatLog = "Adventure continues...";
string saveMessage = "";
float saveMessageTimer = 0.0f;
float saveFlashTimer = 0.0f;
float safeX, safeY;
const int TILE_SIZE = 48;

// --- Save/Load System ---
const char* SAVE_FILE = "save.dat";
const char SAVE_MAGIC[] = "FKGT";
const int SAVE_VERSION = 2;

static bool saveGame(int currentLevel, const Player& player, const vector<Enemy>& enemies,
    float _safeX, float _safeY, float bgmVol, float sfxVol,
    const string& cLog, int _killCount, bool _ghostUnlocked,
    float _ghostCdTimer, float _playTime) {
    FILE* f = fopen(SAVE_FILE, "wb");
    if (!f) { saveMessage = "Save failed!"; saveMessageTimer = 2.0f; return false; }

    fwrite(SAVE_MAGIC, 4, 1, f);
    fwrite(&SAVE_VERSION, sizeof(int), 1, f);
    fwrite(&currentLevel, sizeof(int), 1, f);

    // 基础实体状态
    float px = player.getRealX(), py = player.getRealY();
    int hp = player.getHp(), maxHp = player.getMaxHp();
    int mana = player.getMana(), maxMana = player.getMaxMana();
    float stamina = player.getStamina(), maxStamina = player.getMaxStamina();
    int facingDir = player.getFacingDirection();
    int jumpCount = player.getJumpCount();
    bool isGrounded = player.getIsGrounded();
    float dashCdTimer = player.getDashCooldownTimer();
    int flickerTimer = player.getFlickerTimer();
    bool isDashing = player.getIsDashing();
    float dashTimer = player.getDashTimer();
    float ignorePlatTimer = player.getIgnorePlatformTimer();
    bool isRunningMode = player.getIsRunningMode();
    float focusTimer = player.getFocusTimer();
    bool isFocusing = player.getIsFocusing();
    float healFlashTimer = player.getHealFlashTimer();
    bool alive = player.isAlive();
    float vx = player.getVelocityX(), vy = player.getVelocityY();

    fwrite(&px, sizeof(float), 1, f); fwrite(&py, sizeof(float), 1, f);
    fwrite(&hp, sizeof(int), 1, f); fwrite(&maxHp, sizeof(int), 1, f);
    fwrite(&mana, sizeof(int), 1, f); fwrite(&maxMana, sizeof(int), 1, f);
    fwrite(&stamina, sizeof(float), 1, f); fwrite(&maxStamina, sizeof(float), 1, f);
    fwrite(&facingDir, sizeof(int), 1, f); fwrite(&jumpCount, sizeof(int), 1, f);
    fwrite(&isGrounded, sizeof(bool), 1, f); fwrite(&dashCdTimer, sizeof(float), 1, f);
    fwrite(&flickerTimer, sizeof(int), 1, f); fwrite(&isDashing, sizeof(bool), 1, f);
    fwrite(&dashTimer, sizeof(float), 1, f); fwrite(&ignorePlatTimer, sizeof(float), 1, f);
    fwrite(&isRunningMode, sizeof(bool), 1, f); fwrite(&focusTimer, sizeof(float), 1, f);
    fwrite(&isFocusing, sizeof(bool), 1, f); fwrite(&healFlashTimer, sizeof(float), 1, f);
    fwrite(&alive, sizeof(bool), 1, f); fwrite(&vx, sizeof(float), 1, f);
    fwrite(&vy, sizeof(float), 1, f);

    fwrite(&_safeX, sizeof(float), 1, f); fwrite(&_safeY, sizeof(float), 1, f);
    fwrite(&bgmVol, sizeof(float), 1, f); fwrite(&sfxVol, sizeof(float), 1, f);

    // 🌟 新增：持久化进阶游戏机制数据
    fwrite(&_killCount, sizeof(int), 1, f);
    fwrite(&_ghostUnlocked, sizeof(bool), 1, f);
    fwrite(&_ghostCdTimer, sizeof(float), 1, f);
    fwrite(&_playTime, sizeof(float), 1, f);

    // 怪物序列化
    int enemyCount = (int)enemies.size();
    fwrite(&enemyCount, sizeof(int), 1, f);
    for (const auto& e : enemies) {
        int eType = e.getEnemyType(); float erx = e.getRealX(), ery = e.getRealY();
        int eHp = e.getHp(), eMaxHp = e.getMaxHp(); bool eAlive = e.isAlive();
        int eFlicker = e.getFlickerTimer(), eMoveDir = e.getMoveDirection();
        int eAtkCd = e.getAttackCooldown(), eFrame = e.getCurrentFrame();
        float eAnimT = e.getAnimTimer(); int eAiState = e.getAiState();
        float eStateT = e.getStateTimer(); int eBossState = e.getBossState();
        bool eEnraged = e.getIsEnraged(); int eMaceDir = e.getMaceSwingDir();

        fwrite(&eType, sizeof(int), 1, f); fwrite(&erx, sizeof(float), 1, f);
        fwrite(&ery, sizeof(float), 1, f); fwrite(&eHp, sizeof(int), 1, f);
        fwrite(&eMaxHp, sizeof(int), 1, f); fwrite(&eAlive, sizeof(bool), 1, f);
        fwrite(&eFlicker, sizeof(int), 1, f); fwrite(&eMoveDir, sizeof(int), 1, f);
        fwrite(&eAtkCd, sizeof(int), 1, f); fwrite(&eFrame, sizeof(int), 1, f);
        fwrite(&eAnimT, sizeof(float), 1, f); fwrite(&eAiState, sizeof(int), 1, f);
        fwrite(&eStateT, sizeof(float), 1, f); fwrite(&eBossState, sizeof(int), 1, f);
        fwrite(&eEnraged, sizeof(bool), 1, f); fwrite(&eMaceDir, sizeof(int), 1, f);
    }

    int logLen = (int)cLog.size();
    fwrite(&logLen, sizeof(int), 1, f);
    fwrite(cLog.c_str(), 1, logLen, f);

    fclose(f);
    saveMessage = "Game Saved!";
    saveMessageTimer = 2.0f; saveFlashTimer = 0.4f;
    return true;
}

static bool loadGame(int& currentLevel, Player& player, vector<Enemy>& enemies,
    Map& gameMap, float& _safeX, float& _safeY,
    float& bgmVol, float& sfxVol, string& cLog, bool& mapLoaded,
    int& _killCount, bool& _ghostUnlocked, float& _ghostCdTimer, float& _playTime) {
    FILE* f = fopen(SAVE_FILE, "rb");
    if (!f) { saveMessage = "No save file found!"; saveMessageTimer = 2.0f; return false; }

    char magic[4]; int version;
    fread(magic, 4, 1, f); fread(&version, sizeof(int), 1, f);
    if (memcmp(magic, SAVE_MAGIC, 4) != 0 || version != SAVE_VERSION) {
        fclose(f); saveMessage = "Invalid save file!"; saveMessageTimer = 2.0f; return false;
    }

    fread(&currentLevel, sizeof(int), 1, f);
    if (!gameMap.loadLevel(currentLevel)) {
        fclose(f); saveMessage = "Failed to load level!"; saveMessageTimer = 2.0f; return false;
    }

    float px, py, stamina, maxStamina, dashCdTimer, dashTimer, ignorePlatTimer;
    float focusTimer, healFlashTimer, vx, vy;
    int hp, maxHp, mana, maxMana, facingDir, jumpCount, flickerTimer;
    bool isGrounded, isDashing, isRunningMode, isFocusing, alive;

    fread(&px, sizeof(float), 1, f); fread(&py, sizeof(float), 1, f);
    fread(&hp, sizeof(int), 1, f); fread(&maxHp, sizeof(int), 1, f);
    fread(&mana, sizeof(int), 1, f); fread(&maxMana, sizeof(int), 1, f);
    fread(&stamina, sizeof(float), 1, f); fread(&maxStamina, sizeof(float), 1, f);
    fread(&facingDir, sizeof(int), 1, f); fread(&jumpCount, sizeof(int), 1, f);
    fread(&isGrounded, sizeof(bool), 1, f); fread(&dashCdTimer, sizeof(float), 1, f);
    fread(&flickerTimer, sizeof(int), 1, f); fread(&isDashing, sizeof(bool), 1, f);
    fread(&dashTimer, sizeof(float), 1, f); fread(&ignorePlatTimer, sizeof(float), 1, f);
    fread(&isRunningMode, sizeof(bool), 1, f); fread(&focusTimer, sizeof(float), 1, f);
    fread(&isFocusing, sizeof(bool), 1, f); fread(&healFlashTimer, sizeof(float), 1, f);
    fread(&alive, sizeof(bool), 1, f); fread(&vx, sizeof(float), 1, f);
    fread(&vy, sizeof(float), 1, f);

    player.restoreState(px, py, hp, maxHp, mana, maxMana, stamina, maxStamina,
        facingDir, jumpCount, isGrounded, dashCdTimer, flickerTimer,
        isDashing, dashTimer, ignorePlatTimer, isRunningMode,
        focusTimer, isFocusing, healFlashTimer, alive, vx, vy);

    fread(&_safeX, sizeof(float), 1, f); fread(&_safeY, sizeof(float), 1, f);
    fread(&bgmVol, sizeof(float), 1, f); fread(&sfxVol, sizeof(float), 1, f);

    // 🌟 新增：安全还原进阶机制数据
    fread(&_killCount, sizeof(int), 1, f);
    fread(&_ghostUnlocked, sizeof(bool), 1, f);
    fread(&_ghostCdTimer, sizeof(float), 1, f);
    fread(&_playTime, sizeof(float), 1, f);

    int enemyCount; fread(&enemyCount, sizeof(int), 1, f);
    enemies.clear();
    for (int i = 0; i < enemyCount; i++) {
        int eType, eHp, eMaxHp, eFlicker, eMoveDir, eAtkCd, eFrame;
        int eAiState, eBossState, eMaceDir; float erx, ery, eAnimT, eStateT;
        bool eAlive, eEnraged;

        fread(&eType, sizeof(int), 1, f); fread(&erx, sizeof(float), 1, f);
        fread(&ery, sizeof(float), 1, f); fread(&eHp, sizeof(int), 1, f);
        fread(&eMaxHp, sizeof(int), 1, f); fread(&eAlive, sizeof(bool), 1, f);
        fread(&eFlicker, sizeof(int), 1, f); fread(&eMoveDir, sizeof(int), 1, f);
        fread(&eAtkCd, sizeof(int), 1, f); fread(&eFrame, sizeof(int), 1, f);
        fread(&eAnimT, sizeof(float), 1, f); fread(&eAiState, sizeof(int), 1, f);
        fread(&eStateT, sizeof(float), 1, f); fread(&eBossState, sizeof(int), 1, f);
        fread(&eEnraged, sizeof(bool), 1, f); fread(&eMaceDir, sizeof(int), 1, f);

        enemies.emplace_back(0, 0, eType);
        enemies.back().restoreState(erx, ery, eHp, eMaxHp, eAlive,
            eFlicker, eMoveDir, eAtkCd, eFrame, eAnimT,
            eAiState, eStateT, eBossState, eEnraged, eMaceDir);
    }

    int logLen; fread(&logLen, sizeof(int), 1, f);
    if (logLen > 0 && logLen < 1000) {
        vector<char> buf(logLen + 1); fread(buf.data(), 1, logLen, f);
        buf[logLen] = '\0'; cLog = buf.data();
    }

    fclose(f); mapLoaded = true;
    saveMessage = "Game Loaded!"; saveMessageTimer = 2.0f;
    return true;
}

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
    bgms.push_back(LoadMusicStream("bgm3.mp3")); // 🌟 新增：第三关专属高燃决战 BGM！
    for (auto& m : bgms) SetMusicVolume(m, 0.4f);
    SetMusicVolume(bgms[0], 0.5f);
    SetMusicVolume(bgms[1], 0.6f);
    SetMusicVolume(bgms[3], 0.5f);
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

    vector<Texture2D> knightAttack;
    for (int i = 0; i < 3; i++) knightAttack.push_back(LoadTexture(TextFormat("attack%d.png", i)));

    vector<Texture2D> knightUpAttack;
    for (int i = 0; i < 3; i++) knightUpAttack.push_back(LoadTexture(TextFormat("upattack%d.png", i)));

    vector<Texture2D> knightPogo;
    for (int i = 0; i < 4; i++) knightPogo.push_back(LoadTexture(TextFormat("pogo%d.png", i)));

    vector<Texture2D> knightJump;
    for (int i = 0; i < 2; i++) knightJump.push_back(LoadTexture(TextFormat("jump%d.png", i)));

    vector<Texture2D> knightUp;
    knightUp.push_back(LoadTexture("up.png"));

    vector<Texture2D> knightFall;
    knightFall.push_back(LoadTexture("fall.png"));

    vector<Texture2D> knightLand;
    for (int i = 0; i < 2; i++) knightLand.push_back(LoadTexture(TextFormat("land%d.png", i)));

    vector<Texture2D> knightHeal;
    knightHeal.push_back(LoadTexture("heal.png"));
    vector<Texture2D> knightCast;
    knightCast.push_back(LoadTexture("cast_spell.png"));

    // ==========================================
    // 🌟 假骑士 Boss 专属 5 张精准动作大图
    // ==========================================
    Texture2D texBossStand = LoadTexture("boss_stand.png");
    Texture2D texBossJump = LoadTexture("boss_jump.png");
    Texture2D texBossAir = LoadTexture("boss_air.png");
    Texture2D texBossSwing = LoadTexture("boss_swing.png"); // 蓄力后仰
    Texture2D texBossHit = LoadTexture("boss_hit.png");   // 锤击地面释放冲击波！

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

    Rectangle btnNewGame = { screenWidth / 2.0f - 100, screenHeight / 2.0f - 20, 200, 50 };
    Rectangle btnLoadGame = { screenWidth / 2.0f - 100, screenHeight / 2.0f + 45, 200, 50 };
    Rectangle btnSettings = { screenWidth / 2.0f - 100, screenHeight / 2.0f + 110, 200, 50 };
    Rectangle btnExit = { screenWidth / 2.0f - 100, screenHeight / 2.0f + 175, 200, 50 };
    ifstream saveCheck(SAVE_FILE, ios::binary);
    bool saveFileExists = saveCheck.good();
    saveCheck.close();

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
    int currentAttackType = 0;
    float castVisualTimer = 0.0f;

    float stepTimer = 0.4f;
    bool lastJump = false;
    bool wasGroundedLastFrame = true;
    bool facingLeft = false;
    float frameTimer = 0.0f;
    int currentFrame = 0;
    bool isFacingLeftNow = false;
    float dashTrailTimer = 0.0f;
    float runTrailTimer = 0.0f;
    float playTime = 0.0f;
    int killCount = 0;
    bool ghostUnlocked = false;
    bool isGhosting = false;
    float ghostTimer = 0.0f;
    float ghostCooldownTimer = 0.0f;
    const float GHOST_DURATION = 0.6f;
    const float GHOST_COOLDOWN = 4.0f;
    const int GHOST_UNLOCK_KILLS = 24;
    float fadeAlpha = 0.0f;
    bool isFadingOut = true;
    bool isTransitioning = false;
    int pendingNextLevel = 0;
    float bgmVolume = 0.5f;
    float sfxVolume = 1.0f;

    float jumpAnimTimer = 0.0f;
    float landAnimTimer = 0.0f;
    vector<Texture2D>* lastFrameArray = nullptr;

    while (!WindowShouldClose()) {

        UpdateMusicStream(bgms[currentBgmIndex]);
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;
        if (saveMessageTimer > 0.0f) saveMessageTimer -= dt;
        if (saveFlashTimer > 0.0f) saveFlashTimer -= dt;
        if (currentState == PLAYING) playTime += dt;

        if (!ghostUnlocked && killCount >= GHOST_UNLOCK_KILLS) {
            ghostUnlocked = true;
            combatLog = "虚化能力解锁！按 Q 进入虚化状态";
        }
        if (isGhosting) {
            ghostTimer -= dt;
            if (ghostTimer <= 0.0f) { isGhosting = false; ghostTimer = 0.0f; }
        }
        player.setInvincible(isGhosting);
        if (ghostCooldownTimer > 0.0f) ghostCooldownTimer -= dt;

        if (isTransitioning) {
            const float FADE_SPEED = 2.8f;
            if (isFadingOut) {
                fadeAlpha += FADE_SPEED * dt;
                if (fadeAlpha >= 1.0f) {
                    fadeAlpha = 1.0f;
                    if (pendingNextLevel == -1) {
                        currentState = VICTORY;
                        StopMusicStream(bgms[currentBgmIndex]);
                        isTransitioning = false;
                    } else {
                        currentLevel = pendingNextLevel;
                        mapLoaded = false;
                        StopMusicStream(bgms[currentBgmIndex]);
                        currentBgmIndex = (currentLevel < (int)bgms.size()) ? currentLevel : (int)bgms.size() - 1;
                        PlayMusicStream(bgms[currentBgmIndex]);
                        isFadingOut = false;
                    }
                }
            } else {
                fadeAlpha -= FADE_SPEED * dt;
                if (fadeAlpha <= 0.0f) {
                    fadeAlpha = 0.0f;
                    isTransitioning = false;
                }
            }
        }

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

            if (saveFileExists && CheckCollisionPointRec(mousePos, btnLoadGame) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                StopMusicStream(bgms[currentBgmIndex]);
                bool loaded = loadGame(currentLevel, player, enemies, gameMap, safeX, safeY, bgmVolume, sfxVolume, combatLog, mapLoaded, killCount, ghostUnlocked, ghostCooldownTimer, playTime);
                if (loaded) {
                    for (auto& m : bgms) SetMusicVolume(m, bgmVolume * 0.8f);
                    SetSoundVolume(sfxSwing, sfxVolume); SetSoundVolume(sfxHit, sfxVolume);
                    SetSoundVolume(sfxJump, sfxVolume); SetSoundVolume(sfxLand, sfxVolume);
                    SetSoundVolume(sfxDash, sfxVolume); SetSoundVolume(sfxWalk, sfxVolume);
                    SetSoundVolume(sfxRun, sfxVolume); SetSoundVolume(sfxCast, sfxVolume);
                    SetSoundVolume(sfxHeal, sfxVolume);
                    currentBgmIndex = (currentLevel < (int)bgms.size()) ? currentLevel : (int)bgms.size() - 1;
                    PlayMusicStream(bgms[currentBgmIndex]);
                    isAttacking = false; isPogoAttacking = false; isUpAttacking = false;
                    attackCd = 0; attackFrame = 0;
                    stepTimer = 0.4f; lastJump = false; wasGroundedLastFrame = true;
                    frameTimer = 0.0f; currentFrame = 0;
                    lastFrameArray = nullptr;
                    dashTrailTimer = 0.0f; runTrailTimer = 0.0f;
                    jumpAnimTimer = 0.0f; landAnimTimer = 0.0f;
                    castVisualTimer = 0.0f;
                    hitParticles.clear(); dashTrails.clear(); activeImpacts.clear();
                    currentState = PLAYING;
                }
            }
            if (CheckCollisionPointRec(mousePos, btnNewGame) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                currentLevel = 1; playTime = 0.0f; killCount = 0; ghostUnlocked = false;
                currentState = HOW_TO_PLAY; mapLoaded = false;
                StopMusicStream(bgms[currentBgmIndex]);
                currentBgmIndex = 0;
                PlayMusicStream(bgms[currentBgmIndex]);
            }
            if (CheckCollisionPointRec(mousePos, btnSettings) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                currentState = SETTINGS;
            }
            if (CheckCollisionPointRec(mousePos, btnExit) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                break;
            }
            if (IsKeyPressed(KEY_ENTER)) {
                currentLevel = 1; playTime = 0.0f; killCount = 0; ghostUnlocked = false;
                currentState = HOW_TO_PLAY; mapLoaded = false;
                StopMusicStream(bgms[currentBgmIndex]);
                currentBgmIndex = 0;
                PlayMusicStream(bgms[currentBgmIndex]);
            }
        }
        else if (currentState == HOW_TO_PLAY) {
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_J) ||
                IsKeyPressed(KEY_K) || IsKeyPressed(KEY_L) || IsKeyPressed(KEY_A) ||
                IsKeyPressed(KEY_D) || IsKeyPressed(KEY_W) || IsKeyPressed(KEY_S) ||
                IsKeyPressed(KEY_ESCAPE)) {
                currentState = PLAYING;
            }
        }
        else if (currentState == PLAYING) {
            bool frozen = (isTransitioning && isFadingOut);
            if (!frozen && IsKeyPressed(KEY_ESCAPE)) {
                currentState = PAUSED;
            }
            if (saveGame(currentLevel, player, enemies, safeX, safeY, bgmVolume, sfxVolume, combatLog, killCount, ghostUnlocked, ghostCooldownTimer, playTime)) {
                saveFileExists = true;
            }
            if (!frozen && IsKeyPressed(KEY_F9)) {
                StopMusicStream(bgms[currentBgmIndex]);
                bool loaded = loadGame(currentLevel, player, enemies, gameMap, safeX, safeY, bgmVolume, sfxVolume, combatLog, mapLoaded, killCount, ghostUnlocked, ghostCooldownTimer, playTime);
                if (loaded) {
                    for (auto& m : bgms) SetMusicVolume(m, bgmVolume * 0.8f);
                    SetSoundVolume(sfxSwing, sfxVolume); SetSoundVolume(sfxHit, sfxVolume);
                    SetSoundVolume(sfxJump, sfxVolume); SetSoundVolume(sfxLand, sfxVolume);
                    SetSoundVolume(sfxDash, sfxVolume); SetSoundVolume(sfxWalk, sfxVolume);
                    SetSoundVolume(sfxRun, sfxVolume); SetSoundVolume(sfxCast, sfxVolume);
                    SetSoundVolume(sfxHeal, sfxVolume);
                    currentBgmIndex = (currentLevel < (int)bgms.size()) ? currentLevel : (int)bgms.size() - 1;
                    PlayMusicStream(bgms[currentBgmIndex]);
                    isAttacking = false; isPogoAttacking = false; isUpAttacking = false;
                    attackCd = 0; attackFrame = 0;
                    stepTimer = 0.4f; lastJump = false; wasGroundedLastFrame = true;
                    frameTimer = 0.0f; currentFrame = 0;
                    lastFrameArray = nullptr;
                    dashTrailTimer = 0.0f; runTrailTimer = 0.0f;
                    jumpAnimTimer = 0.0f; landAnimTimer = 0.0f;
                    castVisualTimer = 0.0f;
                    hitParticles.clear(); dashTrails.clear(); activeImpacts.clear();
                }
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
                    // 🌟 假设关卡里放了 Boss (比如你在 map3.csv 放了类型 12)
                    else if (s.type == 12) enemies.emplace_back(s.x, s.y, 2);
                }
                mapLoaded = true;

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

            player.updateProjectiles(enemies, gameMap, dt, killCount);

            if (!frozen && IsKeyPressed(KEY_L)) {
                if (player.tryDash()) PlaySound(sfxDash);
            }

            if (!frozen && IsKeyPressed(KEY_Q) && ghostUnlocked && !isGhosting && ghostCooldownTimer <= 0.0f) {
                isGhosting = true;
                ghostTimer = GHOST_DURATION;
                ghostCooldownTimer = GHOST_COOLDOWN;
                combatLog = "虚化！ ";
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

            bool jump = IsKeyDown(KEY_K);
            if (jump && !lastJump) {
                if (player.getIsGrounded() || player.getJumpCount() < 2) {
                    PlaySound(sfxJump);
                    jumpAnimTimer = 0.2f;
                }
            }
            player.processJump(jump && !lastJump, jump, IsKeyDown(KEY_S));
            lastJump = jump;

            if (IsKeyPressed(KEY_J) && !isAttacking && !player.getIsDashing() && attackCd <= 0) {
                PlaySound(sfxSwing);
                isAttacking = true;
                lockedAttackDir = player.getFacingDirection();
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

                int totalFrames = 3;
                if (currentAttackType == 1) totalFrames = 4;

                bool isActiveFrame = (attackFrame >= 1 && attackFrame <= 2);

                if (isActiveFrame) {
                    AttackResult result = player.attack(enemies, gameMap, currentAttackType, lockedAttackDir, killCount);

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

                // 🌟 统一更新怪物逻辑，捕获假骑士引发的极致震屏
                for (auto& e : enemies) {
                    e.update(gameMap, player, dt);
                    // 如果假骑士刚刚猛烈砸地 (bossState == 4 刚触发)，或者落地产生强大后座力
                    if (e.getName() == "False Knight") {
                        int bState = e.getBossState();
                        // 借用 stateTimer 刚进入某状态的时间点判定震屏
                        if (bState == 4 && e.isFlickering()) {
                            camShakeY = 0.35f; // 大震感
                        }
                    }
                }

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
            bool bossAliveOnLevel3 = false;
            if (currentLevel == 3) {
                for (const auto& e : enemies) {
                    if (e.getEnemyType() == 2 && e.isAlive()) { bossAliveOnLevel3 = true; break; }
                }
            }
            if (player.getX() == goal.x && player.getY() == goal.y && !bossAliveOnLevel3 && !isTransitioning) {
                isTransitioning = true;
                isFadingOut = true;
                fadeAlpha = 0.0f;
                if (currentLevel >= 3) {
                    pendingNextLevel = -1; // -1 means VICTORY
                } else {
                    pendingNextLevel = currentLevel + 1;
                }
            }

            if (!player.isAlive()) {
                currentState = GAME_OVER;
            }
        }
        else if (currentState == PAUSED) {
            if (IsKeyPressed(KEY_ESCAPE)) {
                currentState = PLAYING;
            }
            if (IsKeyPressed(KEY_F5)) {
                saveGame(currentLevel, player, enemies, safeX, safeY, bgmVolume, sfxVolume, combatLog, killCount, ghostUnlocked, ghostCooldownTimer, playTime)
                    ;
            }
            if (IsKeyPressed(KEY_F9)) {
                StopMusicStream(bgms[currentBgmIndex]);
                bool loaded = loadGame(currentLevel, player, enemies, gameMap, safeX, safeY, bgmVolume, sfxVolume, combatLog, mapLoaded, killCount, ghostUnlocked, ghostCooldownTimer, playTime);
                if (loaded) {
                    for (auto& m : bgms) SetMusicVolume(m, bgmVolume * 0.8f);
                    SetSoundVolume(sfxSwing, sfxVolume); SetSoundVolume(sfxHit, sfxVolume);
                    SetSoundVolume(sfxJump, sfxVolume); SetSoundVolume(sfxLand, sfxVolume);
                    SetSoundVolume(sfxDash, sfxVolume); SetSoundVolume(sfxWalk, sfxVolume);
                    SetSoundVolume(sfxRun, sfxVolume); SetSoundVolume(sfxCast, sfxVolume);
                    SetSoundVolume(sfxHeal, sfxVolume);
                    currentBgmIndex = (currentLevel < (int)bgms.size()) ? currentLevel : (int)bgms.size() - 1;
                    PlayMusicStream(bgms[currentBgmIndex]);
                    isAttacking = false; isPogoAttacking = false; isUpAttacking = false;
                    attackCd = 0; attackFrame = 0;
                    stepTimer = 0.4f; lastJump = false; wasGroundedLastFrame = true;
                    frameTimer = 0.0f; currentFrame = 0;
                    lastFrameArray = nullptr;
                    dashTrailTimer = 0.0f; runTrailTimer = 0.0f;
                    jumpAnimTimer = 0.0f; landAnimTimer = 0.0f;
                    castVisualTimer = 0.0f;
                    hitParticles.clear(); dashTrails.clear(); activeImpacts.clear();
                    currentState = PLAYING;
                }
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
                currentLevel = 1; playTime = 0.0f; killCount = 0; ghostUnlocked = false;
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
        else if (currentState == VICTORY) {
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
                currentState = MENU;
                currentLevel = 1; playTime = 0.0f;
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

            // NEW GAME - always available
            bool hoverNew = CheckCollisionPointRec(mousePos, btnNewGame);
            DrawRectangleRec(btnNewGame, hoverNew ? DARKGRAY : BLANK);
            DrawRectangleLinesEx(btnNewGame, 2, hoverNew ? WHITE : GRAY);
            DrawTextEx(myFont, "NEW GAME", { (float)(btnNewGame.x + 45), (float)(btnNewGame.y + 15) }, 20, 1.0f, hoverNew ? WHITE : GRAY);

            // LOAD GAME - grayed out when no save, lit when save exists
            {
                bool hoverLoad = saveFileExists && CheckCollisionPointRec(mousePos, btnLoadGame);
                Color darkGray = {60, 60, 60, 255};
                Color loadLine = saveFileExists ? (hoverLoad ? WHITE : GRAY) : darkGray;
                Color loadText = saveFileExists ? (hoverLoad ? WHITE : GRAY) : darkGray;
                DrawRectangleRec(btnLoadGame, hoverLoad ? DARKGRAY : BLANK);
                DrawRectangleLinesEx(btnLoadGame, 2, loadLine);
                DrawTextEx(myFont, "LOAD GAME", { (float)(btnLoadGame.x + 45), (float)(btnLoadGame.y + 15) }, 20, 1.0f, loadText);
            }

            bool hoverSettings = CheckCollisionPointRec(mousePos, btnSettings);
            DrawRectangleRec(btnSettings, hoverSettings ? DARKGRAY : BLANK);
            DrawRectangleLinesEx(btnSettings, 2, hoverSettings ? WHITE : GRAY);
            DrawTextEx(myFont, "SETTINGS", { (float)(btnSettings.x + 55), (float)(btnSettings.y + 15) }, 20, 1.0f, hoverSettings ? WHITE : GRAY);

            bool hoverExit = CheckCollisionPointRec(mousePos, btnExit);
            DrawRectangleRec(btnExit, hoverExit ? DARKGRAY : BLANK);
            DrawRectangleLinesEx(btnExit, 2, hoverExit ? WHITE : GRAY);
            DrawTextEx(myFont, "EXIT", { (float)(btnExit.x + 80), (float)(btnExit.y + 15) }, 20, 1.0f, hoverExit ? WHITE : GRAY);
        }
        else if (currentState == HOW_TO_PLAY) {
            ClearBackground({ 15, 12, 25, 255 });

            float titleY = 40.0f;
            DrawTextEx(myFont, "CONTROLS", { (float)(screenWidth / 2 - 120), titleY }, 48, 1.0f, GOLD);

            float startY = 120.0f;
            float lineH = 38.0f;
            float col1X = 160.0f;
            float col2X = 500.0f;

            struct KeyInfo { const char* key; const char* action; };
            KeyInfo keys[] = {
                {"A / D", "Move Left / Right"},
                {"W", "Look Up"},
                {"S (on ground)", "Drop through platform"},
                {"S (in air)", "Pogo Down-Slash"},
                {"K", "Jump (Double Jump in air)"},
                {"J", "Attack"},
                {"W + J", "Upward Slash"},
                {"S + J (in air)", "Downward Pogo Slash"},
                {"L", "Dash"},
                {"I (hold 1s)", "Focus Heal (25 HP, costs 33 MP)"},
                {"I (tap)", "Soul Blast (costs 33 MP)"},
                {"Shift", "Run"},
                {"F5", "Quick Save"},
                {"F9", "Quick Load"},
                {"ESC", "Pause"},
            };
            int keyCount = sizeof(keys) / sizeof(keys[0]);

            for (int i = 0; i < keyCount; i++) {
                float y = startY + i * lineH;
                Color keyColor = (i % 2 == 0) ? WHITE : LIGHTGRAY;
                DrawTextEx(myFont, keys[i].key, { col1X, y }, 18, 1.0f, GOLD);
                DrawTextEx(myFont, keys[i].action, { col2X, y }, 18, 1.0f, keyColor);
            }

            float pulse = (sin(GetTime() * 3.0f) + 1.0f) * 0.5f;
            const char* prompt = "Press any key to begin your journey...";
            float promptW = MeasureTextEx(myFont, prompt, 24, 1.0f).x;
            DrawTextEx(myFont, prompt,
                { screenWidth / 2.0f - promptW / 2.0f, startY + keyCount * lineH + 40 },
                24, 1.0f, { 200, 200, 200, (unsigned char)(120 + 80 * pulse) });

            // Decorative knight silhouette hint
            DrawTextEx(myFont, "A knight's duty is never fulfilled...",
                { screenWidth / 2.0f - 210, screenHeight - 60.0f }, 16, 1.0f,
                { 150, 150, 150, 100 });
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

                    // 在 main.cpp 渲染 Wall 的地方核对这段逻辑：
                    if (t == TileType::Wall) {
                        TileType tileAbove = gameMap.getTileAt(worldX, worldY - 1);

                        // 🌟 核心排雷：如果上方是空气，才画带草皮的顶层地面
                        // 如果上方是 SpikeUp (地刺)，说明这是陷阱底座，直接画深层纯泥土！
                        if (tileAbove == TileType::Empty) {
                            Texture2D tex = gameMap.getTexGroundTop();
                            if (tex.id != 0) DrawTexExact(tex, renderX, renderY, TILE_SIZE, TILE_SIZE);
                            else DrawRectangle(renderX, renderY, TILE_SIZE, TILE_SIZE, DARKBROWN); // 就算没图也画褐色，拒绝纯绿！
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
                    bool showPortal = (worldX == goal.x && worldY == goal.y);
                    if (showPortal && currentLevel == 3) {
                        bool anyBossAlive = false;
                        for (const auto& e : enemies) {
                            if (e.getEnemyType() == 2 && e.isAlive()) { anyBossAlive = true; break; }
                        }
                        if (anyBossAlive) showPortal = false;
                    }
                    if (showPortal) {
                        float pulse = (sin(GetTime() * 5.0f) + 1.0f) * 0.5f;
                        Color portalColor = { 255, 255, 150, (unsigned char)(150 + 105 * pulse) };
                        float radius = (0.35f + 0.1f * pulse) * TILE_SIZE;

                        DrawCircle(renderX + TILE_SIZE / 2.0f, renderY + TILE_SIZE / 2.0f, radius, portalColor);
                        DrawCircleLines(renderX + TILE_SIZE / 2.0f, renderY + TILE_SIZE / 2.0f, radius + 2, YELLOW);
                    }
                    else if (worldX == goal.x && worldY == goal.y && currentLevel == 3) {
                        float pulse = (sin(GetTime() * 3.0f) + 1.0f) * 0.5f;
                        DrawCircleLines(renderX + TILE_SIZE / 2.0f, renderY + TILE_SIZE / 2.0f,
                            0.3f * TILE_SIZE, { 100, 100, 100, (unsigned char)(80 + 40 * pulse) });
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

            // ==========================================
            // 🛡️ 实体/怪物/Boss 全景渲染管线
            // ==========================================
            for (auto& e : enemies) {
                if (e.isAlive()) {
                    float renderX = (e.getRealX() - cam.x + camShakeX) * TILE_SIZE;
                    float renderY = (e.getRealY() - cam.y + camShakeY) * TILE_SIZE;
                    bool eFacingLeft = (e.getVelocityX() < 0);

                    if (e.getName() == "Flyer" && flyerTex.id != 0) {
                        DrawSpriteFrame(flyerTex, e.getCurrentFrame(), e.getTotalFrames(), renderX - 0.1f * TILE_SIZE, renderY + 0.4f * TILE_SIZE - 10.0f, 1.0f * TILE_SIZE, 1.0f * TILE_SIZE, e.isFlickering() ? RED : WHITE, eFacingLeft);
                    }
                    else if (e.getName() == "Crawler") {
                        Texture2D cTex = crawlerTex[e.getCurrentFrame()];
                        if (cTex.id != 0) {
                            DrawSpriteFrame(cTex, 0, 1, renderX - 0.1f * TILE_SIZE, renderY + 0.1f * TILE_SIZE, 1.0f * TILE_SIZE, 1.0f * TILE_SIZE, e.isFlickering() ? RED : WHITE, eFacingLeft);
                        }
                    }
                    // 🌟 假骑士 False Knight 5张动作分流渲染
                    else if (e.getName() == "False Knight") {
                        int state = e.getBossState();
                        Texture2D activeBossTex = texBossStand; // 默认

                        if (state == 1) activeBossTex = texBossJump;
                        else if (state == 2) activeBossTex = texBossAir;
                        else if (state == 3) activeBossTex = texBossSwing; // 举锤蓄力
                        else if (state == 4) activeBossTex = texBossHit;   // 轰击地面释放冲击波

                        if (activeBossTex.id != 0) {
                            // 3:2 宽高比例放大映射：绘制宽度约 4.5 格，高度约 3.0 格
                            float drawW = TILE_SIZE * 4.5f;
                            float drawH = TILE_SIZE * 3.0f;
                            bool flipBoss = (e.getVelocityX() > 0); // 原图朝左，向右跑时需翻转

                            // 完美中心锚点映射在两脚之间的底部地面
                            DrawTexExact(activeBossTex, renderX - drawW / 2.0f, renderY - drawH, drawW, drawH, e.isFlickering() ? RED : WHITE, flipBoss);
                        }
                        else {
                            // 缺图兜底紫红色威压巨兽
                            DrawRectangle(renderX - 1.25f * TILE_SIZE, renderY - 3.0f * TILE_SIZE, 2.5f * TILE_SIZE, 3.0f * TILE_SIZE, e.isFlickering() ? RED : PURPLE);
                        }

                        // 🌟 渲染由 Boss 释放的独立滚动冲击波 (Shockwaves)
                        for (auto& wave : e.getShockwaves()) {
                            float wRenderX = (wave.x - cam.x + camShakeX) * TILE_SIZE;
                            float wRenderY = (wave.y - cam.y + camShakeY) * TILE_SIZE;
                            float waveH = wave.height * TILE_SIZE; // 1.8格高的死神弧线

                            // 借助半透明月弧营造极速冲击效果
                            DrawEllipse(wRenderX, wRenderY - waveH / 2.0f, 0.4f * TILE_SIZE, waveH / 2.0f, { 255, 255, 255, 220 });
                            DrawEllipseLines(wRenderX, wRenderY - waveH / 2.0f, 0.45f * TILE_SIZE, waveH / 2.0f + 2, SKYBLUE);
                        }
                    }
                    else {
                        DrawRectangle(renderX - 0.1f * TILE_SIZE, renderY + 0.4f * TILE_SIZE, 0.8f * TILE_SIZE, 0.8f * TILE_SIZE, e.isFlickering() ? ORANGE : RED);
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
                else if (isGhosting) {
                    // Obito-style ghost phasing effect
                    float ghostRatio = ghostTimer / GHOST_DURATION;
                    unsigned char baseAlpha = (unsigned char)(120 + 40 * ghostRatio);
                    Color ghostTint = { 120, 200, 255, baseAlpha };

                    // Phasing offset copies (Kamui displacement)
                    float phaseOffset = (1.0f - ghostRatio) * 15.0f;
                    for (int g = 3; g >= 0; g--) {
                        float gox = pRenderX + phaseOffset * (g - 1.5f) * 0.5f;
                        unsigned char ga = (unsigned char)(baseAlpha * (0.2f + 0.2f * g));
                        DrawTexExact(curFrameTex, gox, pRenderY, TILE_SIZE, TILE_SIZE, {120, 200, 255, ga}, pFlipX);
                    }

                    // Main ghost body
                    DrawTexExact(curFrameTex, pRenderX, pRenderY, TILE_SIZE, TILE_SIZE, ghostTint, pFlipX);

                    // Swirling energy particles (Kamui spiral)
                    float cx = pRenderX + TILE_SIZE / 2.0f;
                    float cy = pRenderY + TILE_SIZE / 2.0f;
                    int particleCount = 18;
                    for (int i = 0; i < particleCount; i++) {
                        float angle = GetTime() * 8.0f + i * (PI * 2.0f / particleCount);
                        float radius = TILE_SIZE * (0.5f + 0.3f * sin(GetTime() * 5.0f + i * 0.6f));
                        float px = cx + cos(angle) * radius;
                        float py = cy + sin(angle) * radius * 0.6f;
                        float pSize = 2.0f + 1.5f * sin(GetTime() * 6.0f + i);
                        unsigned char pa = (unsigned char)(180 + 75 * sin(GetTime() * 4.0f + i));
                        DrawCircle((int)px, (int)py, pSize, {100, 180, 255, pa});
                    }

                    // Inner glow ring
                    DrawCircleLines((int)cx, (int)cy, TILE_SIZE * 0.8f, {100, 180, 255, (unsigned char)(80 + 60 * ghostRatio)});
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

            if (saveFlashTimer > 0.0f) {
                float ratio = saveFlashTimer / 0.4f;
                unsigned char boxAlpha = (unsigned char)(60.0f * ratio);
                DrawRectangle(0, 0, screenWidth, screenHeight, { 255, 255, 200, boxAlpha });

                float textScale = 1.0f + (1.0f - ratio) * 0.8f;
                unsigned char textAlpha = (unsigned char)(255.0f * ratio);
                const char* msg = "GAME SAVED";
                float msgW = MeasureTextEx(myFont, msg, 48 * textScale, 1.0f).x;
                DrawTextEx(myFont, msg,
                    { screenWidth / 2.0f - msgW / 2.0f, screenHeight / 2.0f - 30 },
                    48 * textScale, 1.0f, { 255, 255, 100, textAlpha });
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

            // Boss HP bar (top center)
            for (const auto& e : enemies) {
                if (e.getEnemyType() == 2 && e.isAlive()) {
                    float bossBarW = 500.0f;
                    float bossBarH = 28.0f;
                    float bossBarX = screenWidth / 2.0f - bossBarW / 2.0f;
                    float bossBarY = 12.0f;

                    float bossHpRatio = fmax(0.0f, (float)e.getHp() / e.getMaxHp());
                    Color bossBarColor = e.getIsEnraged() ? Color{220, 40, 40, 255} : Color{180, 30, 30, 255};
                    Color bossFillColor = e.getIsEnraged() ? Color{255, 60, 20, 255} : Color{220, 50, 30, 255};

                    DrawRectangle(bossBarX, bossBarY, bossBarW, bossBarH, { 30, 10, 10, 230 });
                    DrawRectangle(bossBarX + 2, bossBarY + 2, (bossBarW - 4) * bossHpRatio, bossBarH - 4, bossFillColor);

                    if (e.getIsEnraged()) {
                        float ragePulse = (sin(GetTime() * 12.0f) + 1.0f) * 0.5f;
                        DrawRectangle(bossBarX + 2, bossBarY + 2, (bossBarW - 4) * bossHpRatio, bossBarH - 4,
                            { 255, (unsigned char)(40 + 40 * ragePulse), 10, 100 });
                    }

                    DrawRectangleLinesEx({ bossBarX, bossBarY, bossBarW, bossBarH }, 2, WHITE);

                    const char* bossLabel = e.getIsEnraged() ? "FALSE KNIGHT - ENRAGED" : "FALSE KNIGHT";
                    float labelW = MeasureTextEx(myFont, bossLabel, 16, 1.0f).x;
                    DrawTextEx(myFont, bossLabel,
                        { screenWidth / 2.0f - labelW / 2.0f, bossBarY + 5 }, 16, 1.0f,
                        e.getIsEnraged() ? Color{255, 200, 100, 255} : WHITE);
                    break;
                }
            }

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

            // Kill counter & ghost ability UI
            float killUiY = uiY + 55;
            int displayKills = (killCount > GHOST_UNLOCK_KILLS) ? GHOST_UNLOCK_KILLS : killCount;
            float killRatio = (float)displayKills / GHOST_UNLOCK_KILLS;

            DrawRectangle(uiX, killUiY, 120, 10, { 20, 20, 30, 200 });
            if (killRatio > 0.0f) {
                Color killBarColor = ghostUnlocked ? Color{100, 200, 255, 255} : Color{200, 60, 60, 255};
                DrawRectangle(uiX, killUiY, 120 * killRatio, 10, killBarColor);
            }
            DrawRectangleLines(uiX, killUiY, 120, 10, GRAY);
            DrawTextEx(myFont, TextFormat("Kills: %d/%d", killCount, GHOST_UNLOCK_KILLS),
                { uiX, killUiY - 2 }, 14, 1.0f, ghostUnlocked ? Color{100, 200, 255, 255} : WHITE);

            // Ghost ability status
            float ghostUiX = uiX + 130;
            if (ghostUnlocked) {
                const char* ghostStatus;
                Color ghostColor;
                if (isGhosting) {
                    ghostStatus = "Q - PHASING!";
                    ghostColor = Color{100, 200, 255, 255};
                } else if (ghostCooldownTimer > 0.0f) {
                    ghostStatus = TextFormat("Q - %.1fs", ghostCooldownTimer);
                    ghostColor = GRAY;
                } else {
                    ghostStatus = "Q - READY";
                    ghostColor = Color{100, 200, 255, 255};
                }
                DrawTextEx(myFont, ghostStatus, { ghostUiX, killUiY - 3 }, 14, 1.0f, ghostColor);
            } else if (killCount > 0) {
                DrawTextEx(myFont, "Q - LOCKED", { ghostUiX, killUiY - 3 }, 14, 1.0f, DARKGRAY);
            }

            DrawTextEx(myFont, combatLog.c_str(), { uiX, uiY + 75 }, 20, 1.0f, WHITE);
            if (saveMessageTimer > 0.0f) {
                DrawTextEx(myFont, saveMessage.c_str(), { uiX, uiY + 100 }, 22, 1.0f, YELLOW);
            }
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
        else if (currentState == VICTORY) {
            ClearBackground({ 10, 15, 30, 255 });

            float victoryPulse = (sin(GetTime() * 2.5f) + 1.0f) * 0.5f;

            Color goldColor = { 255, 215, 0, (unsigned char)(200 + 55 * victoryPulse) };
            DrawTextEx(myFont, "VICTORY", { (float)(screenWidth / 2 - 160), 100 }, 72, 1.0f, goldColor);

            DrawTextEx(myFont, "The False Knight is defeated!", { (float)(screenWidth / 2 - 240), 200 }, 28, 1.0f, LIGHTGRAY);
            DrawTextEx(myFont, "The kingdom is saved...", { (float)(screenWidth / 2 - 170), 240 }, 22, 1.0f, GRAY);

            int mins = (int)(playTime / 60.0f);
            int secs = (int)(playTime) % 60;
            DrawTextEx(myFont, TextFormat("Clear Time: %02d:%02d", mins, secs),
                { (float)(screenWidth / 2 - 120), 310 }, 24, 1.0f, WHITE);

            DrawTextEx(myFont, TextFormat("HP Remaining: %d / %d", player.getHp(), player.getMaxHp()),
                { (float)(screenWidth / 2 - 130), 350 }, 20, 1.0f, WHITE);

            float lineAlpha = 0.5f + 0.5f * victoryPulse;
            DrawTextEx(myFont, "Press ENTER or ESC to return to menu",
                { (float)(screenWidth / 2 - 260), 440 }, 22, 1.0f,
                { 200, 200, 200, (unsigned char)(150 + 105 * victoryPulse) });

            // Floating particles for celebration
            for (int i = 0; i < 40; i++) {
                float px = screenWidth / 2.0f + cos(GetTime() * 1.7f + i * 0.5f) * 300;
                float py = 300 + sin(GetTime() * 2.1f + i * 0.7f) * 180;
                float size = 2.0f + sin(GetTime() * 3.0f + i) * 1.5f;
                Color spark = { 255, 215, 0, (unsigned char)(100 + 80 * sin(GetTime() * 2.0f + i)) };
                DrawCircle(px, py, size, spark);
            }
        }

        if (currentState == PAUSED) {
            DrawRectangle(0, 0, screenWidth, screenHeight, { 0, 0, 0, 150 });
            DrawTextEx(myFont, "PAUSED", { (float)(screenWidth / 2 - 80), (float)(screenHeight / 2 - 40) }, 50, 1.0f, WHITE);
            DrawTextEx(myFont, "Press ESC to resume", { (float)(screenWidth / 2 - 130), (float)(screenHeight / 2 + 30) }, 18, 1.0f, GRAY);
            DrawTextEx(myFont, "F5 - Save Game", { (float)(screenWidth / 2 - 100), (float)(screenHeight / 2 + 60) }, 16, 1.0f, GRAY);
            DrawTextEx(myFont, "F9 - Load Game", { (float)(screenWidth / 2 - 100), (float)(screenHeight / 2 + 85) }, 16, 1.0f, GRAY);
            if (saveMessageTimer > 0.0f) {
                DrawTextEx(myFont, saveMessage.c_str(), { (float)(screenWidth / 2 - 80), (float)(screenHeight / 2 + 115) }, 20, 1.0f, YELLOW);
            }
        }

        if (isTransitioning) {
            unsigned char alpha = (unsigned char)(fadeAlpha * 255.0f);
            DrawRectangle(0, 0, screenWidth, screenHeight, { 0, 0, 0, alpha });
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

    // 🌟 卸载 Boss 5 张精准资产
    UnloadTexture(texBossStand);
    UnloadTexture(texBossJump);
    UnloadTexture(texBossAir);
    UnloadTexture(texBossSwing);
    UnloadTexture(texBossHit);

    for (int i = 0; i < 4; i++) {
        if (crawlerTex[i].id != 0) UnloadTexture(crawlerTex[i]);
    }
    UnloadTexture(flyerTex);
    UnloadTexture(wallTex);
    UnloadTexture(platformTex);
    UnloadTexture(spikeTex);
    UnloadTexture(bgFar);

    for (int i = 0; i < 4; i++) {
        if (slashTex[i].id != 0) UnloadTexture(slashTex[i]);
        if (hitImpactTex[i].id != 0) UnloadTexture(hitImpactTex[i]);
    }

    CloseAudioDevice();
    CloseWindow();
    return 0;
}