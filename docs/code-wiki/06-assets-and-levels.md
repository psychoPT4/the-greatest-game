# 资源组织与关卡数据

## 1. 资源组织方式

当前项目采用“代码与资源同级平铺”的方式：

- 源码文件在仓库根目录
- 地图 CSV 在仓库根目录
- 大多数贴图、音效、字体也在仓库根目录
- 只有关卡主题背景被拆到了独立目录

这种方式的优点是简单直接，缺点是：

- 资源数量增长后不易管理
- 路径错误难以统一排查
- 缺少资源元数据和命名规范约束

## 2. 主要资源分类

## 2.1 角色动作资源

玩家动作贴图按动作拆分为散图或序列帧：

- 待机：`stand0.png`、`stand1.png`
- 行走：`walk0.png` ~ `walk3.png`
- 跑步：`run0.png` ~ `run4.png`
- 普攻：`attack0.png` ~ `attack2.png`
- 上挑：`upattack0.png` ~ `upattack2.png`
- 下劈：`pogo0.png` ~ `pogo3.png`
- 跳跃：`jump0.png`、`jump1.png`
- 上升 / 下落：`up.png`、`fall.png`
- 落地：`land0.png`、`land1.png`
- 治疗 / 施法：`heal.png`、`cast_spell.png`

## 2.2 敌人与 Boss 资源

- 爬虫：`crawler0.png` ~ `crawler3.png`
- 飞虫：`flyer.png`
- Boss：`boss_stand.png`、`boss_jump.png`、`boss_air.png`、`boss_swing.png`、`boss_hit.png`

## 2.3 特效资源

- 斩击：`slash0.png` ~ `slash3.png`
- 命中爆点：`hit_impact*.png`

## 2.4 地图相关资源

- 平台：`platform.png`
- 地刺：`spike.png`
- 菜单背景：`bg.png`

## 2.5 音频与字体

- BGM：`bgm0.mp3` ~ `bgm3.mp3`
- 音效：`swing.wav`、`hit.wav`、`jump.wav`、`land.wav`、`dash.wav`、`walk.wav`、`run.wav`、`cast_spell.wav`、`heal.wav`
- 字体：`font.TTF`

## 3. 主题目录

## 3.1 `bg_plain/`

职责：

- 第 1 关平原主题背景
- 地表草地与泥土贴图

内容特点：

- `1.png` ~ `5.png`：多层视差背景
- `plain_grass.png`：顶层地表
- `plain_mud.png`：深层地表

## 3.2 `bg_cave/`

职责：

- 第 2 关洞穴主题背景

内容特点：

- `1.png` ~ `9.png`：更多层级的洞穴背景
- `cave_ground.png`：地表与深层共用贴图

## 3.3 `boss_cave/`

职责：

- 第 3 关 Boss 房背景

内容特点：

- `0.png` ~ `6.png`：Boss 房多层背景
- `boss_cave_ground.png`：Boss 房地表

## 4. 地图编码规则

关卡数据通过 `TileType` 编码，当前已确认如下：

```text
0  = Empty
1  = Wall
2  = Platform
3  = Void
4  = SpikeUp
5  = SpikeDown
6  = SpikeLeft
7  = SpikeRight
8  = SpawnCrawler
9  = SpawnFlyer
10 = SpawnPlayer
11 = LevelGoal
12 = Spawnboss
```

虽然 `SpikeDown / SpikeLeft / SpikeRight` 已在枚举中定义，但当前代码主要使用的是：

- `Wall`
- `Platform`
- `Void`
- `SpikeUp`
- 出生点与终点编码

## 5. 地图加载流程

`Map::loadLevel(levelIndex)` 的执行过程如下：

1. 调用 `unloadTheme()` 释放旧资源
2. 清空旧网格和旧出生点数据
3. 读取 `map{levelIndex}.csv`
4. 逐格解析整数值并转换为 `TileType`
5. 如果遇到 `10`，记录玩家出生点
6. 如果遇到 `11`，记录终点
7. 如果遇到 `8 / 9 / 12`，加入敌人出生点列表
8. 统计地图宽高
9. 根据关卡编号加载背景层和地表贴图

## 6. 关卡内容概览

## 6.1 `map1.csv`

特点：

- 尺寸明显大于 Boss 房
- 前期地形宽阔
- 承担初始玩法体验和基础敌人投放

主题资源：

- `bg_plain/`

## 6.2 `map2.csv`

特点：

- 同样是长卷式关卡
- 洞穴主题
- 用于中段推进

主题资源：

- `bg_cave/`

## 6.3 `map3.csv`

特点：

- 尺寸相对固定且封闭
- 明显是 Boss 房布局

关键行可以概括为：

```text
... 10 ... 12 ... 11 ...
```

含义：

- `10`：玩家出生点
- `12`：Boss 出生点
- `11`：终点

同时在第 3 关中，只有 Boss 死亡后终点才真正开放。

## 7. 资源绑定规则

项目没有独立配置文件描述“哪一关用哪些资源”，而是直接在 `Map::loadLevel()` 中按关卡号硬编码：

- `levelIndex == 1`
  - 加载 `bg_plain/`
- `levelIndex == 2`
  - 加载 `bg_cave/`
- `levelIndex == 3`
  - 加载 `boss_cave/`

这说明“关卡主题切换”是代码规则，而不是纯数据规则。

## 8. 渲染中的资源使用方式

## 8.1 地图贴图

墙体绘制时会根据上方 Tile 判断使用哪张地表纹理：

- 如果上方是空气，使用顶层地表
- 否则使用深层地表

这是一种简单但有效的“自动区分表层/深层地表”方案。

## 8.2 视差背景

每层背景都带 `scrollSpeed`：

- 数值越小，越接近远景
- 数值越大，越接近前景

渲染时根据玩家相机位置计算水平偏移，并通过重复绘制形成卷轴背景。

## 8.3 角色与敌人帧选择

角色帧和敌人帧不是由资源系统驱动，而是由主循环或对象状态直接决定：

- 玩家：由动作状态决定使用哪个动作帧数组
- 敌人：由敌人类型和动画帧号决定
- Boss：由 `bossState` 决定使用哪张专用贴图

## 9. 资源管理现状

当前项目的资源管理特点如下：

- 地图主题资源由 `Map` 负责加载和释放
- 大部分角色、敌人、音频资源由 `main.cpp` 一次性加载
- 没有统一的资源缓存器或引用计数
- 没有资源清单或校验层

这意味着：

- 项目体量小时开发效率高
- 项目体量变大后会出现资源路径维护困难

## 10. 后续维护建议

如果以后资源继续增加，建议考虑：

1. 将根目录资源进一步分到 `assets/characters`、`assets/audio`、`assets/effects`、`assets/tilesets`
2. 为地图 Tile 编码单独编写说明表或生成器脚本
3. 为资源加载增加失败日志
4. 将关卡主题配置从 `Map::loadLevel()` 中抽成可配置表
