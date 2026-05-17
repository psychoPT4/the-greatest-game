# 整体架构

## 1. 架构总图

项目的运行时结构可以概括为：

```text
用户输入
  ↓
main.cpp
  ├─ 游戏状态机（菜单 / 教程 / 游戏中 / 暂停 / 设置 / 失败 / 胜利）
  ├─ 资源加载（贴图 / 字体 / 音频）
  ├─ 关卡装载与推进
  ├─ 玩家更新调度
  ├─ 敌人更新调度
  ├─ 相机更新
  ├─ 渲染与 UI
  └─ 存档 / 读档
      ↓
  Player / Enemy / Map / GameCamera
```

这是一个强主控型架构：

- `main.cpp` 是调度中心
- 业务对象不直接互相组织生命周期
- 大部分跨模块流程都从 `main.cpp` 发起

## 2. 运行时状态机

项目使用 `GameState` 管理游戏流程，状态包括：

- `MENU`
- `PLAYING`
- `PAUSED`
- `SETTINGS`
- `GAME_OVER`
- `VICTORY`
- `HOW_TO_PLAY`

状态流转的大体路径如下：

```text
MENU
  ├─ 新游戏 -> HOW_TO_PLAY -> PLAYING
  ├─ 读档 -> PLAYING
  ├─ 设置 -> SETTINGS -> MENU
  └─ 退出 -> 程序结束

PLAYING
  ├─ ESC -> PAUSED -> PLAYING
  ├─ 玩家死亡 -> GAME_OVER
  ├─ 关卡完成 -> 下一关 或 VICTORY
  └─ F5 / F9 -> 保存 / 读档

GAME_OVER
  ├─ ENTER -> 重开
  └─ ESC -> MENU

VICTORY
  └─ ENTER / ESC -> MENU
```

## 3. 主循环的职责拆分

主循环位于 `main.cpp` 的 `while (!WindowShouldClose())` 中，其核心执行顺序可以概括为：

### 3.1 帧级公共更新

- 更新当前 BGM 流
- 读取 `dt`
- 限制超大帧时间
- 更新提示计时器、过场淡入淡出、游戏时长
- 更新虚化能力状态与冷却

### 3.2 按状态执行逻辑

- `MENU`
  - 更新主菜单交互与读档按钮
- `HOW_TO_PLAY`
  - 等待任意关键输入进入游戏
- `PLAYING`
  - 处理输入、攻击、法术、冲刺、玩家更新、敌人更新、危险地形、镜头、通关判断
- `PAUSED`
  - 暂停状态下允许继续、保存和读档
- `SETTINGS`
  - 调整 BGM/SFX 音量
- `GAME_OVER` / `VICTORY`
  - 展示结果页并等待返回菜单

### 3.3 统一渲染

所有状态最终都会进入统一渲染段：

- 先绘制到固定分辨率 `RenderTexture`
- 再按窗口比例缩放到实际窗口

这种方式带来的好处是：

- UI 和世界渲染基准一致
- 可在可变窗口尺寸下保持画面比例稳定
- 不需要让逻辑层直接感知真实窗口大小

## 4. 模块协作关系

## 4.1 `main.cpp -> Map`

`main.cpp` 通过 `Map` 完成：

- 按关卡编号加载 `map{level}.csv`
- 获取玩家出生点
- 获取敌人出生点
- 获取关卡终点
- 查询地图宽高供相机约束使用
- 获取背景层与地表纹理供渲染使用

## 4.2 `main.cpp -> Player`

`main.cpp` 负责从输入构造玩家行为：

- 设置左右移动意图
- 设置跑步模式
- 触发跳跃、冲刺、攻击、法术
- 调用 `Player::update`
- 调用 `Player::updateProjectiles`

也就是说，`Player` 负责“怎么执行动作”，但不是“何时执行动作”。

## 4.3 `main.cpp -> Enemy`

`main.cpp` 统一迭代 `vector<Enemy>`：

- 首次进入关卡时根据出生点实例化
- 每帧调用 `Enemy::update`
- 在渲染阶段按类型选择不同表现
- 统计 Boss 是否存活，用于决定终点是否生效

## 4.4 `main.cpp -> GameCamera`

相机逻辑很轻：

- 以玩家当前位置为跟随目标
- 结合地图宽高裁剪边界
- 输出给渲染层使用的 `Camera2D` 信息

## 5. 数据流

## 5.1 输入数据流

```text
键盘 / 鼠标
  ↓
main.cpp 读取输入
  ↓
Player / 状态机 / 菜单逻辑
  ↓
游戏世界状态变化
```

## 5.2 战斗数据流

```text
输入 J / I / L / Q
  ↓
main.cpp 识别动作
  ↓
Player 生成攻击判定 / 法术投射物 / 冲刺 / 虚化
  ↓
Enemy 受伤 / 死亡 / 反击
  ↓
combatLog / 粒子 / 震屏 / UI 更新
```

## 5.3 关卡数据流

```text
mapN.csv
  ↓
Map::loadLevel()
  ↓
grid / playerSpawn / enemySpawns / goalPoint
  ↓
main.cpp 根据出生点创建 Player / Enemy
```

## 5.4 存档数据流

```text
当前运行时对象
  ↓
saveGame()
  ↓
save.dat
  ↓
loadGame()
  ↓
恢复 Player / Enemy / 关卡 / 音量 / 进度
```

## 6. 关卡推进机制

项目没有独立的关卡管理器，关卡推进逻辑直接写在 `main.cpp`：

- 检查玩家是否站在 `goalPoint`
- 第 3 关时要求 Boss 已死亡
- 触发淡出过场
- 若还有下一关，则 `currentLevel + 1`
- 若当前为最后一关，则进入 `VICTORY`

这说明“关卡切换”本质上是主循环中的一个状态转换分支，而非独立子系统。

## 7. 渲染架构

渲染顺序基本为：

1. 背景层视差
2. 地图瓦片
3. 终点传送门
4. 玩家投射物
5. 敌人与 Boss
6. 玩家角色
7. 斩击特效、命中特效、粒子
8. UI 和文本
9. 暂停遮罩 / 转场淡入淡出

这是典型的 2D 游戏前后景分层绘制方式。

## 8. 当前架构的优点

- 结构简单，阅读成本低
- 单个主循环便于快速定位问题
- 模块数量少，逻辑链路直接
- 关卡与资源切换规则容易追踪

## 9. 当前架构的限制

- `main.cpp` 体量过大，承担了过多职责
- 游戏规则、表现逻辑和 UI 逻辑耦合较强
- 资源加载缺少统一资源管理层
- 日志通过全局变量 `combatLog` 跨模块共享，存在隐式耦合
- 没有独立的测试层或调试基础设施

## 10. 维护建议

如果后续继续扩展项目，最自然的重构方向是：

- 将 `main.cpp` 拆分为输入、更新、渲染、UI、存档几个子模块
- 将资源加载从主函数中抽离
- 为关卡切换、Boss 管理、UI 状态建立轻量级管理器
- 用事件或接口代替 `combatLog` 的全局共享写入
