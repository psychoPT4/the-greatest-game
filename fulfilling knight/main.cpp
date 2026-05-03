#include <iostream>
#include <string>
#include <vector>
#include <math.h>
#include "map.h"
#include "Role.h"
#include "Camera.h"
#include "raylib.h" 

using namespace std;

enum GameState { MENU, PLAYING, GAME_OVER };
GameState currentState = MENU;

struct SporeParticle {
    Vector2 pos;
    Vector2 velocity;
    float radius;
    unsigned char alpha;
};

string combatLog = "Adventure continues...";
float safeX, safeY;
const int TILE_SIZE = 48;

void DrawTexExact(Texture2D tex, float x, float y, float w, float h, Color tint = WHITE, bool flipX = false) {
    Rectangle src = { 0, 0, flipX ? -(float)tex.width : (float)tex.width, (float)tex.height };
    Rectangle dest = { x, y, w, h };
    DrawTexturePro(tex, src, dest, { 0, 0 }, 0.0f, tint);
}

void DrawSpriteFrame(Texture2D tex, int currentFrame, int totalFrames, float x, float y, float w, float h, Color tint = WHITE, bool flipX = false) {
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
    SetTargetFPS(60);

    Texture2D bgTex = LoadTexture("bg.png");
    Texture2D texStand = LoadTexture("knight_stand.png");
    Texture2D texWalk = LoadTexture("knight_walk.png");
    Texture2D texRun = LoadTexture("knight_run.png");
    Texture2D crawlerTex = LoadTexture("crawler.png");
    Texture2D wallTex = LoadTexture("wall.png");
    Texture2D platformTex = LoadTexture("platform.png");
    Texture2D spikeTex = LoadTexture("spike.png");

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

    vector<SporeParticle> particles;
    for (int i = 0; i < 150; i++) {
        particles.push_back({
            { (float)GetRandomValue(0, screenWidth), (float)GetRandomValue(0, screenHeight) },
            { (float)GetRandomValue(-15, 15) / 10.0f, (float)GetRandomValue(-30, -5) / 10.0f },
            (float)GetRandomValue(1, 4),
            (unsigned char)GetRandomValue(50, 200)
            });
    }

    while (!WindowShouldClose()) {

        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;

        if (currentState == MENU) {
            Vector2 mousePos = GetMousePosition();
            for (auto& p : particles) {
                p.pos.x += p.velocity.x;
                p.pos.y += p.velocity.y;
                float dx = p.pos.x - mousePos.x;
                float dy = p.pos.y - mousePos.y;
                float dist = sqrt(dx * dx + dy * dy);
                if (dist < 120.0f && dist > 0.1f) {
                    float force = (120.0f - dist) / 120.0f;
                    p.pos.x += (dx / dist) * force * 5.0f;
                    p.pos.y += (dy / dist) * force * 5.0f;
                }
                if (p.pos.y < -10) p.pos.y = screenHeight + 10;
                if (p.pos.x < -10) p.pos.x = screenWidth + 10;
                if (p.pos.x > screenWidth + 10) p.pos.x = -10;
            }

            if (CheckCollisionPointRec(mousePos, btnStart) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                currentState = PLAYING;
                mapLoaded = false;
            }
            if (CheckCollisionPointRec(mousePos, btnExit) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                break;
            }
        }
        else if (currentState == PLAYING) {
            if (!mapLoaded) {
                if (!gameMap.loadFromCSV("map" + to_string(currentLevel) + ".csv")) {
                    break;
                }
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

            // 【新增】：按 L 键触发冲刺！
            if (IsKeyPressed(KEY_L)) {
                player.startDash();
            }

            int moveDir = (IsKeyDown(KEY_D) - IsKeyDown(KEY_A));
            player.setMoveIntent(moveDir);

            bool jump = IsKeyDown(KEY_K);
            static bool lastJump = false;
            player.processJump(jump && !lastJump, jump);
            lastJump = jump;

            if (attackCd > 0) attackCd--;
            if (IsKeyDown(KEY_J) && attackCd <= 0) {
                AttackResult result = player.attack(enemies, gameMap, IsKeyDown(KEY_S));
                if (result.totalXp > 0) player.addExp(result.totalXp);
                attackCd = 12;
            }

            player.update(gameMap, dt);

            if (gameMap.getTileAt(player.getX(), player.getY() + 1) == TileType::Wall) {
                safeX = player.getRealX(); safeY = player.getRealY();
            }

            for (auto& e : enemies) e.update(gameMap, player, dt);

            Hitbox pBox = player.getHitbox();
            TileType footL = gameMap.getTileAt((int)pBox.left, (int)pBox.bottom);
            TileType footR = gameMap.getTileAt((int)pBox.right, (int)pBox.bottom);

            if (footL == TileType::Void || footR == TileType::Void || player.getY() >= 29) {
                player.takeDamage(player.getMaxHp() / 4, player.getX(), gameMap);
                player.setRealPos(safeX, safeY);
                combatLog = "Fell into the void!";
            }
            else if (footL == TileType::SpikeUp || footR == TileType::SpikeUp) {
                player.takeDamage(20, player.getX(), gameMap);
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
        BeginDrawing();
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

            DrawText("Fulfilling Knight", screenWidth / 2 - 220, screenHeight / 2 - 150, 60, LIGHTGRAY);

            for (const auto& p : particles) {
                Color sporeColor = { 135, 206, 235, p.alpha };
                DrawCircleV(p.pos, p.radius, sporeColor);
            }

            bool hoverStart = CheckCollisionPointRec(mousePos, btnStart);
            DrawRectangleRec(btnStart, hoverStart ? DARKGRAY : BLANK);
            DrawRectangleLinesEx(btnStart, 2, hoverStart ? WHITE : GRAY);
            DrawText("START GAME", btnStart.x + 40, btnStart.y + 15, 20, hoverStart ? WHITE : GRAY);

            bool hoverSettings = CheckCollisionPointRec(mousePos, btnSettings);
            DrawRectangleRec(btnSettings, hoverSettings ? DARKGRAY : BLANK);
            DrawRectangleLinesEx(btnSettings, 2, hoverSettings ? WHITE : GRAY);
            DrawText("SETTINGS", btnSettings.x + 55, btnSettings.y + 15, 20, hoverSettings ? WHITE : GRAY);

            bool hoverExit = CheckCollisionPointRec(mousePos, btnExit);
            DrawRectangleRec(btnExit, hoverExit ? DARKGRAY : BLANK);
            DrawRectangleLinesEx(btnExit, 2, hoverExit ? WHITE : GRAY);
            DrawText("EXIT", btnExit.x + 80, btnExit.y + 15, 20, hoverExit ? WHITE : GRAY);
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
                    float renderX = (worldX - cam.x) * TILE_SIZE;
                    float renderY = (worldY - cam.y) * TILE_SIZE;

                    if (t == TileType::Wall) {
                        if (wallTex.id != 0) DrawTexExact(wallTex, renderX, renderY, TILE_SIZE, TILE_SIZE);
                        else DrawRectangle(renderX, renderY, TILE_SIZE, TILE_SIZE, DARKGRAY);
                    }
                    else if (t == TileType::Platform) {
                        if (platformTex.id != 0) DrawTexExact(platformTex, renderX, renderY, TILE_SIZE, TILE_SIZE / 4.0f);
                        else DrawRectangle(renderX, renderY, TILE_SIZE, TILE_SIZE / 4, GRAY);
                    }
                    else if (t == TileType::SpikeUp) {
                        if (spikeTex.id != 0) DrawTexExact(spikeTex, renderX, renderY, TILE_SIZE, TILE_SIZE);
                        else DrawTriangle({ renderX + TILE_SIZE / 2, renderY }, { renderX, renderY + TILE_SIZE }, { renderX + TILE_SIZE, renderY + TILE_SIZE }, RED);
                    }
                    else if (t == TileType::Void) {
                        DrawRectangle(renderX, renderY, TILE_SIZE, TILE_SIZE, { 20, 10, 30, 200 });
                    }

                    SpawnPoint goal = gameMap.getGoalPoint();
                    if (worldX == goal.x && worldY == goal.y) DrawCircle(renderX + TILE_SIZE / 2, renderY + TILE_SIZE / 2, TILE_SIZE / 2, YELLOW);
                }
            }

            // 绘制怪物
            for (const auto& e : enemies) {
                if (e.isAlive()) {
                    float renderX = (e.getRealX() - cam.x - 0.10f) * TILE_SIZE;
                    float renderY = (e.getRealY() - cam.y + 0.40f) * TILE_SIZE;

                    if (crawlerTex.id != 0) {
                        DrawTexExact(crawlerTex, renderX, renderY, 1.0f * TILE_SIZE, 0.6f * TILE_SIZE, e.isFlickering() ? RED : WHITE);
                    }
                    else {
                        DrawRectangle((e.getRealX() - cam.x) * TILE_SIZE, (e.getRealY() - cam.y) * TILE_SIZE, 0.8f * TILE_SIZE, 0.5f * TILE_SIZE, e.isFlickering() ? ORANGE : RED);
                    }
                }
            }

            // 绘制玩家骑士（动画状态机）
            float pRenderX = (player.getRealX() - cam.x - 0.25f) * TILE_SIZE;
            float pRenderY = (player.getRealY() - cam.y) * TILE_SIZE;

            static bool facingLeft = false;
            // 判断朝向（冲刺期间朝向被锁定）
            if (!player.getIsDashing()) {
                if (IsKeyDown(KEY_A)) facingLeft = true;
                else if (IsKeyDown(KEY_D)) facingLeft = false;
            }

            static float frameTimer = 0.0f;
            static int currentFrame = 0;
            const float FRAME_UPDATE_TIME = 0.12f;
            frameTimer += GetFrameTime();

            Texture2D activeTex = texStand;
            int totalFrames = 2;

            // 优先判定冲刺和跑动状态
            if (player.getIsDashing()) {
                // 如果你有专门的 dash 贴图可以加上，目前借用 run 的贴图
                activeTex = texRun;
                totalFrames = 5;
            }
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
            else {
                activeTex = texStand;
                totalFrames = 2;
            }

            static unsigned int lastTexId = activeTex.id;
            if (activeTex.id != lastTexId) {
                currentFrame = 0;
                frameTimer = 0.0f;
                lastTexId = activeTex.id;
            }

            if (frameTimer >= FRAME_UPDATE_TIME) {
                currentFrame = (currentFrame + 1) % totalFrames;
                frameTimer = 0.0f;
            }

            if (activeTex.id != 0) {
                DrawSpriteFrame(activeTex, currentFrame, totalFrames, pRenderX, pRenderY, TILE_SIZE, TILE_SIZE, player.isFlickering() ? RED : WHITE, facingLeft);
            }
            else {
                DrawRectangle((player.getRealX() - cam.x) * TILE_SIZE, (player.getRealY() - cam.y) * TILE_SIZE, 0.5f * TILE_SIZE, 0.8f * TILE_SIZE, player.isFlickering() ? SKYBLUE : BLUE);
            }

            DrawText(TextFormat("HP: %d/%d  LVL: %d  XP: %d/%d", player.getHp(), player.getMaxHp(), player.getLevel(), player.getExp(), player.getExpToNext()), 10, 10, 20, GREEN);
            DrawText(combatLog.c_str(), 10, 40, 20, WHITE);
        }
        else if (currentState == GAME_OVER) {
            DrawText("GAME OVER", 280, 150, 40, RED);
            DrawText("The knight has fallen...", 280, 220, 20, LIGHTGRAY);
        }

        EndDrawing();
    }

    UnloadTexture(bgTex);
    UnloadTexture(texStand);
    UnloadTexture(texWalk);
    UnloadTexture(texRun);
    UnloadTexture(crawlerTex);
    UnloadTexture(wallTex);
    UnloadTexture(platformTex);
    UnloadTexture(spikeTex);

    CloseWindow();
    return 0;
}