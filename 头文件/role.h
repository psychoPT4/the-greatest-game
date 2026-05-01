#ifndef ROLE_H
#define ROLE_H

#include <string>
#include "map.h" // 确保这里能正确引用到你的地图类

// ==========================================
// 1. 基类：Role (游戏实体)
// ==========================================
class Role {
protected:
    int x;
    int y;
    int hp;
    int maxHp;         // 最大血量上限
    int baseDamage;    // 基础伤害 (主角的普攻 / 怪物的触碰伤害)
    std::string name;

public:
    Role(std::string n, int startX, int startY, int maxHp, int dmg);
    virtual ~Role() {}

    // 核心规定：所有实体在更新逻辑时，都必须能读取地图数据！
    virtual void update(const Map& gameMap) = 0; 
    
    virtual void takeDamage(int amount); 
    bool isDead() const { return hp <= 0; }

    // 获取属性的对外接口
    int getX() const { return x; }
    int getY() const { return y; }
    std::string getName() const { return name; }
    int getBaseDamage() const { return baseDamage; } 
};

// ==========================================
// 2. 派生类：Player (主角)
// ==========================================
class Player : public Role {
private:
    // 养成属性
    int level;
    int currentExp;
    int expToNextLevel;
    
    // 战斗属性
    int specialDamage; 
    int mana; 

    // 物理与重力属性
    float velocityY;    
    float gravity;      
    float jumpForce;    
    bool isGrounded;    

    // 内部升级辅助函数
    void levelUp();

public:
    Player(int startX, int startY);

    // 重写基类的更新函数
    void update(const Map& gameMap) override;
    
    // 玩家特有的移动与跳跃逻辑
    void move(int dx, int dy, const Map& gameMap);
    void jump();

    // 战斗与养成接口
    void gainExp(int amount);
    void normalAttack(Role& target);  
    void specialAttack(Role& target); 
};

// ==========================================
// 3. 派生类：Enemy (怪物)
// ==========================================
class Enemy : public Role {
private:
    int moveDirection;
    int expReward; // 击杀掉落经验

public:
    Enemy(int startX, int startY, int expGive);

    // 重写基类的更新函数
    void update(const Map& gameMap) override;
    
    int getExpReward() const { return expReward; }
};

#endif // ROLE_H