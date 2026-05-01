#ifndef ROLE_H
#define ROLE_H

#include <string>
#include "map.h"

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

    virtual void update(const Map& gameMap) = 0;
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
    float velocityY, gravity, jumpForce;
    bool isGrounded;

public:
    Player(int startX, int startY);
    void update(const Map& gameMap) override;
    void move(int dx, int dy, const Map& gameMap); // 独立 X 轴处理
    void jump(); // 瞬间改变初速度
    void attack(Role& target, const Map& gameMap);
};

class Enemy : public Role {
private:
    int moveDirection, expReward, attackCooldown;

public:
    Enemy(int startX, int startY, int expGive);
    void update(const Map& gameMap, Player& player); 
    void update(const Map& gameMap) override {} 
};

#endif