# Fulfilling Knight Code Wiki

本目录是对当前仓库的结构化代码文档整理，目标是帮助后续开发者快速理解项目的整体架构、核心模块、关键类与函数、依赖关系、关卡数据以及运行方式。

## 文档导航

- `01-project-overview.md`
  - 项目定位、技术栈、仓库布局、核心玩法与总体设计判断
- `02-architecture.md`
  - 运行时架构、主循环、状态机、模块协作关系与关键数据流
- `03-modules.md`
  - `main.cpp`、`Player`、`Enemy`、`Map`、`GameCamera` 等模块的职责拆解
- `04-key-symbols.md`
  - 关键类、结构体、函数与代表性接口说明
- `05-runtime-and-dependencies.md`
  - 构建方式、依赖项、运行要求、存档机制、已知风险
- `06-assets-and-levels.md`
  - 资源组织方式、地图编码、关卡数据与主题资源加载规则

## 推荐阅读顺序

1. 先看 `01-project-overview.md`，建立整体认知
2. 再看 `02-architecture.md`，理解程序如何运行
3. 然后看 `03-modules.md` 与 `04-key-symbols.md`，进入模块与代码细节
4. 最后看 `05-runtime-and-dependencies.md` 与 `06-assets-and-levels.md`，补齐环境与数据层信息

## 项目一句话总结

这是一个基于 C++20 和 Raylib 的单可执行 2D 平台动作游戏项目，采用“单入口主循环 + 少量领域对象 + CSV 关卡驱动 + 根目录资源直读”的实现方式，核心逻辑集中在 `main.cpp`，玩家、敌人、地图和相机分别由独立模块承担。
