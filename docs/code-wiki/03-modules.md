# 主要模块职责

## 1. `main.cpp`：主控与编排模块

### 1.1 模块职责

`main.cpp` 是全项目最核心的文件，负责：

- 初始化窗口、音频、字体、贴图与音效
- 定义全局游戏状态机
- 驱动主循环
- 处理输入与菜单交互
- 协调玩家、敌人、地图、相机
- 控制关卡切换、过场、胜负判定
- 执行存档与读档
- 渲染场景与 UI

### 1.2 模块特征

它不是“纯入口文件”，而是实际上的“游戏导演层”。如果用分层视角理解，它同时承担了：

- 应用入口层
- 状态机层
- 逻辑调度层
- 表现层
- 部分数据持久化层

### 1.3 代表性内容

- `GameState`
- `saveGame()`
- `loadGame()`
- 菜单与设置逻辑
- 关卡初始化逻辑
- 每帧游戏更新逻辑
- 统一渲染逻辑

## 2. `Player` 模块：角色与战斗核心

文件：

- `player.h`
- `player.cpp`

### 2.1 模块职责

`Player` 负责玩家角色的全部核心行为：

- 角色状态管理
- 水平移动与重力
- 起跳、二段跳、平台下落
- 冲刺与无敌判定
- 普通攻击、上挑、下劈
- 治疗与灵魂弹法术
- 投射物更新
- 受伤、击退、死亡

### 2.2 内部状态类别

`Player` 内部维护的信息较多，大体可分为：

- 位置与碰撞
  - `realX`、`realY`、`hbWidth`、`hbHeight`
- 生存与资源
  - `hp`、`mana`、`stamina`
- 运动学参数
  - `velocityX`、`velocityY`、`gravity`、`jumpForce`
- 动作状态
  - `isGrounded`、`isDashing`、`isRunningMode`
- 法术状态
  - `focusTimer`、`isFocusing`
- 投射物
  - `projectiles`

### 2.3 设计判断

`Player` 是仓库里最像“完整领域对象”的类：

- 数据集中
- 行为也集中
- 对外接口相对清晰

它的边界比 `main.cpp` 清晰得多，是目前代码中最适合继续扩展的模块之一。

## 3. `Enemy` 模块：敌人与 Boss 行为模块

文件：

- `enemy.h`
- `enemy.cpp`

### 3.1 模块职责

`Enemy` 统一封装三种敌人：

- `Crawler`
- `Flyer`
- `False Knight`

它负责：

- 敌人基础属性与生命管理
- 受伤、死亡、受击闪烁
- 敌人移动与 AI 状态机
- Boss 特殊行为与冲击波系统
- 与玩家的接触伤害

### 3.2 设计特点

这个模块把普通敌人与 Boss 放在同一个类中，通过 `enemyType` 和若干状态字段分支：

- `enemyType`
- `aiState`
- `bossState`
- `stateTimer`
- `isEnraged`

优点是结构集中、实现简单。

缺点是：

- 分支越来越多时会变得难以维护
- 普通敌人与 Boss 的状态字段共享，语义不完全统一

### 3.3 Boss 逻辑特点

`False Knight` 的实现已经形成了一个独立的小型状态机：

- 待机逼近
- 蓄力起跳
- 空中换位或后撤跳
- 举锤前摇
- 砸地后摇
- 冲击波生成与更新

因此 `Enemy` 模块的复杂度主要由 Boss 行为拉高。

## 4. `Map` 模块：世界数据与主题资源模块

文件：

- `map.h`
- `map.cpp`

### 4.1 模块职责

`Map` 负责两类事情：

- 世界数据
  - 地图宽高
  - Tile 二维网格
  - 玩家出生点
  - 敌人出生点
  - 关卡终点
- 主题资源
  - 地表贴图
  - 视差背景层

### 4.2 设计特点

`Map` 并不是纯数据容器，它同时兼具“关卡资源绑定器”的角色。

`loadLevel(levelIndex)` 在一次调用中同时完成：

- 读取 CSV
- 解析 Tile
- 识别出生点与终点
- 根据关卡编号加载主题贴图

这让使用很方便，但也使它同时承担了数据层和资源层职责。

### 4.3 Tile 语义

`TileType` 不仅表达地形，还编码游戏语义：

- `Wall`
- `Platform`
- `Void`
- `SpikeUp`
- `SpawnCrawler`
- `SpawnFlyer`
- `SpawnPlayer`
- `LevelGoal`
- `Spawnboss`

这说明地图文件不是“纯视觉地图”，而是“关卡逻辑地图”。

## 5. `GameCamera` 模块：视口模块

文件：

- `camera.h`

### 5.1 模块职责

`GameCamera` 的职责很单一：

- 以目标位置为中心更新镜头
- 限制镜头不要超出地图边界
- 返回 `Camera2D` 给 Raylib 渲染层使用

### 5.2 模块评价

这是项目中最轻量的模块：

- 几乎没有业务耦合
- 算法简单
- 易于替换或扩展

如果未来要做镜头缓动、区域锁镜、Boss 房镜头控制，这个模块会成为自然扩展点。

## 6. 地图数据模块：`map*.csv`

### 6.1 职责

地图 CSV 提供：

- 静态地形布局
- 玩家出生点
- 敌人出生点
- 通关终点
- Boss 房布局

### 6.2 当前关卡分工

- `map1.csv`
  - 初始关卡，主题为平原
- `map2.csv`
  - 第二关，主题为洞穴
- `map3.csv`
  - 第三关，Boss 房，包含玩家出生点、Boss 出生点和终点

## 7. 模块依赖关系

```text
main.cpp
  ├─ 依赖 Map
  ├─ 依赖 Player
  ├─ 依赖 Enemy
  ├─ 依赖 GameCamera
  └─ 直接依赖 Raylib

Player
  ├─ 依赖 Map
  └─ 依赖 Enemy（攻击与投射物命中）

Enemy
  ├─ 依赖 Map
  └─ 依赖 Player

Map
  └─ 依赖 Raylib（贴图加载与释放）

GameCamera
  └─ 依赖 Raylib（Camera2D）
```

## 8. 跨模块共享状态

当前有一个明显的跨模块共享点：

- `combatLog`

它在 `main.cpp` 中定义，在 `player.cpp` 和 `enemy.cpp` 中通过 `extern` 写入，用于展示战斗日志。

这是一种简单有效但耦合偏强的方式，说明当前项目更偏向快速迭代而非严格分层。

## 9. 模块复杂度排序

按维护复杂度大致排序如下：

1. `main.cpp`
2. `Enemy`
3. `Player`
4. `Map`
5. `GameCamera`

如果后续需要继续扩展项目，优先应关注前 3 个模块的可维护性。
