# 项目总览

## 1. 项目定位

`Fulfilling Knight` 是一个基于 `C++20 + Raylib` 实现的 2D 平台动作游戏项目，玩法核心包括：

- 平台移动、跳跃与二段跳
- 普通攻击、上挑、下劈弹反
- 冲刺、治疗、远程法术
- 普通敌人与 Boss 战
- 关卡切换与二进制存档

从代码组织方式看，它不是大型引擎化项目，而是一个典型的“小型单体游戏程序”：

- 入口集中在 `main.cpp`
- 游戏对象较少，核心领域对象为 `Player`、`Enemy`、`Map`、`GameCamera`
- 关卡由 `map*.csv` 驱动
- 美术、音频、地图和代码基本平铺在仓库根目录

## 2. 技术栈

- 语言标准：`C++20`
- 图形与音频库：`Raylib`
- 工程体系：`Visual Studio .vcxproj`
- 运行平台倾向：`Windows + MSVC + raylib.dll`
- 关卡数据格式：`CSV`
- 存档格式：自定义二进制 `save.dat`

## 3. 仓库布局

当前仓库没有常见的 `src/`、`assets/`、`tests/` 分层，而是采用“源码文件 + 资源文件 + 地图文件”同级平铺的方式。

### 3.1 代码文件

- `main.cpp`
  - 程序入口、状态机、主循环、UI、音频、渲染、关卡推进、存档读档
- `player.h` / `player.cpp`
  - 玩家状态、移动物理、攻击、法术、投射物
- `enemy.h` / `enemy.cpp`
  - 敌人抽象、普通敌人 AI、Boss AI、冲击波逻辑
- `map.h` / `map.cpp`
  - Tile 定义、CSV 关卡加载、出生点和终点提取、主题资源绑定
- `camera.h`
  - 镜头跟随和视口边界裁剪

### 3.2 工程文件

- `fulfilling knight.vcxproj`
  - Visual Studio 工程入口
- `fulfilling knight.vcxproj.filters`
  - IDE 分组配置
- `.gitignore`
  - VS 构建产物与本地存档忽略规则

### 3.3 数据与资源

- `map1.csv`、`map2.csv`、`map3.csv`
  - 三个关卡的数据源
- `bg_plain/`、`bg_cave/`、`boss_cave/`
  - 三套关卡主题背景与地表贴图
- 根目录中的大量 `png/mp3/wav/TTF`
  - 角色帧、敌人帧、音效、BGM、字体

## 4. 核心设计判断

### 4.1 架构风格

项目采用的是“单入口主循环编排”架构：

- `main.cpp` 负责所有高层控制流
- 领域逻辑通过少量类分散实现
- 没有 ECS、脚本层、事件总线、资源管理器等复杂基础设施

### 4.2 数据驱动程度

项目具备中等程度的数据驱动特征：

- 地图布局由 CSV 提供
- 敌人出生点、玩家出生点、终点都编码在 Tile 中
- 关卡主题资源按关卡编号切换

但同时也保留了较强的硬编码特征：

- 资源路径大量直接写在 `main.cpp` 和 `map.cpp`
- BGM 切换规则写死在主循环
- 敌人类型与 Tile 值映射写死在代码中

### 4.3 模块边界

当前模块边界总体清晰，但 `main.cpp` 过重：

- `Player` 负责角色动作和战斗
- `Enemy` 负责敌人与 Boss 行为
- `Map` 负责世界格子与主题资源
- `GameCamera` 负责镜头数学
- `main.cpp` 负责把以上模块编排成完整游戏

这意味着后续维护时，最先需要关注和拆分的文件通常也是 `main.cpp`。

## 5. 代码阅读优先级

如果需要快速理解项目，推荐按下面顺序阅读源码：

1. `main.cpp`
2. `player.h` / `player.cpp`
3. `enemy.h` / `enemy.cpp`
4. `map.h` / `map.cpp`
5. `camera.h`
6. `map1.csv` ~ `map3.csv`

## 6. 当前仓库的工程化特征

已确认当前仓库：

- 有构建配置
- 有运行资源
- 有存档逻辑
- 没有自动化测试
- 没有 CI/CD
- 没有容器化或部署脚本

因此它更接近“本地 IDE 直接构建运行的游戏工程”，而不是标准化的多环境发布仓库。
