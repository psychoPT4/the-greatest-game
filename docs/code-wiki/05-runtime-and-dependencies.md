# 依赖关系与运行方式

## 1. 构建入口

当前项目的正式构建入口是：

- `fulfilling knight.vcxproj`

仓库内未发现以下替代构建方案：

- `CMakeLists.txt`
- `Makefile`
- `Dockerfile`
- GitHub Actions / GitLab CI
- 自动化测试配置

因此推荐的理解方式是：

- 这是一个以 Visual Studio 为中心维护的本地游戏工程

## 2. 编译环境要求

根据 `.vcxproj`，当前已确认的环境要求包括：

- 编译器工具链：`MSVC v143`
- 目标平台：`Windows`
- 目标 SDK：`Windows SDK 10.0`
- 语言标准：`C++20`

工程定义了以下配置：

- `Debug|Win32`
- `Release|Win32`
- `Debug|x64`
- `Release|x64`

其中当前最明确配置完整的是：

- `Debug|x64`

## 3. 第三方依赖

### 3.1 Raylib

项目核心外部依赖是 `Raylib`，用于：

- 窗口管理
- 贴图加载
- 音频播放
- 字体加载
- 绘图
- 输入读取

工程文件中已写死本地 Raylib 头文件和库目录：

- `C:\tool\raylib-6.0_win64_msvc16\include`
- `C:\tool\raylib-6.0_win64_msvc16\lib`

链接库包括：

- `raylib.lib`
- `raylibdll.lib`
- `winmm.lib`

### 3.2 运行时 DLL

仓库中存在：

- `raylib.dll`

说明项目预期是在 Windows 下直接依赖 DLL 运行。

## 4. 运行所需文件

程序运行依赖大量相对路径资源，因此启动目录必须能够直接访问下列文件与目录：

### 4.1 代码与数据

- `map1.csv`
- `map2.csv`
- `map3.csv`
- `save.dat`（可选，第一次运行时不存在）

### 4.2 图形资源

- 玩家角色帧：`stand*.png`、`walk*.png`、`run*.png`、`attack*.png` 等
- 敌人与 Boss 贴图：`crawler*.png`、`flyer.png`、`boss_*.png`
- 关卡贴图：`platform.png`、`spike.png`
- 背景资源目录：`bg_plain/`、`bg_cave/`、`boss_cave/`

### 4.3 音频与字体

- `bgm0.mp3` ~ `bgm3.mp3`
- `swing.wav`、`hit.wav`、`jump.wav`、`dash.wav` 等
- `font.ttf` 或等效字体文件

## 5. 启动方式

## 5.1 推荐方式

在 Windows + Visual Studio 中：

1. 打开 `fulfilling knight.vcxproj`
2. 选择 `Debug|x64`
3. 确保本机已安装工程中指定路径的 Raylib
4. 以项目根目录作为工作目录启动

## 5.2 理论上的命令行方式

如果开发机环境已正确配置，也可以使用 MSBuild：

```powershell
msbuild "fulfilling knight.vcxproj" /p:Configuration=Debug /p:Platform=x64
```

但这依然本质上依赖 Windows + MSVC + Raylib 本地环境。

## 6. 程序启动后做什么

程序启动时会依次执行：

1. 创建窗口
2. 初始化音频设备
3. 设置目标帧率
4. 加载 BGM 和 SFX
5. 加载字体
6. 加载菜单背景图和角色/敌人贴图
7. 创建 `RenderTexture`
8. 初始化 `Player`、`Map`、`Enemy` 列表和 `GameCamera`
9. 进入主循环

这意味着运行失败通常会集中在：

- 资源文件缺失
- 路径大小写不匹配
- Raylib 环境不可用

## 7. 依赖关系

## 7.1 编译期依赖

```text
main.cpp
  ├─ map.h
  ├─ Player.h
  ├─ Enemy.h
  ├─ Camera.h
  └─ raylib.h

player.cpp
  ├─ Player.h
  └─ Enemy.h

enemy.cpp
  ├─ Enemy.h
  └─ raylib.h

map.cpp
  ├─ map.h
  └─ raylib.h

camera.h
  └─ raylib.h
```

## 7.2 运行期依赖

```text
可执行程序
  ├─ raylib.dll
  ├─ 角色与敌人贴图
  ├─ 关卡 CSV
  ├─ 音效与 BGM
  ├─ 字体
  └─ 背景资源目录
```

## 8. 存档机制

项目支持快速存档和快速读档：

- `F5` 保存
- `F9` 读档

### 8.1 存档文件

- 文件名：`save.dat`
- 文件头魔数：`FKGT`
- 版本号：`2`

### 8.2 存档内容

存档会持久化：

- 当前关卡编号
- 玩家生命、法力、体力、朝向、跳跃状态、速度等
- 当前安全落脚点
- 音量设置
- 击杀数
- 虚化能力解锁状态与冷却
- 游戏时间
- 所有敌人的状态
- 当前战斗日志文本

### 8.3 存档特点

- 手写二进制序列化
- 没有压缩或加密
- 读写效率高
- 结构变更时需要同步更新版本兼容逻辑

## 9. 已确认缺失项

当前仓库未发现：

- 自动化测试
- 基准测试
- 持续集成
- 自动发布流程
- 打包脚本

因此“项目运行方式”更多依赖开发者本地环境，而不是仓库内的标准化工具链。

## 10. 已知运行风险

在当前仓库中，已经观察到若干可能直接影响编译或运行的问题。

### 10.1 大小写不一致

源码包含：

- `#include "Player.h"`
- `#include "Enemy.h"`
- `#include "Camera.h"`

但仓库文件名实际为：

- `player.h`
- `enemy.h`
- `camera.h`

在 Windows 上通常不敏感，但在大小写敏感文件系统上会失败。

### 10.2 字体文件名不一致

代码加载的是：

- `font.ttf`

仓库中的文件是：

- `font.TTF`

这同样会在大小写敏感环境中导致问题。

### 10.3 命中特效文件名疑似不匹配

代码尝试加载：

- `hit_impact0.png`
- `hit_impact1.png`
- `hit_impact2.png`
- `hit_impact3.png`

仓库中的资源实际是：

- `hit_impact0.png.png`
- `hit_impact1.png.png`
- `hit_impact2.png.png`
- `hit_impact3.png.png`

因此命中特效贴图大概率无法正确加载。

### 10.4 疑似缺失的资源

代码中引用了：

- `wall.png`
- `bg_far.png`

但根目录未发现对应文件。

### 10.5 工程配置完整性

目前只有 `Debug|x64` 明确写入 Raylib include/lib 路径，其他配置是否能直接构建并不确定。

## 11. 实际维护建议

如果要把这个项目长期维护下去，建议优先补齐：

1. 统一资源与头文件大小写
2. 修复错误命名的命中特效资源
3. 补充 README 中的构建说明
4. 为资源缺失场景添加检查日志
5. 为 Windows 之外的平台增加构建方案
