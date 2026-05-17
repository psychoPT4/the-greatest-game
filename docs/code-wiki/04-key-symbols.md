# 关键类与函数说明

## 1. 基础结构体

## 1.1 `Hitbox`

职责：

- 表示轴对齐碰撞盒
- 提供 `intersects()` 用于碰撞检测

使用位置：

- 玩家受击盒
- 敌人受击盒
- 攻击判定
- 法术投射物
- Boss 冲击波

价值：

- 是整个战斗与碰撞系统的最低层数据结构

## 1.2 `Projectile`

职责：

- 表示玩家释放的灵魂弹

核心字段：

- 位置 `x / y`
- 速度 `speed`
- 朝向 `facingDir`
- 生命周期 `lifeTimer`
- 启用状态 `active`

关键接口：

- `getHitbox()`

## 1.3 `Shockwave`

职责：

- 表示 Boss 砸地产生的地面冲击波

核心字段：

- 位置
- 水平速度
- 方向
- 生命周期
- 冲击波高度

关键接口：

- `getHitbox()`

## 1.4 `AttackResult`

职责：

- 承载一次近战攻击的结果

字段：

- `pogoSuccess`
- `hitSomething`

用途：

- 用于主循环决定是否触发命中特效、震屏、音效与下劈弹跳

## 1.5 `SpawnPoint`

职责：

- 表示地图中的出生点或目标点

字段：

- `x`
- `y`
- `type`

用途：

- 玩家出生
- 敌人出生
- 关卡终点记录

## 1.6 `ParallaxLayer`

职责：

- 描述一层视差背景

字段：

- `tex`
- `scrollSpeed`

## 2. 核心类

## 2.1 `Player`

### 定位

玩家角色的核心领域对象。

### 关键职责

- 维护角色生命、法力、体力与动作状态
- 实现移动物理与碰撞
- 实现近战、法术、冲刺与受伤

### 代表接口

- `setRealPos(float nx, float ny)`
  - 直接设置玩家实际位置，常用于出生、传送和危险地形回退
- `takeDamage(int damage, int sourceX, const Map& gameMap)`
  - 执行受伤、受击闪烁、击退与死亡判定
- `setMoveIntent(int dir)`
  - 设置当前水平移动意图
- `processJump(bool jumpPressed, bool jumpHeld, bool downHeld)`
  - 处理起跳、二段跳、平台下落和大小跳
- `update(const Map& gameMap, float dt)`
  - 更新角色运动学，是玩家物理逻辑核心
- `attack(std::vector<Enemy>& enemies, const Map& gameMap, int attackType, int lockedDir, int& killCount)`
  - 执行近战判定并返回攻击结果
- `tryDash()`
  - 尝试进入冲刺状态
- `processMagic(bool magicHeld, bool magicReleased, float dt)`
  - 统一处理治疗和灵魂弹施法
- `updateProjectiles(std::vector<Enemy>& enemies, const Map& gameMap, float dt, int& killCount)`
  - 更新投射物飞行、碰撞和销毁
- `restoreState(...)`
  - 从存档恢复玩家运行状态

### 关键实现说明

#### `Player::update`

这是玩家模块中最重要的函数，负责：

- 冲刺中的速度覆盖
- 地面/空中水平速度变化
- 重力叠加与最大下落速度限制
- 墙体碰撞
- 平台着陆
- 头顶碰撞
- 体力恢复或持续消耗

它基本决定了角色手感。

#### `Player::attack`

该函数统一实现三种近战模式：

- 平砍
- 上挑
- 空中下劈

它会：

- 根据攻击类型生成不同碰撞盒
- 遍历敌人进行判定
- 命中时补充法力
- 击杀时累加击杀数
- 在下劈命中或击中地刺时触发反弹

#### `Player::processMagic`

该函数把“长按治疗”和“短按发射法术”合并到一个逻辑入口里：

- 长按到阈值且满足站立条件时回血
- 短按释放时发射投射物

好处是输入语义统一，坏处是两个能力被强绑定在同一个按键与状态机中。

## 2.2 `Enemy`

### 定位

敌人与 Boss 的统一抽象类。

### 关键职责

- 保存敌人基础信息
- 根据类型执行 AI
- 处理受伤和死亡
- 处理与玩家的接触攻击

### 代表接口

- `update(const Map& gameMap, Player& player, float dt)`
  - 每帧更新敌人行为
- `takeDamage(int damage, int sourceX, const Map& gameMap)`
  - 执行受击、死亡与部分击退逻辑
