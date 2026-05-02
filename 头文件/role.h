#ifndef ROLE_H
#define ROLE_H

#include <string>
#include "map.h"
#include<vector>

class Player; 

class Role {
protected:
    int x, y;
    int hp, maxHp;
    int baseDamage;
    float realX, realY; 
    std::string name;
    bool alive;
    int flickerTimer; 

public:
    Role(std::string n, int startX, int startY, int maxHp, int dmg);
    virtual ~Role() {}

    // 更新函数增加 deltaTime 参数
    virtual void update(const Map& gameMap, float dt) = 0;
    virtual void takeDamage(int amount, int attackerX, const Map& gameMap); 
    
    bool isAlive() const { return alive; }
    bool isFlickering() const { return flickerTimer > 0; }
    int getX() const { return x; }
    int getY() const { return y; }
    int getHp() const { return hp; }
    int getMaxHp() const { return maxHp; }
    std::string getName() const { return name; }
};

class Player : public Role {
private:
    int level, currentExp, expToNextLevel;
    int mana, facingDirection, jumpCount;
    bool isGrounded;
    
    // 【新增动力学参数】
    float velocityX, velocityY;
    float maxRunSpeed, runAccel, groundFriction, airDrag;
    float gravity, jumpForce, maxFallSpeed;
    
    int moveIntent; // 当前玩家的输入意图 (-1, 0, 1)

public:
    Player(int startX, int startY);
    void update(const Map& gameMap, float dt) override;
    
    // 取代原来的 move 和 jump，改为状态输入
    void setMoveIntent(int dir); 
    void processJump(bool jumpPressed, bool jumpHeld); 
    bool attack(std::vector<class Enemy>& enemies, const Map& gameMap, bool downPressed);
};

class Enemy : public Role {
private:
    int moveDirection, expReward, attackCooldown;
    float moveTimer;
public:
    Enemy(int startX, int startY, int expGive);
    void update(const Map& gameMap, Player& player, float dt); 
    void update(const Map& gameMap, float dt) override {} 
};

#endif