- `getHitbox()`
  - 返回敌人碰撞盒
- `getShockwaves()`
  - 返回 Boss 冲击波列表，供渲染层使用
- `restoreState(...)`
  - 从存档恢复敌人状态

### 关键实现说明

#### `Enemy::update`

这是敌人模块的核心函数，内部按 `enemyType` 分支：

- `Flyer`
  - 巡逻、发现玩家、追击、回位
- `Crawler`
  - 地面巡逻、警戒、突进、防跌落转向
- `False Knight`
  - 逼近、起跳、空中位移、举锤、砸地、冲击波、狂暴

它本质上是整个敌人系统的总状态机。

#### `Enemy::takeDamage`

该函数负责：

- 闪烁期间免疫重复受伤
- 血量扣减
- 死亡判定
- 普通敌人受击击退
- Boss 受击反馈但不被击退

它也是玩家攻击与投射物命中的最终落点。

## 2.3 `Map`

### 定位

地图数据与关卡主题资源的统一容器。

### 代表接口

- `loadLevel(int levelIndex)`
  - 加载一关的 CSV 与主题资源
- `unloadTheme()`
  - 释放当前背景与地表纹理
- `getTileAt(int x, int y) const`
  - 读取指定格子的 Tile 类型
- `getEnemySpawns() const`
  - 返回敌人出生点列表
- `getPlayerSpawn() const`
  - 返回玩家出生点
- `getGoalPoint() const`
  - 返回关卡终点

### 关键实现说明

#### `Map::loadLevel`

该函数完成四件事：

1. 清空旧地图与旧主题
2. 读取 `mapN.csv`
3. 把整数值转换为 `TileType`
4. 根据关卡编号加载不同背景层与地表贴图

这是项目中最关键的数据入口函数。

#### `Map::getTileAt`

这个函数除了普通查询，还内置了边界语义：

- `y < 0` 返回 `Empty`
- `y >= height` 返回 `Void`
- `x < 0` 或 `x >= width` 返回 `Wall`

这种设计让调用者不必在每次探测前手动写边界判断。

## 2.4 `GameCamera`

### 定位

简化版 2D 镜头对象。

### 代表接口

- `update(float targetX, float targetY, int mapW, int mapH)`
  - 更新镜头位置并限制边界
- `getCamera() const`
  - 生成 `Camera2D`

### 说明

当前实现只提供基础跟随与边界裁剪，不处理镜头缓动、锁区或事件镜头。

## 3. 关键自由函数

## 3.1 `saveGame(...)`

职责：

- 把当前运行状态写入 `save.dat`

内容包括：

- 当前关卡
- 玩家状态
- 安全落脚点
- 音量设置
- 击杀数与虚化解锁状态
- 游戏时长
- 全部敌人状态
- 战斗日志文本

特点：

- 使用 `FILE*` 和 `fwrite`
- 带文件头魔数 `FKGT`
- 带版本号 `SAVE_VERSION = 2`

## 3.2 `loadGame(...)`

职责：

- 从 `save.dat` 还原完整游戏状态

执行流程：

1. 校验文件头和版本
2. 加载目标关卡
3. 恢复玩家状态
4. 恢复音量和进度数据
5. 恢复敌人状态
6. 恢复日志文本

它是游戏可继续性的重要保障。

## 3.3 `DrawTexExact(...)`

职责：

- 用固定宽高绘制贴图
- 支持水平翻转

用途：

- 角色、Boss、瓦片等多处精确绘制

## 3.4 `DrawSpriteFrame(...)`

职责：

- 从序列帧贴图中截取指定帧并绘制

用途：

- 飞虫、角色帧、残影绘制

## 4. 关键枚举

## 4.1 `GameState`

定义整个应用的状态机，是主循环控制流的核心枚举。

## 4.2 `MagicAction`

表示法术处理结果：

- `MAGIC_NONE`
- `CAST_SPELL`
- `HEALED`

用于主循环识别本帧是否需要播放对应音效和特效。

## 4.3 `TileType`

定义地图中每个格子的语义，是关卡系统的基础枚举。

## 5. 最值得优先理解的函数

如果只打算先读最关键的 8 个符号，建议优先看：

1. `main()`
2. `saveGame()`
3. `loadGame()`
4. `Player::update()`
5. `Player::attack()`
6. `Player::processMagic()`
7. `Enemy::update()`
8. `Map::loadLevel()`
