#pragma execution_character_set("utf-8")

//#include <mmstream.h>
#pragma comment(lib,"winmm.lib")

//头文件
#include <stdio.h>    // 文件操作、printf
#include <string.h>   // memset
#include <tchar.h>    // TCHAR 和 _T()
#include <conio.h>    // _kbhit() _getch()（EasyX 下可用 peekmessage 替代）
#include <windows.h>  // 颜色设置、Sleep休眠、GetTickCount
#include <mmsystem.h> // 【附带修复】MCI 播放音乐必需（mciSendString / mciGetErrorString）
#include <graphics.h> // EasyX 图形库
#include <math.h>     // 用来画圆角

/*
=========================================================
界面总览：
    主菜单界面 / 关卡选择界面 / 游戏运行界面 / 暂停界面 /
    剧情界面 / 结算界面 / 设置界面 / 团队介绍界面 /
    图鉴界面 / 无尽模式界面

主菜单界面：
    展示：顶部标题「萝莉王」；中间选项：关卡模式、无尽模式、
          无尽记录、团队介绍、设置、退出游戏。
    操作：鼠标移动高亮、左键点击。
    跳转：
        关卡模式 → STATE_LEVEL_SELECT
        无尽模式 → STATE_ENDLESS（需先通关全部 5 关）
        无尽记录 → STATE_ENDLESS（暂归同一界面）
        图鉴&玩法教学 → STATE_GALLERY
        团队介绍 → STATE_TEAM
        设置 → STATE_SETTINGS
        退出游戏 → SaveGame() 后退出

关卡选择界面：
    展示：5 个关卡按钮；已通关正常显示，未解锁置灰；
          返回主菜单按钮；每关下方显示敌人类型、数量、波次。
    跳转：点击已解锁关卡 → LoadLevel(i) → STATE_PLAYING；
          未解锁点击无响应；返回 → STATE_MENU。

团队介绍界面：
    展示：开发者名单与分工说明。
    跳转：返回 → STATE_MENU。

暂停菜单界面：
    展示：标题「游戏暂停」；选项：继续/重开/返回主菜单。
    触发：游戏中空格键，或点击左上角暂停按钮。
    跳转：
        继续 → 按模式返回 STATE_PLAYING / STATE_ENDLESS
        重新开始 → LoadLevel / LoadEndless 重载当前模式
        返回主菜单 → STATE_MENU
    备注：暂停期间敌人移动/塔攻击/刷怪全部冻结。

游戏主界面：
    展示：地图、路径、基地、敌人、塔；UI 显示金币/波次/
          剩余敌人/基地生命/关卡；左上角暂停按钮。
    跳转：
        暂停按钮或空格 → STATE_PAUSED
        ESC → STATE_MENU
        胜利或失败 → STATE_RESULT

结算界面：
    展示：胜利显示「恭喜通关」；失败显示「真遗憾呐……杂鱼」。
    跳转：胜利且非最后一关 → 下一关；否则 → 返回主菜单。

设置界面：
    展示：音量加减按钮、返回主菜单。
    跳转：返回 → STATE_MENU。

无尽模式界面：
    解锁条件：通关全部 5 关后解锁。
    展示：无尽地图（复用地图 3），敌人一波比一波强；
          UI 显示当前波数、历史最高波数、基地生命、金币。
    特殊机制：只有失败，没有胜利。

剧情界面：
    开局：进入关卡自动播放，播放期间游戏暂停，
          鼠标左键点击跳过进入游戏。
    通关：胜利后自动播放，点击跳过进入结算界面。
    备注：具体剧情文案待定。
=========================================================
*/

// ==================== 数据设计 ====================

/*
WINDOW_W / WINDOW_H
含义：游戏窗口宽高，单位：像素
取值：WINDOW_W=1280，WINDOW_H=720
*/
#define WINDOW_W 1280
#define WINDOW_H 720

/*
SCREEN_MARGIN_X / SCREEN_MARGIN_Y
含义：地图绘制区相对窗口的边距（用于居中），单位：像素
换算：地图绘制区宽度 = WINDOW_W - 2 * SCREEN_MARGIN_X
*/
#define SCREEN_MARGIN_X 100
#define SCREEN_MARGIN_Y 0

/*
MAPWINDOW_W / MAPWINDOW_H
含义：地图绘制区的宽高，单位：像素
取值：1050 × 650
*/
#define MAPWINDOW_W 1050
#define MAPWINDOW_H 650

/*
MAP_COUNT
含义：总关卡数量
*/
#define MAP_COUNT 5

/*
MAP_W / MAP_H
含义：地图宽度列数、高度行数
*/
#define MAP_W 21
#define MAP_H 13

/*
TILE_SIZE_X / TILE_SIZE_Y
含义：地图格子的像素尺寸
换算：TILE_SIZE_X = MAPWINDOW_W / MAP_W = 50；TILE_SIZE_Y 同理
*/
#define TILE_SIZE_X 50
#define TILE_SIZE_Y 50

/*
PATH_COUNT
含义：每张地图最多路径条数
*/
#define PATH_COUNT 2

/*
FPS
含义：游戏目标帧率（CD 用帧数计算的基准）
*/
#define FPS 60

/*
MAX_PATH_NODES
含义：一条路径最多节点数
*/
#define MAX_PATH_NODES 100

/*
MAX_ENEMIES
含义：敌人数组最大容量
*/
#define MAX_ENEMIES 200

/*
MAX_TOWERS
含义：防御塔数组最大容量
*/
#define MAX_TOWERS 100

/*
MAX_ENEMY_TYPES
含义：敌人种类数
*/
#define MAX_ENEMY_TYPES 4

/*
MAX_WAVES
含义：每关最多波数（无尽模式需要较大容量）
*/
#define MAX_WAVES 200

/*
MAX_SPAWN_GROUP
含义：一波最多同时出几种怪
*/
#define MAX_SPAWN_GROUP 8

/*
MAX_TOWER_TYPES
含义：防御塔种类数
*/
#define MAX_TOWER_TYPES 3

/*
INIT_MONEY
含义：开局初始金币
*/
#define INIT_MONEY 150

/*
INIT_BASE_HP
含义：基地初始生命值
*/
#define INIT_BASE_HP 20

/*
TOTAL_LEVELS
含义：总关卡数（与 MAP_COUNT 一致）
*/
#define TOTAL_LEVELS 5

/*
SAVE_FILE
含义：本地存档文件名
*/
#define SAVE_FILE _T("loli_save.dat")


/*
Count_1a,Count_1b,Count_2a,Count_2b,Count_3a,Count_3b,Count_4a,Count_4b,Count_5a,Count_5b
含义:剧情图数量
(如Count_1a是第一关开始剧情图数量,Count_2b是第二关结束剧情图数量)
*/
#define Count_1a 8
#define Count_1b 12
#define Count_2a 9
#define Count_2b 9
#define Count_3a 10
#define Count_3b 8
#define Count_4a 8
#define Count_4b 13
#define Count_5a 16
#define Count_5b 22


/*
Point
含义：二维坐标点
*/
typedef struct {
    int x;  // 横坐标
    int y;  // 纵坐标
} Point;

/*
Path
含义：一条敌人路径
*/
typedef struct {
    Point nodes[MAX_PATH_NODES];  // 路径节点数组
    int count;                    // 实际节点数量
} Path;

/*
SpawnGroup
含义：出怪小组：定义这一波里，哪种怪，出多少只，走哪条路
*/
typedef struct {
    int enemyType; // 敌人类型（0=Yh, 1=Aw, 2=Dd, 3=BOSS）
    int count;     // 数量
    int pathId;    // 路径：0=第1条, 1=第2条
} SpawnGroup;

/*
WaveConfig
含义：一波波次配置
*/
typedef struct {
    SpawnGroup groups[MAX_SPAWN_GROUP]; // 这一波的出怪组合
    int groupCount;                     // 本波使用的 group 数量
    float spawnInterval;                // 同一波内出怪间隔（秒）
    float waveDelay;                    // 本波清空后等待多少秒出下一波
    int isBossWave;                     // 1=BOSS 专属波次
} WaveConfig;

/*
Base
含义：玩家基地
*/
typedef struct {
    int hp;       // 当前生命
    int maxHp;    // 最大生命
    int x;        // 基地中心像素 x
    int y;        // 基地中心像素 y
    int radius;   // 基地半径（像素）
} Base;

/*
EnemyConfig
含义：敌人配置表（一种敌人一条）
*/
typedef struct {
    int hp;        // 最大血量
    float speed;   // 移动速度（像素/秒）
    int damage;    // 撞基地扣除的基地生命
    int reward;    // 击杀奖励金币
} EnemyConfig;

/*
Enemy
含义：一个敌人的运行时数据
*/
typedef struct {
    float x, y;       // 当前像素坐标
    int hp;           // 当前血量
    int maxHp;        // 最大血量
    int damage;       // 撞基地扣血
    float speed;      // 移动速度（像素/秒）
    int pathIndex;    // 当前目标路径节点下标，初始 1
    int alive;        // 1=存活，0=死亡
    int reward;       // 击杀奖励金币
    int type;         // 敌人类型下标
    float slowTimer;  // 减速剩余时间（秒）
    int pathId;       // 走哪条路径 0/1
    int isBoss;       // 1=BOSS
} Enemy;

/*
TowerConfig
含义：防御塔配置表（一种塔一条）
*/
typedef struct {
    int cost;            // 造价金币
    int atk;             // 攻击力
    int rangeGrid;       // 攻击范围 N×N 的 N
    float attackInterval;// 攻击间隔（秒）
    int unlockLevel;     // 解锁关卡（0~4）
    int special;         // 攻击类型：SINGLE / AOE
} TowerConfig;

/*
Tower
含义：一个防御塔的运行时数据
*/
typedef struct {
    int x, y;              // 塔中心像素坐标
    int atk;               // 攻击力
    int rangeGrid;         // 攻击范围 N×N 的 N
    float attackInterval;  // 攻击间隔（秒）
    float cooldown;        // 剩余冷却；<=0 表示可攻击
    int cost;              // 造价（用于返还）
    int placed;            // 1=已放置，0=空槽
    int type;              // 塔类型（towerConfigs 下标）
    int special;           // 攻击类型（从配置表复制）
} Tower;

/*
GameState
含义：当前界面状态
*/
typedef enum {
    STATE_MENU,          // 主菜单
    STATE_LEVEL_SELECT,  // 关卡选择
    STATE_PLAYING,       // 游戏进行中
    STATE_PAUSED,        // 暂停
    STATE_SETTINGS,      // 设置
    STATE_TEAM,          // 团队介绍
    STATE_STORY,         // 剧情
    STATE_RESULT,        // 结算（胜利/失败）
    STATE_ENDLESS,       // 无尽模式
    // ===== 新增图鉴相关 =====
    STATE_GALLERY,        // 图鉴主界面
    STATE_GALLERY_FRIEND, // 友方展示
    STATE_GALLERY_ENEMY,  // 敌方展示
    STATE_HINATA,         // 日向图鉴
    STATE_CHINO,          // 智乃图鉴
    STATE_KANNA,          // 康娜图鉴
    STATE_YH,             // Yh 图鉴
    STATE_AW,             // Aw 图鉴
    STATE_DD,             // Dd 图鉴
    STATE_01,             // [01] 图鉴
    // =========================
    STATE_EXIT            // 退出游戏
} GameState;

/*
TowerSpecialType
含义：塔的攻击类型
*/
typedef enum {
    SINGLE = 0, // 单体攻击
    AOE = 1     // 群体攻击
} TowerSpecialType;

/*
TowerName
含义：塔的名字枚举（增加可读性）
*/
typedef enum {
    Hinata = 0, // 日向
    Chino = 1,  // 智乃
    Kanna = 2   // 康娜
} TowerName;

/*
Game
含义：全局唯一游戏数据
分类：
    [持续存在] mapData / paths / enemyConfigs / towerConfigs / levelWaves /
              highScore / unlockedCount / endlessBestWave / soundOn / volume
    [单局存在] base / money / waveIndex / totalWaves / currentMap /
              enemies[] / towers[] / spawnTimer / enemiesRemaining /
              baseFlashTimer / selectedTowerType / isEndlessMode
*/
typedef struct {
    // ========== 持续数据（跨局保存） ==========
    int mapData[MAP_COUNT][MAP_H][MAP_W];         // 所有地图格子数据
    Path paths[MAP_COUNT][PATH_COUNT];            // 所有地图路径数据
    EnemyConfig enemyConfigs[MAX_ENEMY_TYPES];    // 敌人配置表
    TowerConfig towerConfigs[MAX_TOWER_TYPES];    // 防御塔配置表
    WaveConfig levelWaves[MAP_COUNT][MAX_WAVES];  // 所有关卡波次配置
    int highScore;                                // 历史最高分
    int unlockedCount;                            // 已解锁关卡数
    int endlessBestWave;                          // 无尽模式最高波数
    int soundOn;                                  // 音效开关（1=开，0=关）
    int volume;                                   // 音量（0~100）

    // ========== 单局数据（每局开始重置） ==========
    Base base;                              // 基地
    int money;                              // 当前金币
    int totalWaves;                         // 当前关卡总波数
    int waveIndex;                          // 当前波次下标
    int currentMap;                         // 当前关卡下标
    Enemy enemies[MAX_ENEMIES];             // 场上敌人数组
    int enemyCount;                         // 敌人数组已用长度（含死亡槽位）
    Tower towers[MAX_TOWERS];               // 已建塔数组
    int towerCount;                         // 已建塔数量
    float spawnTimer;                       // 出怪计时器
    int enemiesRemaining;                   // 本波剩余待生成敌人数
    float baseFlashTimer;                   // 基地受击红圈计时器
    int hoverGridX;                         // 鼠标悬停格子 X
    int hoverGridY;                         // 鼠标悬停格子 Y
    int selectedTowerType;                  // 当前选中塔（-1=未选中）
    int storyPhase;                         // 剧情阶段 0/1/2
    GameState state;                        // 当前界面状态
    int groupIndex;                         // 当前波次刷到第几组
    int currentGroupSpawnedCount;           // 当前组已刷出多少只
    int mouseX;                             // 鼠标当前 X
    int mouseY;                             // 鼠标当前 Y
    int endlessWave;                        // 无尽模式当前波数
    int isEndlessMode;                      // 1=无尽模式
} Game;

/*
game
含义：全局唯一游戏对象
*/
Game game;

// ==================== 资源文件区 ====================

IMAGE im_menuBg;         // 主菜单背景图
IMAGE im_level_selectBg; // 关卡选择背景图
IMAGE im_mapBg;          // 游戏地图背景图
IMAGE im_pauseBg;        // 暂停界面背景图
IMAGE im_setBg;          // 设置界面背景图
IMAGE im_galBg;          // 图鉴主界面背景图
IMAGE im_Friend;         // 友方图鉴主界面背景图
IMAGE im_Enemy;          // 敌方图鉴主界面背景图
IMAGE im_Hinata;         // 日向图鉴界面背景图
IMAGE im_Chino;          // 智乃图鉴界面背景图
IMAGE im_Kanna;          // 康娜图鉴界面背景图
IMAGE im_Yh;             // Yh 图鉴界面背景图
IMAGE im_Aw;             // Aw 图鉴界面背景图
IMAGE im_Dd;             // Dd 图鉴界面背景图
IMAGE im_01;             // [01] 图鉴界面背景图
IMAGE Detail;            // 团队介绍界面背景图
IMAGE im_resultBg;       // 结算界面背景图

IMAGE Show_info;         // UI 展示板
IMAGE Bt_pause;          // 暂停按钮
IMAGE Bt_Hinata;         // 日向塔按钮
IMAGE Bt_Chino;          // 智乃塔按钮
IMAGE Bt_Kanna;          // 康娜塔按钮

IMAGE im_towerHinata_color;      // 日向塔正常态原色图
IMAGE im_towerHinata_atk_color;  // 日向塔攻击态原色图
IMAGE im_towerHinata_mask;       // 日向塔正常态掩码图
IMAGE im_towerHinata_atk_mask;   // 日向塔攻击态掩码图
IMAGE im_towerChino_color;       // 智乃塔正常态原色图
IMAGE im_towerChino_atk_color;   // 智乃塔攻击态原色图
IMAGE im_towerChino_mask;        // 智乃塔正常态掩码图
IMAGE im_towerChino_atk_mask;    // 智乃塔攻击态掩码图
IMAGE im_towerKanna_color;       // 康娜塔正常态原色图
IMAGE im_towerKanna_atk_color;   // 康娜塔攻击态原色图
IMAGE im_towerKanna_mask;        // 康娜塔正常态掩码图
IMAGE im_towerKanna_atk_mask;    // 康娜塔攻击态掩码图

IMAGE im_enemyYh_color;      // Yh 敌人原色图
IMAGE im_enemyYh_mask;       // Yh 敌人掩码图
IMAGE im_enemyAw_color;      // Aw 敌人原色图
IMAGE im_enemyAw_mask;       // Aw 敌人掩码图
IMAGE im_enemyDd_color;      // Dd 敌人原色图
IMAGE im_enemyDd_mask;       // Dd 敌人掩码图
IMAGE im_boss_color;         // BOSS 原色图
IMAGE im_boss_mask;          // BOSS 掩码图
IMAGE a1[Count_1a];      //第1关开始剧情图 
IMAGE b1[Count_1b];      //第1关结束剧情图 
IMAGE a2[Count_2a];      //第2关开始剧情图 
IMAGE b2[Count_2b];      //第2关结束剧情图 
IMAGE a3[Count_3a];      //第3关开始剧情图 
IMAGE b3[Count_3b];      //第3关结束剧情图 
IMAGE a4[Count_4a];      //第4关开始剧情图 
IMAGE b4[Count_4b];      //第4关结束剧情图 
IMAGE a5[Count_5a];      //第5关开始剧情图 
IMAGE b5[Count_5b];      //第5关结束剧情图 

/***************修改*****************/
/* 剧情翻页控制：当前正在显示的剧情图下标（从 0 开始）。
 * Story_update 负责递增、DrawStory 按此下标取图；
 * 每次进入一段新剧情（关卡开始 / 胜利结束）时都会被重置为 0。 */
int pageNow = 0;
/***************修改*****************/

//音乐文件
const wchar_t* bgm[] = {
    L"menu.mp3",   // 0 主菜单
    L"game1.mp3",  // 1 第1关
    L"game2.mp3",  // 2 第2关
    L"game3.mp3",  // 3 第3关
    L"game4.mp3",  // 4 第4关
    L"game5.mp3",  // 5 第5关
    L"win.mp3",    // 6 胜利
    L"1a.mp3",     // 7 第1关开始剧情
    L"1b.mp3",     // 8 第1关结束剧情
    L"2a.mp3",     // 9 第2关开始剧情
    L"2b.mp3",     // 10 第2关结束剧情
    L"3a.mp3",     // 11 第3关开始剧情
    L"3b.mp3",     // 12 第3关结束剧情
    L"4a.mp3",     // 13 第4关开始剧情
    L"4b.mp3",     // 14 第4关结束剧情
    L"5a.mp3",     // 15 第5关开始剧情
    L"5b.mp3",     // 16 第5关结束剧情
};

int currentBgmIndex = -1;  // 当前播放的音乐索引，-1=没播

/*
Button
含义：按钮数据
*/
typedef struct {
    int x, y, w, h;      // 按钮左上角坐标和宽高
    const TCHAR* text;   // 按钮显示文字（用 _T() 包装）
} Button;

// ==================== service 层 ====================

//音量调节
/* 功能：设置当前 BGM 的音量（0~100）。
 * 参数：volume 音量，内部会被钳制到 [0,100]，再 *10 后发给 MCI。 */
void SetBGMVolume(int volume);

//无尽模式新增
/* 功能：判断某座塔在当前模式下是否解锁。
 *      无尽模式恒返回 1（全解锁）；
 *      普通模式比较 currentMap 与 towerConfigs[type].unlockLevel。 */
int IsTowerUnlocked(const Game* g, int towerType);

//service层需要提供一个InitAssets函数
/* 功能：游戏启动时一次性从硬盘加载所有图片资源到内存。
 * 参数：无
 * 返回值：void */
void InitAssets(void);

//service层需要提供一个InitConfigs函数
/* 功能：
 *   1. 初始化 towerConfigs[3]（Hinata / Chino / Kanna 的造价、攻击力、
 *      范围、攻速、解锁关卡、攻击类型）。
 *   2. 初始化 enemyConfigs[4]（Yh / Aw / Dd / BOSS 的血量、速度、
 *      基地伤害、击杀奖励）。
 * 参数：Game* g 全局游戏数据
 * 返回值：void */
void InitConfigs(Game* g);

//service层需要提供一个InitWaveConfigs函数
/* 功能：按需求文档【第1~5关波次设计】初始化
 *      g->levelWaves[MAP_COUNT][MAX_WAVES]。
 *   1. 第1关：纯 Yh，5 波，3/5/7/8/10，波次间隔 8s。
 *   2. 第2关：Yh + Aw（Aw 从第 3 波开始），6 波，间隔 7s。
 *   3. 第3关：双路径，7 波，数量约第2关 ×1.5，间隔 6s。
 *   4. 第4关：加入 Dd（肉盾，从第4波起），8 波，间隔 6s。
 *   5. 第5关：BOSS 关，4 波小怪，波间 5s；
 *      BOSS 不通过波次生成，由 LoadLevel 直接生成。
 * 参数：Game* g 全局游戏数据
 * 返回值：void */
void InitWaveConfigs(Game* g);

//service层需要提供一个InitData函数
/* 功能：程序启动时的统一初始化入口：
 *   1. memset 清空整个 Game。
 *   2. 填默认全局数据（音效、音量、无尽最高波、已解锁关卡数等）。
 *   3. 依次调用 InitConfigs / InitWaveConfigs / InitMaps。
 *   4. ResetRound 清空单局数据。
 *   5. （可开）LoadGame 读取本地存档覆盖设置与进度。
 * 参数：Game* g 全局游戏数据
 * 返回值：void */
void InitData(Game* g);

//service层需要提供一个ResetRound函数
/* 功能：重置“单局数据”，用于重新开始、切关、进入下一关前清场。
 *      具体重置 base / money / waveIndex / enemies / towers /
 *      spawnTimer / enemiesRemaining / baseFlashTimer /
 *      hoverGrid / selectedTowerType / isEndlessMode 等。
 *      不清空 mapData / paths / enemyConfigs / towerConfigs / 存档数据。
 * 参数：Game* g 全局游戏数据
 * 返回值：void */
void ResetRound(Game* g);

//service层需要提供一个LoadLevel函数
/* 功能：把第 level 关的数据载入 g（关卡下标 0~4）：
 *   1. ResetRound 清空上一局残留。
 *   2. 设 currentMap = level；若 level+1 > unlockedCount 则解锁。
 *   3. 基地血量 = INIT_BASE_HP；基地位置 = 该关所有路径终点坐标的平均。
 *   4. 初始金币按关卡：第1~2关150，第3~4关200，第5关250。
 *   5. 统计 levelWaves[level] 有效波次写入 totalWaves，初始化出怪状态。
 *   6. 若 level == 4（第5关），开局直接生成 BOSS 并标记 isBoss=1、speed=0。
 * 参数：Game* g 全局游戏数据；int level 关卡下标 0~4
 * 返回值：void */
void LoadLevel(Game* g, int level);

//service层需要提供一个HasNextLevel函数
/* 功能：判断当前关是否还有下一关。
 *      第1~4关返回 1，第5关返回 0。
 * 参数：const Game* g 全局游戏数据
 * 返回值：int 1=有下一关，0=已通关全部 5 关 */
int HasNextLevel(const Game* g);

//service层需要提供一个SpawnUpdate函数
/* 功能：按关卡波次配置定时生成敌人：
 *   阶段A：enemiesRemaining > 0 时，spawnTimer 累加 dt，
 *          达到 spawnInterval 就从当前 group 取一种怪 SpawnOneEnemy。
 *   阶段B：enemiesRemaining == 0 且场上小怪清空时，
 *          累加 waveDelay，到点进入下一波；无尽模式下一波会被
 *          GenerateEndlessWave 动态覆盖。
 * 参数：Game* g 全局游戏数据；float dt 本帧时间（秒）
 * 返回值：void */
void SpawnUpdate(Game* g, float dt);

//service层需要提供一个SpawnOneEnemy函数
/* 功能：从出生点生成一个 type 型敌人，走 pathId 指定路径：
 *   1. 优先复用已死亡槽位；找不到再尾部追加（不超过 MAX_ENEMIES）。
 *   2. 从 enemyConfigs[type] 拷贝属性。
 *   3. 无尽模式额外按波数缩放血量/速度。
 *   4. 初始坐标取 paths[currentMap][pathId] 的起点格中心。
 *   5. pathIndex=1、alive=1、slowTimer=0、isBoss=0。
 * 参数：Game* g 全局游戏数据；int type 敌人类型；int pathId 路径 0/1
 * 返回值：void */
void SpawnOneEnemy(Game* g, int type, int pathId);

//service层需要提供一个EnemyMove函数
/* 功能：敌人沿指定路径逐点移动：
 *   1. BOSS 跳过（原地待命）。
 *   2. 按 speed*dt 计算步长；slowTimer>0 时速度减半。
 *   3. 到达当前节点则吸附并 pathIndex++，继续下一节点。
 *   4. pathIndex >= path->count 表示已到终点，交由 BaseHitCheck 判定。
 * 参数：Game* g 全局游戏数据；float dt 本帧时间（秒）
 * 返回值：void */
void EnemyMove(Game* g, float dt);

//service层需要提供一个UpdateEnemyStatus函数
/* 功能：递减 slowTimer；归零后清空（速度在 EnemyMove 中动态计算，
 *      无需恢复原值）。
 * 参数：Game* g 全局游戏数据；float dt 本帧时间（秒）
 * 返回值：void */
void UpdateEnemyStatus(Game* g, float dt);

//service层需要提供一个TowerAttackUpdate函数
/* 功能：防御塔自动攻击：
 *   1. cooldown 每帧减 dt，未归零则跳过。
 *   2. 归零后遍历敌人，用 N×N 方形范围判定命中。
 *   3. Hinata 单体/无特效；Chino 命中减速 2 秒；
 *      Kanna 群攻且最多命中 3 个。
 *   4. 敌人 hp<=0 时调用 KillEnemy。
 *   5. 只有本帧真的命中(hitAny)才重置 cooldown=attackInterval。
 * 参数：Game* g 全局游戏数据；float dt 本帧时间（秒）
 * 返回值：void */
void TowerAttackUpdate(Game* g, float dt);

//service层需要提供一个KillEnemy函数
/* 功能：敌人死亡处理：
 *   1. alive = 0，调用 PickReward 发金币。
 *   2. 第5关且死的是小怪 → 找到 BOSS 扣 25% 最大血量；
 *      BOSS 血量归零则一并 alive=0 并发奖励。
 * 参数：Game* g 全局游戏数据；int enemyIndex 敌人下标
 * 返回值：void */
void KillEnemy(Game* g, int enemyIndex);

//service层需要提供一个PickReward函数
/* 功能：金币 += enemies[enemyIndex].reward。
 * 参数：Game* g 全局游戏数据；int enemyIndex 敌人下标
 * 返回值：void */
void PickReward(Game* g, int enemyIndex);

//service层需要提供一个BaseHitCheck函数
/* 功能：基地碰撞检测：
 *   1. 遍历存活敌人，判断所在格子与基地格子横竖差 ≤1。
 *   2. 命中后 base.hp -= enemy.damage，触发 baseFlashTimer = 0.2s。
 *   3. 敌人 alive=0（不调 KillEnemy，避免重复发奖励）。
 * 参数：Game* g 全局游戏数据
 * 返回值：void */
void BaseHitCheck(Game* g);

//service层需要提供一个CanPlaceAt函数
/* 功能：判断格子(gx,gy)是否可建造：
 *   1. 坐标必须在 [0,MAP_W)×[0,MAP_H)。
 *   2. mapData[currentMap][gy][gx] == 2（可建造高台）。
 *   3. 该格子上没有已放置的塔。
 * 参数：const Game* g 全局游戏数据；int gx,gy 格子下标
 * 返回值：int 1=可建造，0=不可建造 */
int CanPlaceAt(const Game* g, int gx, int gy);

//service层需要提供一个PlaceTower函数
/* 功能：在 (gx,gy) 放一座 type 型塔：
 *   1. CanPlaceAt 检查；IsTowerUnlocked 检查解锁；money 检查。
 *   2. 成功则扣钱，写入 towers[towerCount]，towerCount++。
 * 参数：Game* g；int gx,gy 格子下标；int type 塔下标
 * 返回值：int 1=成功，0=不可建，-1=金币不足，-2=未解锁 */
int PlaceTower(Game* g, int gx, int gy, int type);

//service层需要提供一个RemoveTower函数
/* 功能：拆除 (gx,gy) 上的已建塔：
 *   1. 遍历 towers，匹配像素坐标反算出的格子。
 *   2. placed=0，清空该塔字段，返还 50% 造价。
 * 参数：Game* g；int gx,gy 格子下标
 * 返回值：int 1=拆除成功，0=该格无塔 */
int RemoveTower(Game* g, int gx, int gy);

//service层需要提供一个CountAliveEnemies函数
/* 功能：统计当前存活的小怪数量（排除 BOSS）。
 * 参数：const Game* g 全局游戏数据
 * 返回值：int 存活小怪数 */
int CountAliveEnemies(const Game* g);

//service层需要提供一个CheckWinLose函数
/* 功能：每帧判定胜负：
 *   1. base.hp <= 0 → STATE_RESULT（失败）。
 *   2. waveIndex >= totalWaves 且 enemiesRemaining==0 且
 *      场上无存活小怪且（非第5关 或 BOSS 已死）→ STATE_RESULT（胜利）。
 *   3. 其余 → STATE_PLAYING。
 *   注意：无尽模式不走此函数，由主循环单独判失败。
 * 参数：Game* g 全局游戏数据
 * 返回值：GameState STATE_RESULT / STATE_PLAYING */
GameState CheckWinLose(Game* g);

//service层需要提供一个SaveGame函数
/* 功能：保存本地存档（save_llw.txt）：
 *      unlockedCount、highScore、endlessBestWave 三项。
 * 参数：const Game* g 全局游戏数据
 * 返回值：void */
void SaveGame(const Game* g);

//service层需要提供一个LoadGame函数
/* 功能：读取本地存档；无存档则使用默认值（unlockedCount=1 等）。
 * 参数：Game* g 全局游戏数据
 * 返回值：void */
void LoadGame(Game* g);

//service层需要提供一个InitMaps函数
/* 功能：初始化地图与路径：
 *   1. 清空 mapData / paths。
 *   2. 手写 5 关的路径节点（路径0、路径1）。
 *   3. 用 kMapCell 模板填充 mapData；再把路径格子标记为 1。
 * 参数：Game* g 全局游戏数据
 * 返回值：void */
void InitMaps(Game* g);

//service层需要提供一个pixelToGrid函数
/* 功能：像素坐标 → 地图格子下标：
 *   gx=(px-SCREEN_MARGIN_X)/TILE_SIZE_X, gy=(py-SCREEN_MARGIN_Y)/TILE_SIZE_Y。
 *   越界/地图外返回 (-1,-1)。
 * 参数：int px,py 像素坐标
 * 返回值：Point 格子坐标 */
Point pixelToGrid(int px, int py);

//service层需要一个LoadEndless函数
/* 功能：进入无尽模式：
 *   1. ResetRound；isEndlessMode=1；currentMap=3（第4关地图，双路径）。
 *   2. 基地血量/位置（用地图3路径终点平均值）。
 *   3. 初始金币 200；出怪状态清零；totalWaves=MAX_WAVES。
 *   4. 立即 GenerateEndlessWave(0) 并统计第0波怪数。
 * 参数：Game* g 全局游戏数据
 * 返回值：void */
void LoadEndless(Game* g);

//service层需要一个GenerateEndlessWave函数
/* 功能：按波数动态生成 g->levelWaves[currentMap][waveIndex]：
 *   1. 难度公式：Yh=5+2L；Aw（L≥5）=2+(L-5)；Dd（L≥10）=1+(L-10)/2。
 *   2. 数量平分到两条路径（多出的部分给路径0）。
 *   3. 交错填充 groups 数组，让两路交替出怪。
 *   4. spawnInterval=0.8s, waveDelay=5.0s, isBossWave=0。
 * 参数：Game* g 全局游戏数据；int waveIndex 波次下标
 * 返回值：void */
void GenerateEndlessWave(Game* g, int waveIndex);

//service层需要提供一个PlayBGM函数
/* 功能：播放指定 BGM：
 *   - index 越界直接返回；
 *   - 与 currentBgmIndex 相同则不重复播放；
 *   - 播放前先 close 上一次的 MCI 别名，open 后立即设置音量并 play；
 *   - repeat 只用于后续 BgmKeepPlaying 的续播判定（wav 本身不支持 repeat）。
 * 参数：int index bgm 数组下标；int repeat 1=循环，0=只播一遍
 * 返回值：void */
void PlayBGM(int index, int repeat);

//service层需要提供一个StopBGM函数
/* 功能：停止当前 BGM 并关闭 MCI 别名，将 currentBgmIndex 复位为 -1。
 * 参数：无
 * 返回值：void */
void StopBGM();

//service层需要提供一个UpdateBGM函数
/* 功能：根据 game.state 切歌：
 *   - soundOn == 0 直接 Stop 并返回；
 *   - MENU/LEVEL_SELECT → PlayBGM(0, 循环)；
 *   - PLAYING → PlayBGM(currentMap+1, 循环)；
 *   - RESULT → PlayBGM(6, 只播一遍)；
 *   - ENDLESS → PlayBGM(5, 循环)；
 *   - 每帧末尾调用 BgmKeepPlaying 实现 wav 单曲循环。
 * 参数：Game* g 全局游戏数据
 * 返回值：void */
void UpdateBGM(Game* g);

// ==================== view 层 ====================

//view层需要提供一个Button_isClicked函数
/* 功能：判断鼠标左键是否点击在按钮内。
 * 参数：const Button* btn 按钮；const ExMessage* msg 当前输入消息
 * 返回值：int 1=命中，0=未命中 */
int Button_isClicked(const Button* btn, const ExMessage* msg);

//view层需要提供一个Button_draw函数
/* 功能：绘制按钮背景（渐变圆角）+ 边框 + 居中文字。
 * 参数：const Button* btn 按钮
 * 返回值：void */
void Button_draw(const Button* btn);

//view层需要提供一个Menu_update函数
/* 功能：主菜单输入处理：
 *   左键点击“开始游戏”→ STATE_LEVEL_SELECT
 *   左键点击“无尽模式”→ 若 unlockedCount>=5，LoadEndless 后 STATE_ENDLESS
 *   左键点击“图鉴&玩法教学”→ STATE_GALLERY
 *   左键点击“设置”→ STATE_SETTINGS
 *   左键点击“制作名单&声明”→ STATE_TEAM
 *   左键点击“残忍退出”→ SaveGame 后 STATE_EXIT
 * 参数：Game* g；ExMessage* msg
 * 返回值：GameState 下一状态 */
GameState Menu_update(Game* g, ExMessage* msg);

//view层需要提供一个DrawMenu函数
/* 功能：绘制主菜单：清屏 + 背景图 + 6 个选项（悬停高亮）。
 * 参数：const Game* g
 * 返回值：void */
void DrawMenu(const Game* g);

//view层需要提供一个LevelSelect_update函数
/* 功能：关卡选择输入处理：
 *   点击已解锁关卡 → LoadLevel(i) → STATE_PLAYING
 *   点击未解锁关卡 → 无响应
 *   点击“返回主菜单” → STATE_MENU
 * 参数：Game* g；ExMessage* msg
 * 返回值：GameState 下一状态 */
GameState LevelSelect_update(Game* g, ExMessage* msg);

//view层需要提供一个DrawLevelSelect函数
/* 功能：绘制关卡选择：清屏 + 背景图 + 5 个关卡按钮 + 返回按钮，
 *      鼠标悬停时高亮外框。
 * 参数：const Game* g
 * 返回值：void */
void DrawLevelSelect(const Game* g);

//view层需要提供一个Team_update函数
/* 功能：团队介绍界面输入处理：点击返回 → STATE_MENU。
 * 参数：Game* g；ExMessage* msg
 * 返回值：GameState 下一状态 */
GameState Team_update(Game* g, ExMessage* msg);

//view层需要提供一个DrawTeam函数
/* 功能：绘制团队介绍界面：清屏 + 背景图 + 返回按钮（悬停高亮）。
 * 参数：const Game* g
 * 返回值：void */
void DrawTeam(const Game* g);

//view层需要提供一个Pause_update函数
/* 功能：暂停菜单输入处理：
 *   - 点击“继续游戏”或空格：按 isEndlessMode 返回 STATE_ENDLESS 或 STATE_PLAYING；
 *   - 点击“重新开始”：无尽模式 LoadEndless，普通模式 LoadLevel(currentMap)；
 *   - 点击“返回主菜单”：清 isEndlessMode 后 STATE_MENU。
 * 参数：Game* g；ExMessage* msg
 * 返回值：GameState 下一状态 */
GameState Pause_update(Game* g, ExMessage* msg);

//view层需要提供一个DrawPause函数
/* 功能：绘制暂停菜单：清屏 + 背景图 + 三个按钮（悬停高亮）。
 * 参数：const Game* g
 * 返回值：void */
void DrawPause(const Game* g);

//view层需要提供一个GameView_update函数
/* 功能：游戏主界面输入处理：
 *   1. 键盘：空格→STATE_PAUSED，ESC→STATE_MENU。
 *   2. 鼠标左键：
 *      - 左上角暂停按钮 → STATE_PAUSED；
 *      - 右上三个塔按钮（需 IsTowerUnlocked）→ 切换 selectedTowerType；
 *      - 点击地图 → pixelToGrid + CanPlaceAt + PlaceTower。
 *   3. 鼠标右键：pixelToGrid 后 RemoveTower。
 * 参数：Game* g；ExMessage* msg
 * 返回值：GameState 通常为 STATE_PLAYING */
GameState GameView_update(Game* g, ExMessage* msg);

//view层需要提供一个GameRender函数
/* 功能：渲染游戏主界面（固定顺序）：
 *   1. cleardevice + 地图背景 + 右上角 UI 框 + DrawUI + 暂停按钮。
 *   2. 三个塔按钮（按 IsTowerUnlocked 显示，选中时画红框）。
 *   3. 遍历 towers 画塔，攻击态时附加范围圆。
 *   4. 遍历 enemies 画敌人 + 血条。
 *   5. baseFlashTimer>0 时叠红圈。
 * 参数：const Game* g
 * 返回值：void */
void GameRender(const Game* g);

//view层需要提供一个Story_update函数
/* 功能：剧情对话层输入处理（占位）。
 * 参数：Game* g；ExMessage* msg
 * 返回值：GameState 默认 STATE_STORY */
GameState Story_update(Game* g, ExMessage* msg);

//view层需要提供一个DrawStory函数
/* 功能：绘制剧情对话层（占位）。
 * 参数：const Game* g
 * 返回值：void */
void DrawStory(const Game* g);

//view层需要提供一个Result_update函数
/* 功能：结算界面输入处理：
 *   - 点击按钮：若胜利且还有下一关 → LoadLevel(currentMap+1) → STATE_PLAYING；
 *   - 否则 → STATE_MENU。
 * 参数：Game* g；ExMessage* msg
 * 返回值：GameState 下一状态 */
GameState Result_update(Game* g, ExMessage* msg);

//view层需要提供一个DrawResult函数
/* 功能：绘制结算界面：
 *   - 背景图 + 文案框（胜/负不同配色与文字）；
 *   - 底部按钮：胜利且非最后一关→“下一关”，否则→“返回主菜单”。
 * 参数：const Game* g
 * 返回值：void */
void DrawResult(const Game* g);

//view层需要提供一个Settings_update函数
/* 功能：设置界面输入处理：
 *   - “+” 按钮：volume+=10（上限100），并立即 SetBGMVolume；
 *   - “-” 按钮：volume-=10（下限0），并立即 SetBGMVolume；
 *   - “返回主菜单” → STATE_MENU。
 * 参数：Game* g；ExMessage* msg
 * 返回值：GameState 下一状态 */
GameState Settings_update(Game* g, ExMessage* msg);

//view层需要提供一个DrawSettings函数
/* 功能：绘制设置界面：清屏 + 背景图 + 音量加减按钮 + 返回按钮，
 *      鼠标悬停高亮。
 * 参数：const Game* g
 * 返回值：void */
void DrawSettings(const Game* g);

//view层需要提供一个EndlessView_update函数
/* 功能：无尽模式输入处理：
 *   - 键盘：空格 → STATE_PAUSED；ESC → 清 isEndlessMode 后 STATE_MENU；
 *   - 左键：暂停按钮 / 塔按钮 / 地图建塔；
 *   - 右键：拆塔。
 * 参数：Game* g；ExMessage* msg
 * 返回值：GameState 通常为 STATE_ENDLESS */
GameState EndlessView_update(Game* g, ExMessage* msg);

//view层需要提供一个DrawEndless函数
/* 功能：绘制无尽模式界面：与 GameRender 类似，但 UI 文字改成
 *      “金币/波数/最高”，基地血条位置略有调整；塔按钮同普通关卡。
 * 参数：const Game* g
 * 返回值：void */
void DrawEndless(const Game* g);

//view层需要提供一个Gallery_update函数
/* 功能：图鉴主界面输入处理：
 *   点击“友方图鉴” → STATE_GALLERY_FRIEND；
 *   点击“敌方图鉴” → STATE_GALLERY_ENEMY；
 *   点击“返回主菜单” → STATE_MENU。
 * 参数：Game* g；ExMessage* msg
 * 返回值：GameState 下一状态 */
GameState Gallery_update(Game* g, ExMessage* msg);

//view层需要提供一个DrawGallery函数
/* 功能：绘制图鉴主界面：背景图 + 三个按钮（悬停高亮）。
 * 参数：const Game* g
 * 返回值：void */
void DrawGallery(const Game* g);

//view层需要提供一个GalleryFriend_update函数
/* 功能：友方图鉴输入处理：
 *   点击日向/智乃/康娜 → STATE_HINATA / STATE_CHINO / STATE_KANNA；
 *   点击返回 → STATE_GALLERY。
 * 参数：Game* g；ExMessage* msg
 * 返回值：GameState 下一状态 */
GameState GalleryFriend_update(Game* g, ExMessage* msg);

//view层需要提供一个DrawGalleryFriend函数
/* 功能：绘制友方图鉴：背景图 + 返回按钮 + 三个角色按钮。
 * 参数：const Game* g
 * 返回值：void */
void DrawGalleryFriend(const Game* g);

//view层需要提供一个GalleryEnemy_update函数
/* 功能：敌方图鉴输入处理：
 *   点击 Yh/Aw/Dd/Boss → STATE_YH / STATE_AW / STATE_DD / STATE_01；
 *   点击返回 → STATE_GALLERY。
 * 参数：Game* g；ExMessage* msg
 * 返回值：GameState 下一状态 */
GameState GalleryEnemy_update(Game* g, ExMessage* msg);

//view层需要提供一个DrawGalleryEnemy函数
/* 功能：绘制敌方图鉴：背景图 + 返回按钮 + 4 个敌人按钮。
 * 参数：const Game* g
 * 返回值：void */
void DrawGalleryEnemy(const Game* g);

//图鉴角色详细信息子界面:
/* 功能：友方/敌方具体角色详情页（点击返回回到对应列表界面）。
 * 参数：Game* g；ExMessage* msg
 * 返回值：GameState 下一状态 */
GameState GalleryHinata_update(Game* g, ExMessage* msg);
void DrawGalleryHinata(const Game* g);

GameState GalleryChino_update(Game* g, ExMessage* msg);
void DrawGalleryChino(const Game* g);

GameState GalleryKanna_update(Game* g, ExMessage* msg);
void DrawGalleryKanna(const Game* g);

GameState GalleryYh_update(Game* g, ExMessage* msg);
void DrawGalleryYh(const Game* g);

GameState GalleryAw_update(Game* g, ExMessage* msg);
void DrawGalleryAw(const Game* g);

GameState GalleryDd_update(Game* g, ExMessage* msg);
void DrawGalleryDd(const Game* g);

GameState Gallery01_update(Game* g, ExMessage* msg);
void DrawGallery01(const Game* g);

//view层需要提供一个DrawUI函数
/* 功能：绘制游戏内 UI：
 *   1. 右上角信息栏：金币 / 波次(当前+1/总) / 剩余敌人 / 关卡(currentMap+1)；
 *   2. 基地下方血条（按 hp/maxHp 比例填充，>50% 绿 / 否则红）。
 * 参数：const Game* g
 * 返回值：void */
void DrawUI(const Game* g);

//新增绘制圆角函数
void GradientRoundRectV(int left, int top, int right, int bottom,
    int radius, COLORREF colorTop, COLORREF colorBottom);
void DrawRoundRectBorder(int left, int top, int right, int bottom,
    int radius, COLORREF borderColor, int lineWidth);

// ==================== main游戏循环逻辑 ====================
// 1. 初始化：InitData + 设置初始状态 + initgraph + InitAssets + 双缓冲。
// 2. 每帧：
//    a. 按 state 把输入分发给对应 update；
//    b. STATE_PLAYING / STATE_ENDLESS 时跑 service 更新
//       （Spawn → Move → Status → TowerAttack → BaseHit → WinLose）；
//    c. 按 state 调对应 Draw 渲染；
//    d. FlushBatchDraw + Sleep(1)。
int main(void) {
    InitData(&game);

    game.state = STATE_MENU;
    UpdateBGM(&game);
    initgraph(WINDOW_W, WINDOW_H);
    InitAssets();
    BeginBatchDraw();

    ULONGLONG lastTime = GetTickCount64();

    while (game.state != STATE_EXIT) {
        UpdateBGM(&game);

        ULONGLONG now = GetTickCount64();
        float dt = (now - lastTime) / 1000.0f;
        lastTime = now;

        // ---------- a. 消息：按状态分发给对应界面的 update ----------
        ExMessage msg;
        while (peekmessage(&msg)) {

            // 实时记录鼠标位置
            if (msg.message == WM_MOUSEMOVE) {
                game.mouseX = msg.x;
                game.mouseY = msg.y;
            }

            switch (game.state) {
            case STATE_MENU:
                game.state = Menu_update(&game, &msg);
                break;
            case STATE_LEVEL_SELECT:
                game.state = LevelSelect_update(&game, &msg);
                break;
            case STATE_PLAYING:
                game.state = GameView_update(&game, &msg);
                break;
            case STATE_PAUSED:
                game.state = Pause_update(&game, &msg);
                break;
            case STATE_SETTINGS:
                game.state = Settings_update(&game, &msg);
                break;
            case STATE_TEAM:
                game.state = Team_update(&game, &msg);
                break;
            case STATE_STORY:
                game.state = Story_update(&game, &msg);
                break;
            case STATE_RESULT:
                game.state = Result_update(&game, &msg);
                break;
            case STATE_ENDLESS:
                game.state = EndlessView_update(&game, &msg);
                break;
                // ===== 图鉴相关 =====
            case STATE_GALLERY:
                game.state = Gallery_update(&game, &msg);
                break;
            case STATE_GALLERY_FRIEND:
                game.state = GalleryFriend_update(&game, &msg);
                break;
            case STATE_GALLERY_ENEMY:
                game.state = GalleryEnemy_update(&game, &msg);
                break;
            case STATE_HINATA:
                game.state = GalleryHinata_update(&game, &msg);
                break;
            case STATE_CHINO:
                game.state = GalleryChino_update(&game, &msg);
                break;
            case STATE_KANNA:
                game.state = GalleryKanna_update(&game, &msg);
                break;
            case STATE_YH:
                game.state = GalleryYh_update(&game, &msg);
                break;
            case STATE_AW:
                game.state = GalleryAw_update(&game, &msg);
                break;
            case STATE_DD:
                game.state = GalleryDd_update(&game, &msg);
                break;
            case STATE_01:
                game.state = Gallery01_update(&game, &msg);
                break;
                // =================
            default:
                break;
            }
        }

        // ---------- b. 每帧逻辑：仅游戏状态执行 service 更新 ----------
        // 每帧顺序固定：
        // 刷怪 → 敌人移动 → 状态更新 → 塔攻击 → 基地碰撞 → 胜负判定
        if (game.state == STATE_PLAYING || game.state == STATE_ENDLESS) {

            SpawnUpdate(&game, dt);          // 按波次定时生成敌人
            EnemyMove(&game, dt);            // 敌人沿路径移动
            UpdateEnemyStatus(&game, dt);    // 减速等状态计时递减
            TowerAttackUpdate(&game, dt);    // 塔按冷却自动攻击范围内所有敌人
            BaseHitCheck(&game);             // 敌人碰到基地扣血并消失

            // 基地闪红计时递减
            if (game.baseFlashTimer > 0.0f) {
                game.baseFlashTimer -= dt;
                if (game.baseFlashTimer < 0.0f) game.baseFlashTimer = 0.0f;
            }

            // ========== 无尽模式：只判失败 ==========
            if (game.state == STATE_ENDLESS ) {
                if (game.base.hp <= 0) {
                    // 更新历史最高波数
                    if (game.endlessWave > game.endlessBestWave) {
                        game.endlessBestWave = game.endlessWave;
                    }
                    SaveGame(&game);
                    game.state = STATE_RESULT;
                }
            }
            // ========== 普通关卡：走 CheckWinLose ==========
            else {
                GameState result = CheckWinLose(&game);
                if (result != STATE_PLAYING && game.base.hp > 0) {
                    game.state = STATE_STORY;
                    pageNow = 0;
                } 
                else if(result != STATE_PLAYING){
                    game.state = STATE_RESULT; 
                }
            }
        }

        // ---------- c. 绘制：按当前状态渲染本帧 ----------
        switch (game.state) {
        case STATE_MENU:
            DrawMenu(&game);
            break;
        case STATE_LEVEL_SELECT:
            DrawLevelSelect(&game);
            break;
        case STATE_PLAYING:
            GameRender(&game);
            break;
        case STATE_PAUSED:
            DrawPause(&game);
            break;
        case STATE_SETTINGS:
            DrawSettings(&game);
            break;
        case STATE_TEAM:
            DrawTeam(&game);
            break;
        case STATE_STORY:
            DrawStory(&game);
            break;
        case STATE_RESULT:
            DrawResult(&game);
            break;
        case STATE_ENDLESS:
            DrawEndless(&game);
            break;
            // ===== 图鉴相关 =====
        case STATE_GALLERY:
            DrawGallery(&game);
            break;
        case STATE_GALLERY_FRIEND:
            DrawGalleryFriend(&game);
            break;
        case STATE_GALLERY_ENEMY:
            DrawGalleryEnemy(&game);
            break;
        case STATE_HINATA:
            DrawGalleryHinata(&game);
            break;
        case STATE_CHINO:
            DrawGalleryChino(&game);
            break;
        case STATE_KANNA:
            DrawGalleryKanna(&game);
            break;
        case STATE_YH:
            DrawGalleryYh(&game);
            break;
        case STATE_AW:
            DrawGalleryAw(&game);
            break;
        case STATE_DD:
            DrawGalleryDd(&game);
            break;
        case STATE_01:
            DrawGallery01(&game);
            break;
            // =================
        default:
            break;
        }

        FlushBatchDraw();
        Sleep(1);
    }

    EndBatchDraw();
    closegraph();
    return 0;
}


/********** 函数实现 **********/

// ==================== service 实现区 ====================

/* 功能：判断某座塔在当前模式下是否解锁。 */
int IsTowerUnlocked(const Game* g, int towerType) {
    if (g->isEndlessMode) return 1;   // 无尽模式全解锁
    return g->currentMap >= g->towerConfigs[towerType].unlockLevel;
}

/* 功能：一次性从硬盘加载所有图片资源到内存。 */
void InitAssets(void) {
    loadimage(&im_menuBg, L"image\\UI\\Mainmenu_clean.png", WINDOW_W, WINDOW_H);
    loadimage(&im_level_selectBg, L"image\\UI\\Select_clean.png", WINDOW_W, WINDOW_H);
    loadimage(&im_mapBg, L"image\\UI\\Map.png", WINDOW_W, WINDOW_H);
    loadimage(&im_pauseBg, L"image\\UI\\Pause_clean.png", WINDOW_W, WINDOW_H);
    loadimage(&im_setBg, L"image\\UI\\Setting_clean.png", WINDOW_W, WINDOW_H);
    loadimage(&Detail, L"image\\UI\\Detail.png", WINDOW_W, WINDOW_H);
    loadimage(&im_resultBg, L"image\\UI\\Results_clean.png", WINDOW_W, WINDOW_H);
    // 图鉴
    loadimage(&im_galBg, L"image\\UI\\Gallery.png", WINDOW_W, WINDOW_H);
    loadimage(&im_Friend, L"image\\UI\\Gallery_F.png", WINDOW_W, WINDOW_H);
    loadimage(&im_Enemy, L"image\\UI\\Gallery_E.png", WINDOW_W, WINDOW_H);
    loadimage(&im_Hinata, L"image\\UI\\Hinata.png", WINDOW_W, WINDOW_H);
    loadimage(&im_Chino, L"image\\UI\\Chino.png", WINDOW_W, WINDOW_H);
    loadimage(&im_Kanna, L"image\\UI\\Kanna.png", WINDOW_W, WINDOW_H);
    loadimage(&im_Yh, L"image\\UI\\Yh.png", WINDOW_W, WINDOW_H);
    loadimage(&im_Aw, L"image\\UI\\Aw.png", WINDOW_W, WINDOW_H);
    loadimage(&im_Dd, L"image\\UI\\Dd.png", WINDOW_W, WINDOW_H);
    loadimage(&im_01, L"image\\UI\\01.png", WINDOW_W, WINDOW_H);

    loadimage(&Show_info, L"image\\UI\\Show_info.png", 130, 200);
    loadimage(&Bt_pause, L"image\\UI\\Bt_pause.png", 100, 100);
    loadimage(&Bt_Hinata, L"image\\UI\\Bt_Hinata.png", 130, 100);
    loadimage(&Bt_Chino, L"image\\UI\\Bt_Chino.png", 130, 100);
    loadimage(&Bt_Kanna, L"image\\UI\\Bt_Kanna.png", 130, 100);

    loadimage(&im_towerHinata_color, L"image\\Hinata\\Hinata_idle_img.png", 53, 53);
    loadimage(&im_towerHinata_atk_color, L"image\\Hinata\\Hinata_atk_img.png", 53, 53);
    loadimage(&im_towerHinata_mask, L"image\\Hinata\\Hinata_idle_mask.png", 53, 53);
    loadimage(&im_towerHinata_atk_mask, L"image\\Hinata\\Hinata_atk_mask.png", 53, 53);
    loadimage(&im_towerChino_color, L"image\\Chino\\Chino_idle_img.png", 53, 53);
    loadimage(&im_towerChino_atk_color, L"image\\Chino\\Chino_atk_img.png", 53, 53);
    loadimage(&im_towerChino_mask, L"image\\chino\\Chino_idle_mask.png", 53, 53);
    loadimage(&im_towerChino_atk_mask, L"image\\chino\\Chino_atk_mask.png", 53, 53);
    loadimage(&im_towerKanna_color, L"image\\Kanna\\Kanna_idle_img.png", 53, 53);
    loadimage(&im_towerKanna_atk_color, L"image\\Kanna\\Kanna_atk_img.png", 53, 53);
    loadimage(&im_towerKanna_mask, L"image\\Kanna\\Kanna_idle_mask.png", 53, 53);
    loadimage(&im_towerKanna_atk_mask, L"image\\Kanna\\Kanna_atk_mask.png", 53, 53);

    loadimage(&im_enemyYh_color, L"image\\Enemy\\Yh_img.png", 50, 50);
    loadimage(&im_enemyYh_mask, L"image\\Enemy\\Yh_mask.png", 50, 50);
    loadimage(&im_enemyAw_color, L"image\\Enemy\\Aw_img.png", 50, 50);
    loadimage(&im_enemyAw_mask, L"image\\Enemy\\Aw_mask.png", 50, 50);
    loadimage(&im_enemyDd_color, L"image\\Enemy\\Dd_img.png", 50, 50);
    loadimage(&im_enemyDd_mask, L"image\\Enemy\\Dd_mask.png", 50, 50);
    loadimage(&im_boss_color, L"image\\Enemy\\Boss_img.png", 150, 150);
    loadimage(&im_boss_mask, L"image\\Enemy\\Boss_mask.png", 150, 150);

    //以下循环代表加载剧情图
    int i = 0;
    for (i = 0;i < Count_1a;i++)
    {
        wchar_t A1[100];
        swprintf(A1, 100, L"image\\Story\\1a\\%d.png", i + 1);
        loadimage(&a1[i], A1);
    }
    for (i = 0;i < Count_1b;i++)
    {
        wchar_t B1[100];
        swprintf(B1, 100, L"image\\Story\\1b\\%d.png", i + 1);
        loadimage(&b1[i], B1);
    }

    for (i = 0;i < Count_2a;i++)
    {
        wchar_t A2[100];
        swprintf(A2, 100, L"image\\Story\\2a\\%d.png", i + 1);
        loadimage(&a2[i], A2);
    }
    for (i = 0;i < Count_2b;i++)
    {
        wchar_t B2[100];
        swprintf(B2, 100, L"image\\Story\\2b\\%d.png", i + 1);
        loadimage(&b2[i], B2);
    }

    for (i = 0;i < Count_3a;i++)
    {
        wchar_t A3[100];
        swprintf(A3, 100, L"image\\Story\\3a\\%d.png", i + 1);
        loadimage(&a3[i], A3);
    }
    for (i = 0;i < Count_3b;i++)
    {
        wchar_t B3[100];
        swprintf(B3, 100, L"image\\Story\\3b\\%d.png", i + 1);
        loadimage(&b3[i], B3);
    }

    for (i = 0;i < Count_4a;i++)
    {
        wchar_t A4[100];
        swprintf(A4, 100, L"image\\Story\\4a\\%d.png", i + 1);
        loadimage(&a4[i], A4);
    }
    for (i = 0;i < Count_4b;i++)
    {
        wchar_t B4[100];
        swprintf(B4, 100, L"image\\Story\\4b\\%d.png", i + 1);
        loadimage(&b4[i], B4);
    }

    for (i = 0;i < Count_5a;i++)
    {
        wchar_t A5[100];
        swprintf(A5, 100, L"image\\Story\\5a\\%d.png", i + 1);
        loadimage(&a5[i], A5);
    }
    for (i = 0;i < Count_5b;i++)
    {
        wchar_t B5[100];
        swprintf(B5, 100, L"image\\Story\\5b\\%d.png", i + 1);
        loadimage(&b5[i], B5);
    }





}

//============================================================================
/* kMapCell：地图格子模板（0=空地，2=可建造高台）。 */
static const unsigned char kMapCell[MAP_H][MAP_W] = {
    /* 列 x =  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 */
    { 0, 0, 0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0 },   /* y= 0 */
    { 0, 0, 0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0 },   /* y= 1 */
    { 0, 0, 0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0 },   /* y= 2 */
    { 0, 0, 0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 2 },   /* y= 3 */
    { 0, 0, 0, 0, 2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 2, 0, 0 },   /* y= 4 */
    { 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0 },   /* y= 5 */
    { 0, 0, 0, 0, 2, 2, 2, 2, 2, 0, 2, 0, 2, 2, 2, 2, 2, 2, 2, 0, 0 },   /* y= 6 */
    { 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0 },   /* y= 7 */
    { 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0 },   /* y= 8 */
    { 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0 },   /* y= 9 */
    { 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0 },   /* y=10 */
    { 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0 },   /* y=11 */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0 },   /* y=12 */
};

/* 功能：初始化地图与路径。
 *   1. 清空 mapData / paths。
 *   2. 手写 5 关的路径节点。
 *   3. 用 kMapCell 填充地图；路径格子标为 1。
 */
void InitMaps(Game* g) {
    memset(g->mapData, 0, sizeof(g->mapData));
    memset(g->paths, 0, sizeof(g->paths));

    //========第一关========
    g->paths[0][0].count = 4;
    g->paths[0][0].nodes[0].x = 17; g->paths[0][0].nodes[0].y = 0;
    g->paths[0][0].nodes[1].x = 17; g->paths[0][0].nodes[1].y = 5;
    g->paths[0][0].nodes[2].x = 11; g->paths[0][0].nodes[2].y = 5;
    g->paths[0][0].nodes[3].x = 11; g->paths[0][0].nodes[3].y = 12;
    g->paths[0][1].count = 0;

    //=========第二关=======
    g->paths[1][0].count = 4;
    g->paths[1][0].nodes[0].x = 17; g->paths[1][0].nodes[0].y = 0;
    g->paths[1][0].nodes[1].x = 17; g->paths[1][0].nodes[1].y = 5;
    g->paths[1][0].nodes[2].x = 11; g->paths[1][0].nodes[2].y = 5;
    g->paths[1][0].nodes[3].x = 11; g->paths[1][0].nodes[3].y = 12;
    g->paths[1][1].count = 0;

    //=========第三关======
    g->paths[2][0].count = 4;
    g->paths[2][0].nodes[0].x = 17; g->paths[2][0].nodes[0].y = 0;
    g->paths[2][0].nodes[1].x = 17; g->paths[2][0].nodes[1].y = 5;
    g->paths[2][0].nodes[2].x = 11; g->paths[2][0].nodes[2].y = 5;
    g->paths[2][0].nodes[3].x = 11; g->paths[2][0].nodes[3].y = 12;
    g->paths[2][1].count = 4;
    g->paths[2][1].nodes[0].x = 5;  g->paths[2][1].nodes[0].y = 0;
    g->paths[2][1].nodes[1].x = 5;  g->paths[2][1].nodes[1].y = 5;
    g->paths[2][1].nodes[2].x = 9;  g->paths[2][1].nodes[2].y = 5;
    g->paths[2][1].nodes[3].x = 9;  g->paths[2][1].nodes[3].y = 12;

    //=========第四关======
    g->paths[3][0].count = 4;
    g->paths[3][0].nodes[0].x = 17; g->paths[3][0].nodes[0].y = 0;
    g->paths[3][0].nodes[1].x = 17; g->paths[3][0].nodes[1].y = 5;
    g->paths[3][0].nodes[2].x = 11; g->paths[3][0].nodes[2].y = 5;
    g->paths[3][0].nodes[3].x = 11; g->paths[3][0].nodes[3].y = 12;
    g->paths[3][1].count = 4;
    g->paths[3][1].nodes[0].x = 5;  g->paths[3][1].nodes[0].y = 0;
    g->paths[3][1].nodes[1].x = 5;  g->paths[3][1].nodes[1].y = 5;
    g->paths[3][1].nodes[2].x = 9;  g->paths[3][1].nodes[2].y = 5;
    g->paths[3][1].nodes[3].x = 9;  g->paths[3][1].nodes[3].y = 12;

    //=========第五关======
    g->paths[4][0].count = 4;
    g->paths[4][0].nodes[0].x = 17; g->paths[4][0].nodes[0].y = 0;
    g->paths[4][0].nodes[1].x = 17; g->paths[4][0].nodes[1].y = 5;
    g->paths[4][0].nodes[2].x = 11; g->paths[4][0].nodes[2].y = 5;
    g->paths[4][0].nodes[3].x = 11; g->paths[4][0].nodes[3].y = 12;
    g->paths[4][1].count = 4;
    g->paths[4][1].nodes[0].x = 5;  g->paths[4][1].nodes[0].y = 0;
    g->paths[4][1].nodes[1].x = 5;  g->paths[4][1].nodes[1].y = 5;
    g->paths[4][1].nodes[2].x = 9;  g->paths[4][1].nodes[2].y = 5;
    g->paths[4][1].nodes[3].x = 9;  g->paths[4][1].nodes[3].y = 12;

    //=========设置地图格子===========
    /*
    0：空地/装饰/不可建造
    1：路径（不能建塔）
    2：高台（可建塔）
    */
    for (int lv = 0; lv < MAP_COUNT; lv++) {
        for (int y = 0; y < MAP_H; y++) {
            for (int x = 0; x < MAP_W; x++) {
                g->mapData[lv][y][x] = kMapCell[y][x];
            }
        }
        for (int pid = 0; pid < PATH_COUNT; pid++) {
            Path* path = &g->paths[lv][pid];
            if (path->count < 2) continue;
            for (int i = 0; i < path->count - 1; i++) {
                int x1 = path->nodes[i].x, y1 = path->nodes[i].y;
                int x2 = path->nodes[i + 1].x, y2 = path->nodes[i + 1].y;
                int stepX = (x2 > x1) ? 1 : (x2 < x1 ? -1 : 0);
                int stepY = (y2 > y1) ? 1 : (y2 < y1 ? -1 : 0);
                int cx = x1, cy = y1;
                while (1) {
                    if (cx >= 0 && cx < MAP_W && cy >= 0 && cy < MAP_H) {
                        g->mapData[lv][cy][cx] = 1;
                    }
                    if (cx == x2 && cy == y2) break;
                    if (cx != x2) cx += stepX;
                    if (cy != y2) cy += stepY;
                }
            }
        }
    }
}

/* 功能：设置当前 BGM 的音量（0~100）。 */
void SetBGMVolume(int volume)
{
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    int mciVolume = volume * 10;
    wchar_t cmd[64];
    swprintf(cmd, 64, L"setaudio nowplaying volume to %d", mciVolume);
    mciSendString(cmd, NULL, 0, NULL);
}

/* 功能：初始化塔 / 敌人配置表。 */
void InitConfigs(Game* g) {
    // ================= 防御塔配置 =================
    // Hinata（日向）：基础输出，便宜好用
    g->towerConfigs[Hinata].cost = 50;
    g->towerConfigs[Hinata].atk = 20;
    g->towerConfigs[Hinata].rangeGrid = 5;
    g->towerConfigs[Hinata].attackInterval = 1.0f;
    g->towerConfigs[Hinata].unlockLevel = 0;
    g->towerConfigs[Hinata].special = SINGLE;

    // Chino（智乃）：有减速，专克高速敌人
    g->towerConfigs[Chino].cost = 75;
    g->towerConfigs[Chino].atk = 20;
    g->towerConfigs[Chino].rangeGrid = 8;
    g->towerConfigs[Chino].attackInterval = 1.0f;
    g->towerConfigs[Chino].unlockLevel = 1;
    g->towerConfigs[Chino].special = SINGLE;

    // Kanna（康娜）：单次伤害高但 CD 长，群攻
    g->towerConfigs[Kanna].cost = 120;
    g->towerConfigs[Kanna].atk = 40;
    g->towerConfigs[Kanna].rangeGrid = 6;
    g->towerConfigs[Kanna].attackInterval = 1.5f;
    g->towerConfigs[Kanna].unlockLevel = 3;
    g->towerConfigs[Kanna].special = AOE;

    // ================= 敌人配置 =================
    // 类型 0：Yh（普通）
    g->enemyConfigs[0].hp = 150;
    g->enemyConfigs[0].speed = 1.0f * TILE_SIZE_X;
    g->enemyConfigs[0].damage = 2;
    g->enemyConfigs[0].reward = 20;

    // 类型 1：Aw（高速）
    g->enemyConfigs[1].hp = 80;
    g->enemyConfigs[1].speed = 2.0f * TILE_SIZE_X;
    g->enemyConfigs[1].damage = 2;
    g->enemyConfigs[1].reward = 15;

    // 类型 2：Dd（肉盾）
    g->enemyConfigs[2].hp = 5000;
    g->enemyConfigs[2].speed = 0.5f * TILE_SIZE_X;
    g->enemyConfigs[2].damage = 5;
    g->enemyConfigs[2].reward = 25;

    // 类型 3：BOSS
    g->enemyConfigs[3].hp = 10000;
    g->enemyConfigs[3].speed = 0.0f * TILE_SIZE_X;
    g->enemyConfigs[3].damage = 10000;
    g->enemyConfigs[3].reward = 0;
}

/* 功能：初始化 5 个关卡的波次布局。 */
void InitWaveConfigs(Game* g) {
    // 每组格式：{ 敌人类型, 数量, 路径 }
    // 参数顺序：{{groups...}, groupCount, spawnInterval, waveDelay, isBossWave}

    // 第 1 关 — 教学关：纯 Yh，5 波，3/5/7/8/10，波次间隔 8 秒
    g->levelWaves[0][0] = WaveConfig{ {{0,  3, 0}}, 1, 0.8f, 8.0f, 0 };
    g->levelWaves[0][1] = WaveConfig{ {{0,  5, 0}}, 1, 0.8f, 8.0f, 0 };
    g->levelWaves[0][2] = WaveConfig{ {{0,  7, 0}}, 1, 0.8f, 8.0f, 0 };
    g->levelWaves[0][3] = WaveConfig{ {{0,  8, 0}}, 1, 0.8f, 8.0f, 0 };
    g->levelWaves[0][4] = WaveConfig{ {{0, 10, 0}}, 1, 0.8f, 8.0f, 0 };

    // 第 2 关 — 引入 Aw（第 3 波起），6 波，单路径，间隔 7 秒
    g->levelWaves[1][0] = WaveConfig{ {{0, 3, 0}},                    1, 0.8f, 7.0f, 0 };
    g->levelWaves[1][1] = WaveConfig{ {{0, 5, 0}},                    1, 0.8f, 7.0f, 0 };
    g->levelWaves[1][2] = WaveConfig{ {{0, 5, 0}, {1, 2, 0}},         2, 0.8f, 7.0f, 0 };
    g->levelWaves[1][3] = WaveConfig{ {{0, 6, 0}, {1, 3, 0}},         2, 0.8f, 7.0f, 0 };
    g->levelWaves[1][4] = WaveConfig{ {{0, 7, 0}, {1, 4, 0}},         2, 0.8f, 7.0f, 0 };
    g->levelWaves[1][5] = WaveConfig{ {{0, 8, 0}, {1, 5, 0}},         2, 0.8f, 7.0f, 0 };

    // 第 3 关 — 双路径，7 波，间隔 6 秒
    g->levelWaves[2][0] = WaveConfig{ {{0, 3, 0}, {0, 3, 1}},                        2, 0.8f, 6.0f, 0 };
    g->levelWaves[2][1] = WaveConfig{ {{0, 4, 0}, {0, 4, 1}},                        2, 0.8f, 6.0f, 0 };
    g->levelWaves[2][2] = WaveConfig{ {{0, 3, 0}, {0, 3, 1}, {1, 2, 0}, {1, 2, 1}},  4, 0.8f, 6.0f, 0 };
    g->levelWaves[2][3] = WaveConfig{ {{0, 4, 0}, {0, 4, 1}, {1, 3, 0}, {1, 3, 1}},  4, 0.8f, 6.0f, 0 };
    g->levelWaves[2][4] = WaveConfig{ {{0, 5, 0}, {0, 5, 1}, {1, 3, 0}, {1, 4, 1}},  4, 0.8f, 6.0f, 0 };
    g->levelWaves[2][5] = WaveConfig{ {{0, 6, 0}, {0, 6, 1}, {1, 4, 0}, {1, 4, 1}},  4, 0.8f, 6.0f, 0 };
    g->levelWaves[2][6] = WaveConfig{ {{0, 6, 0}, {0, 6, 1}, {1, 5, 0}, {1, 5, 1}},  4, 0.8f, 6.0f, 0 };

    // 第 4 关 — 加入 Dd（第 4 波起），8 波，双路径，间隔 6 秒
    g->levelWaves[3][0] = WaveConfig{ {{0, 3, 0}, {0, 3, 1}},                                                2, 0.8f, 6.0f, 0 };
    g->levelWaves[3][1] = WaveConfig{ {{0, 4, 0}, {0, 4, 1}},                                                2, 0.8f, 6.0f, 0 };
    g->levelWaves[3][2] = WaveConfig{ {{0, 3, 0}, {0, 3, 1}, {1, 2, 0}, {1, 2, 1}},                          4, 0.8f, 6.0f, 0 };
    g->levelWaves[3][3] = WaveConfig{ {{0, 3, 0}, {0, 3, 1}, {1, 2, 0}, {1, 2, 1}, {2, 1, 0}, {2, 1, 1}},   6, 0.8f, 6.0f, 0 };
    g->levelWaves[3][4] = WaveConfig{ {{0, 4, 0}, {0, 4, 1}, {1, 3, 0}, {1, 3, 1}, {2, 1, 0}, {2, 1, 1}},   6, 0.8f, 6.0f, 0 };
    g->levelWaves[3][5] = WaveConfig{ {{0, 4, 0}, {0, 4, 1}, {1, 3, 0}, {1, 3, 1}, {2, 1, 0}, {2, 1, 1}},   6, 0.8f, 6.0f, 0 };
    g->levelWaves[3][6] = WaveConfig{ {{0, 5, 0}, {0, 5, 1}, {1, 4, 0}, {1, 4, 1}, {2, 1, 0}, {2, 1, 1}},   6, 0.8f, 6.0f, 0 };
    g->levelWaves[3][7] = WaveConfig{ {{0, 6, 0}, {0, 6, 1}, {1, 5, 0}, {1, 5, 1}, {2, 1, 0}, {2, 1, 1}},   6, 0.8f, 6.0f, 0 };

    // 第 5 关 — BOSS 关：4 波小怪，波间 5 秒
    // BOSS 不通过波次生成，由 LoadLevel 直接生成并静止在上方中央
    // 每清空一波小怪，BOSS 扣 25% 血（在 KillEnemy 中处理）
    g->levelWaves[4][0] = WaveConfig{ {{0, 4, 0}, {0, 4, 1}},                                             2, 0.8f, 5.0f, 1 };
    g->levelWaves[4][1] = WaveConfig{ {{0, 3, 0}, {0, 3, 1}, {1, 3, 0}, {1, 3, 1}},                       4, 0.8f, 5.0f, 1 };
    g->levelWaves[4][2] = WaveConfig{ {{1, 4, 0}, {1, 4, 1}, {2, 1, 0}, {2, 1, 1}},                       4, 0.8f, 5.0f, 1 };
    g->levelWaves[4][3] = WaveConfig{ {{0, 3, 0}, {0, 2, 1}, {1, 3, 0}, {1, 2, 1}, {2, 2, 0}, {2, 1, 1}}, 6, 0.8f, 5.0f, 1 };
}

/* 功能：程序启动的统一初始化入口。
 * 注意：unlockedCount 由 InitData 默认给 1，或由 LoadGame 覆盖；
 *      目前调试用直接给 5。取消 LoadGame 的注释即可读档。
 */
void InitData(Game* g) {
    memset(g, 0, sizeof(Game));

    // ========== 全局静态默认值 ==========
    // g->unlockedCount = 1;   // 正式版
    g->unlockedCount = 5;      // 调试用：解锁全部 5 关
    g->highScore = 0;
    g->endlessBestWave = 0;
    g->soundOn = 1;
    g->volume = 50;
    g->state = STATE_MENU;
    g->currentMap = 0;
    g->selectedTowerType = -1;
    g->storyPhase = 0;

    g->endlessWave = 0;
    g->endlessBestWave = 0;

    // ========== 载入静态配置表 ==========
    InitConfigs(g);
    InitWaveConfigs(g);
    InitMaps(g);

    // ========== 清空单局数据 ==========
    ResetRound(g);

    // ========== 读档（可选） ==========
    LoadGame(g);   // 开发者模式
}

/* 功能：重置单局数据（不清空持久数据）。 */
void ResetRound(Game* g) {
    g->base.hp = 0;
    g->base.maxHp = 0;
    g->base.x = 0;
    g->base.y = 0;
    g->base.radius = 0;

    g->money = 0;
    g->totalWaves = 0;
    g->waveIndex = 0;
    g->enemiesRemaining = 0;
    g->spawnTimer = 0.0f;

    memset(g->enemies, 0, sizeof(g->enemies));
    g->enemyCount = 0;

    memset(g->towers, 0, sizeof(g->towers));
    g->towerCount = 0;

    g->baseFlashTimer = 0.0f;
    g->hoverGridX = -1;
    g->hoverGridY = -1;
    g->selectedTowerType = -1;
    g->storyPhase = 0;

    g->groupIndex = 0;
    g->currentGroupSpawnedCount = 0;
    g->isEndlessMode = 0;
}

/* 功能：载入指定关卡。 */
void LoadLevel(Game* g, int level) {
    if (g == NULL) return;
    if (level < 0 || level >= MAP_COUNT) return;

    ResetRound(g);
    pageNow = 0;   /***************修改：进入关卡(播放开始剧情)前重置翻页进度*****************/
    g->currentMap = level;
    if (level + 1 > g->unlockedCount) {
        g->unlockedCount = level + 1;
    }

    g->base.hp = INIT_BASE_HP;
    g->base.maxHp = INIT_BASE_HP;
    g->base.radius = 30;

    // 基地位置 = 该关所有路径终点的平均格中心
    {
        int sumX = 0, sumY = 0, cnt = 0;
        for (int pid = 0; pid < PATH_COUNT; pid++) {
            Path* p = &g->paths[level][pid];
            if (p->count > 0) {
                Point end = p->nodes[p->count - 1];
                sumX += end.x;
                sumY += end.y;
                cnt++;
            }
        }
        if (cnt > 0) {
            int baseGx = sumX / cnt;
            int baseGy = sumY / cnt;
            if (g->currentMap < 2) {
                g->base.x = SCREEN_MARGIN_X + baseGx * TILE_SIZE_X - TILE_SIZE_X / 2;
            }
            else {
                g->base.x = SCREEN_MARGIN_X + baseGx * TILE_SIZE_X + TILE_SIZE_X / 2;
            }
            g->base.y = SCREEN_MARGIN_Y + baseGy * TILE_SIZE_Y + TILE_SIZE_Y / 2;
        }
        else {
            g->base.x = 600;
            g->base.y = 600;
        }
    }

    if (level <= 1)      g->money = 150;
    else if (level <= 3) g->money = 200;
    else                 g->money = 250;

    // 统计有效波次
    int totalWaves = 0;
    for (int i = 0; i < MAX_WAVES; i++) {
        if (g->levelWaves[level][i].groupCount <= 0) break;
        totalWaves++;
    }
    g->totalWaves = totalWaves;

    g->waveIndex = 0;
    g->spawnTimer = 0.0f;
    g->groupIndex = 0;
    g->currentGroupSpawnedCount = 0;

    // 第 1 波总怪数
    g->enemiesRemaining = 0;
    if (totalWaves > 0) {
        WaveConfig* first = &g->levelWaves[level][0];
        for (int i = 0; i < first->groupCount; i++) {
            g->enemiesRemaining += first->groups[i].count;
        }
    }

    // 第 5 关：开局直接生成 BOSS
    if (level == 4) {
        SpawnOneEnemy(g, 3, 0);
        if (g->enemyCount > 0) {
            Enemy* boss = &g->enemies[g->enemyCount - 1];
            boss->isBoss = 1;
            boss->speed = 0.0f;
            boss->x = 600;
            boss->y = 100;
        }
    }
}

/* 功能：进入无尽模式。 */
void LoadEndless(Game* g) {
    if (!g) return;

    ResetRound(g);
    g->isEndlessMode = 1;

    g->currentMap = 3;   // 用第4关地图（双路径都启用）
    g->endlessWave = 0;

    g->base.hp = INIT_BASE_HP;
    g->base.maxHp = INIT_BASE_HP;
    g->base.radius = 30;

    // 基地位置 = 地图3路径终点平均值
    {
        int sumX = 0, sumY = 0, cnt = 0;
        for (int pid = 0; pid < PATH_COUNT; pid++) {
            Path* p = &g->paths[3][pid];
            if (p->count > 0) {
                Point end = p->nodes[p->count - 1];
                sumX += end.x;
                sumY += end.y;
                cnt++;
            }
        }
        if (cnt > 0) {
            int baseGx = sumX / cnt;
            int baseGy = sumY / cnt;
            g->base.x = SCREEN_MARGIN_X + baseGx * TILE_SIZE_X + TILE_SIZE_X / 2;
            g->base.y = SCREEN_MARGIN_Y + baseGy * TILE_SIZE_Y + TILE_SIZE_Y / 2;
        }
        else {
            g->base.x = 600;
            g->base.y = 600;
        }
    }

    g->money = 200;

    g->waveIndex = 0;
    g->spawnTimer = 0.0f;
    g->groupIndex = 0;
    g->currentGroupSpawnedCount = 0;
    g->totalWaves = MAX_WAVES;
    g->enemiesRemaining = 0;

    // 立即生成第 0 波配置
    GenerateEndlessWave(g, 0);
    g->endlessWave = 0;
    WaveConfig* w0 = &g->levelWaves[3][0];
    for (int i = 0; i < w0->groupCount; i++) {
        g->enemiesRemaining += w0->groups[i].count;
    }

    g->state = STATE_ENDLESS;
}

/* 功能：判断当前关是否还有下一关。 */
int HasNextLevel(const Game* g) {
    return g->currentMap < TOTAL_LEVELS - 1;
}

/* 功能：按波次配置定时生成敌人。 */
void SpawnUpdate(Game* g, float dt) {
    if (g->waveIndex >= g->totalWaves) return;

    WaveConfig* wave = &g->levelWaves[g->currentMap][g->waveIndex];

    // 阶段 A：本波还有怪没刷完 → 定时刷怪
    if (g->enemiesRemaining > 0) {
        g->spawnTimer += dt;
        if (g->spawnTimer >= wave->spawnInterval) {
            g->spawnTimer -= wave->spawnInterval;

            // 跳过已经刷完的组
            while (g->groupIndex < wave->groupCount &&
                g->currentGroupSpawnedCount >= wave->groups[g->groupIndex].count) {
                g->groupIndex++;
                g->currentGroupSpawnedCount = 0;
            }

            if (g->groupIndex < wave->groupCount) {
                SpawnGroup* grp = &wave->groups[g->groupIndex];
                SpawnOneEnemy(g, grp->enemyType, grp->pathId);
                g->currentGroupSpawnedCount++;
                g->enemiesRemaining--;
            }
        }
    }
    // 阶段 B：本波刷完 → 等清场 + 计时，进入下一波
    else {
        if (CountAliveEnemies(g) == 0) {
            g->spawnTimer += dt;
            if (g->spawnTimer >= wave->waveDelay) {
                g->waveIndex++;
                g->groupIndex = 0;
                g->currentGroupSpawnedCount = 0;
                g->spawnTimer = 0.0f;

                // 无尽模式：动态生成下一波配置
                if (g->state == STATE_ENDLESS) {
                    GenerateEndlessWave(g, g->waveIndex);
                    g->endlessWave = g->waveIndex;
                }

                if (g->waveIndex < g->totalWaves) {
                    WaveConfig* next = &g->levelWaves[g->currentMap][g->waveIndex];
                    int total = 0;
                    for (int i = 0; i < next->groupCount; i++) {
                        total += next->groups[i].count;
                    }
                    g->enemiesRemaining = total;
                }
            }
        }
        else {
            g->spawnTimer = 0.0f;
        }
    }
}

/* 功能：生成一个 type 型敌人，走 pathId 路径。 */
void SpawnOneEnemy(Game* g, int type, int pathId) {
    if (type < 0 || type >= MAX_ENEMY_TYPES) return;
    if (pathId < 0 || pathId >= PATH_COUNT) return;

    // 优先复用已死亡槽位
    int idx = -1;
    for (int i = 0; i < g->enemyCount; i++) {
        if (!g->enemies[i].alive) { idx = i; break; }
    }
    if (idx == -1) {
        if (g->enemyCount >= MAX_ENEMIES) return;
        idx = g->enemyCount;
        g->enemyCount++;
    }

    Enemy* e = &g->enemies[idx];
    e->type = type;
    e->hp = g->enemyConfigs[type].hp;
    e->maxHp = g->enemyConfigs[type].hp;
    e->speed = g->enemyConfigs[type].speed;
    e->damage = g->enemyConfigs[type].damage;
    e->reward = g->enemyConfigs[type].reward;

    // 无尽模式按波数缩放属性
    if (g->state == STATE_ENDLESS) {
        int wave = g->waveIndex + 1;
        float hpScale = 1.0f + (wave - 1) * 0.1f;
        e->hp = (int)(g->enemyConfigs[type].hp * hpScale);
        e->maxHp = e->hp;

        float spdScale = 1.0f + (wave / 5) * 0.1f;
        if (spdScale > 1.5f) spdScale = 1.5f;
        e->speed = g->enemyConfigs[type].speed * spdScale;
    }

    Path* path = &g->paths[g->currentMap][pathId];
    if (path->count > 0) {
        Point start = path->nodes[0];
        e->x = (float)(SCREEN_MARGIN_X + start.x * TILE_SIZE_X + TILE_SIZE_X / 2);
        e->y = (float)(SCREEN_MARGIN_Y + start.y * TILE_SIZE_Y + TILE_SIZE_Y / 2);
    }
    else {
        e->x = 0.0f;
        e->y = 0.0f;
    }

    e->pathId = pathId;
    e->pathIndex = 1;
    e->alive = 1;
    e->slowTimer = 0.0f;
    e->isBoss = 0;
}

/* 功能：按波数动态生成无尽模式波次配置。 */
void GenerateEndlessWave(Game* g, int waveIndex) {
    WaveConfig* w = &g->levelWaves[g->currentMap][waveIndex];
    memset(w, 0, sizeof(WaveConfig));

    int level = waveIndex + 1;

    // 难度公式
    int yhTotal = 5 + level * 2;
    int awTotal = (level >= 5) ? 2 + (level - 5) : 0;
    int ddTotal = (level >= 10) ? 1 + (level - 10) / 2 : 0;

    // 平分到两条路径
    int yhPath0 = yhTotal / 2;
    int yhPath1 = yhTotal - yhPath0;
    int awPath0 = awTotal / 2;
    int awPath1 = awTotal - awPath0;
    int ddPath0 = ddTotal / 2;
    int ddPath1 = ddTotal - ddPath0;

    int gi = 0;

    // 交错放置 groups（Yh→Yh→Aw→Aw→Dd→Dd）
    if (yhPath0 > 0) { w->groups[gi].enemyType = 0; w->groups[gi].count = yhPath0; w->groups[gi].pathId = 0; gi++; }
    if (yhPath1 > 0) { w->groups[gi].enemyType = 0; w->groups[gi].count = yhPath1; w->groups[gi].pathId = 1; gi++; }
    if (awPath0 > 0) { w->groups[gi].enemyType = 1; w->groups[gi].count = awPath0; w->groups[gi].pathId = 0; gi++; }
    if (awPath1 > 0) { w->groups[gi].enemyType = 1; w->groups[gi].count = awPath1; w->groups[gi].pathId = 1; gi++; }
    if (ddPath0 > 0) { w->groups[gi].enemyType = 2; w->groups[gi].count = ddPath0; w->groups[gi].pathId = 0; gi++; }
    if (ddPath1 > 0) { w->groups[gi].enemyType = 2; w->groups[gi].count = ddPath1; w->groups[gi].pathId = 1; gi++; }

    w->groupCount = gi;
    w->spawnInterval = 0.8f;
    w->waveDelay = 5.0f;
    w->isBossWave = 0;
}

/* 功能：敌人沿路径移动。 */
void EnemyMove(Game* g, float dt) {
    if (!g) return;

    for (int i = 0; i < g->enemyCount; i++) {
        Enemy* e = &g->enemies[i];
        if (!e->alive) continue;
        if (e->isBoss) continue;

        Path* path = &g->paths[g->currentMap][e->pathId];
        if (e->pathIndex >= path->count) continue;

        Point target = path->nodes[e->pathIndex];
        float tx = (float)(SCREEN_MARGIN_X + target.x * TILE_SIZE_X + TILE_SIZE_X / 2);
        float ty = (float)(SCREEN_MARGIN_Y + target.y * TILE_SIZE_Y + TILE_SIZE_Y / 2);

        float realSpeed = e->speed;
        if (e->slowTimer > 0.0f) realSpeed = e->speed * 0.5f;
        float step = realSpeed * dt;

        float dx = tx - e->x;
        float dy = ty - e->y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist <= step || dist <= 0.001f) {
            e->x = tx;
            e->y = ty;
            e->pathIndex++;
        }
        else {
            e->x += (dx / dist) * step;
            e->y += (dy / dist) * step;
        }
    }
}

/* 功能：递减敌人状态计时（目前只有减速）。 */
void UpdateEnemyStatus(Game* g, float dt) {
    for (int i = 0; i < g->enemyCount; i++) {
        Enemy* e = &g->enemies[i];
        if (e->alive == 0) continue;

        if (e->slowTimer > 0) {
            e->slowTimer -= dt;
            if (e->slowTimer <= 0) {
                e->slowTimer = 0.0f;
            }
        }
    }
}

/* 功能：防御塔自动攻击。 */
void TowerAttackUpdate(Game* g, float dt) {
    for (int i = 0; i < g->towerCount; i++) {
        Tower* tower = &g->towers[i];
        if (!tower->placed) continue;

        tower->cooldown -= dt;
        // 空闲时 cooldown 停在 0，方便动画判断
        if (tower->cooldown < 0.0f) tower->cooldown = 0.0f;

        if (tower->cooldown > 0.0f) continue;

        int hitCount = 0;
        int hitAny = 0;

        for (int j = 0; j < g->enemyCount; j++) {
            Enemy* enemy = &g->enemies[j];
            if (enemy->alive == 0) continue;
            if (enemy->isBoss) continue;

            int towerGridX = (tower->x - SCREEN_MARGIN_X) / TILE_SIZE_X;
            int towerGridY = (tower->y - SCREEN_MARGIN_Y) / TILE_SIZE_Y;
            int enemyGridX = (int)((enemy->x - SCREEN_MARGIN_X) / TILE_SIZE_X);
            int enemyGridY = (int)((enemy->y - SCREEN_MARGIN_Y) / TILE_SIZE_Y);

            int halfRange = tower->rangeGrid / 2;
            if (abs(enemyGridX - towerGridX) > halfRange) continue;
            if (abs(enemyGridY - towerGridY) > halfRange) continue;

            enemy->hp -= tower->atk;
            hitAny = 1;

            if (tower->type == Hinata) {
                // 日向：无特效
            }
            else if (tower->type == Chino) {
                enemy->slowTimer = 2.0f;
            }
            else if (tower->type == Kanna) {
                hitCount++;
            }

            if (enemy->hp <= 0) {
                enemy->alive = 0;
                KillEnemy(g, j);
            }

            if (tower->special == SINGLE) break;
            if (tower->type == Kanna && hitCount >= 3) break;
        }

        // 只有真的命中才重置冷却
        if (hitAny) {
            tower->cooldown = tower->attackInterval;
        }
    }
}

/* 功能：敌人死亡处理。 */
void KillEnemy(Game* g, int enemyIndex) {
    if (!g) return;
    if (enemyIndex < 0 || enemyIndex >= g->enemyCount) return;

    Enemy* e = &g->enemies[enemyIndex];
    e->alive = 0;
    PickReward(g, enemyIndex);

    // 第 5 关：小怪死亡 → BOSS 扣 25% 最大血量
    if (g->currentMap == 4 && !e->isBoss) {
        for (int i = 0; i < g->enemyCount; i++) {
            if (g->enemies[i].alive && g->enemies[i].isBoss) {
                g->enemies[i].hp -= g->enemies[i].maxHp / 35;
                if (g->enemies[i].hp <= 0) {
                    g->enemies[i].alive = 0;
                    PickReward(g, i);
                }
                break;
            }
        }
    }
}

/* 功能：击杀奖励。 */
void PickReward(Game* g, int enemyIndex) {
    if (!g) return;
    if (enemyIndex < 0 || enemyIndex >= g->enemyCount) return;
    g->money += g->enemies[enemyIndex].reward;
}

/* 功能：基地碰撞检测。 */
void BaseHitCheck(Game* g) {
    if (!g) return;

    for (int i = 0; i < g->enemyCount; i++) {
        Enemy* e = &g->enemies[i];

        int baseGx = (g->base.x - SCREEN_MARGIN_X) / TILE_SIZE_X;
        int baseGy = (g->base.y - SCREEN_MARGIN_Y) / TILE_SIZE_Y;

        if (!e->alive) continue;
        if (e->isBoss) continue;

        int eGx = (int)((e->x - SCREEN_MARGIN_X) / TILE_SIZE_X);
        int eGy = (int)((e->y - SCREEN_MARGIN_Y) / TILE_SIZE_Y);

        int dx = abs(eGx - baseGx);
        int dy = abs(eGy - baseGy);
        if (dx <= 1 && dy <= 1) {
            g->base.hp -= e->damage;
            g->baseFlashTimer = 0.2f;
            e->alive = 0;
        }
    }
}

/* 功能：判断格子是否可建造。 */
int CanPlaceAt(const Game* g, int gx, int gy) {
    if (gx >= MAP_W || gx < 0 || gy >= MAP_H || gy < 0) return 0;
    if (g->mapData[g->currentMap][gy][gx] != 2) return 0;

    for (int i = 0; i < g->towerCount; i++) {
        if (g->towers[i].placed != 1) continue;
        int towerGx = (g->towers[i].x - SCREEN_MARGIN_X) / TILE_SIZE_X;
        int towerGy = (g->towers[i].y - SCREEN_MARGIN_Y) / TILE_SIZE_Y;
        if (towerGx == gx && towerGy == gy) return 0;
    }
    return 1;
}

/* 功能：放置防御塔。 */
int PlaceTower(Game* g, int gx, int gy, int type) {
    if (CanPlaceAt(g, gx, gy) == 0) return 0;
    if (!IsTowerUnlocked(g, type)) return -2;

    int towerCost = g->towerConfigs[type].cost;
    if (g->money < towerCost) return -1;
    if (g->towerCount >= MAX_TOWERS) return 0;

    int idx = g->towerCount;
    Tower* tower = &g->towers[idx];

    tower->x = SCREEN_MARGIN_X + gx * TILE_SIZE_X + TILE_SIZE_X / 2;
    tower->y = SCREEN_MARGIN_Y + gy * TILE_SIZE_Y + TILE_SIZE_Y / 2;
    tower->atk = g->towerConfigs[type].atk;
    tower->rangeGrid = g->towerConfigs[type].rangeGrid;
    tower->attackInterval = g->towerConfigs[type].attackInterval;
    tower->cost = towerCost;
    tower->special = g->towerConfigs[type].special;
    tower->cooldown = 0.0f;
    tower->placed = 1;
    tower->type = type;

    g->towerCount++;
    g->money -= towerCost;
    return 1;
}

/* 功能：拆除防御塔。 */
int RemoveTower(Game* g, int gx, int gy) {
    for (int i = 0; i < g->towerCount; i++) {
        int towerGx = (g->towers[i].x - SCREEN_MARGIN_X) / TILE_SIZE_X;
        int towerGy = (g->towers[i].y - SCREEN_MARGIN_Y) / TILE_SIZE_Y;
        if (g->towers[i].placed == 1 && towerGx == gx && towerGy == gy) {
            int refund = (int)(g->towers[i].cost * 0.5f);
            g->money += refund;
            g->towers[i].placed = 0;
            g->towers[i].x = 0;
            g->towers[i].y = 0;
            g->towers[i].atk = 0;
            g->towers[i].rangeGrid = 0;
            g->towers[i].attackInterval = 0.0f;
            g->towers[i].cooldown = 0.0f;
            g->towers[i].cost = 0;
            g->towers[i].type = -1;
            return 1;
        }
    }
    return 0;
}

/* 功能：统计场上存活小怪数（排除 BOSS）。 */
int CountAliveEnemies(const Game* g) {
    if (g == NULL) return 0;
    int count = 0;
    for (int i = 0; i < g->enemyCount; i++) {
        if (g->enemies[i].alive && !g->enemies[i].isBoss) {
            count++;
        }
    }
    return count;
}

/* 功能：每帧判定胜负（普通关卡用）。 */
GameState CheckWinLose(Game* g) {
    if (g->base.hp <= 0) return STATE_RESULT;

    if (g->waveIndex >= g->totalWaves && g->enemiesRemaining == 0) {
        if (CountAliveEnemies(g) == 0) {
            if (g->currentMap == 4) {
                int bossAlive = 0;
                for (int i = 0; i < g->enemyCount; i++) {
                    if (g->enemies[i].alive && g->enemies[i].isBoss) {
                        bossAlive = 1;
                        break;
                    }
                }
                if (bossAlive) return STATE_PLAYING;
            }
            return STATE_RESULT;
        }
    }
    return STATE_PLAYING;
}

/* 功能：保存本地存档。 */
void SaveGame(const Game* g) {
    FILE* fp;
    errno_t err = fopen_s(&fp, "save_llw.txt", "w");
    if (fp == NULL) {
        printf("存档打开失败");
        return;
    }
    fprintf(fp, "%d %d %d", g->unlockedCount, g->highScore, g->endlessBestWave);
    fclose(fp);
}

/* 功能：读取本地存档。 */
void LoadGame(Game* g) {
    FILE* fp;
    errno_t err = fopen_s(&fp, "save_llw.txt", "r");
    if (fp == NULL) {
        printf("无存档,使用默认初始数据 \n");
        g->unlockedCount = 1;
        g->endlessBestWave = 0;
        g->highScore = 0;
        return;
    }
    fscanf_s(fp, "%d %d %d", &g->unlockedCount, &g->highScore, &g->endlessBestWave);
    fclose(fp);
    printf("读档成功\n");
}

/* 功能：像素坐标 → 格子下标。 */
Point pixelToGrid(int px, int py) {
    Point p = { -1, -1 };

    if (px < SCREEN_MARGIN_X || px >= SCREEN_MARGIN_X + MAPWINDOW_W ||
        py < SCREEN_MARGIN_Y || py >= SCREEN_MARGIN_Y + MAPWINDOW_H) {
        return p;
    }

    int gx = (px - SCREEN_MARGIN_X) / TILE_SIZE_X;
    int gy = (py - SCREEN_MARGIN_Y) / TILE_SIZE_Y;

    if (gx >= 0 && gx < MAP_W && gy >= 0 && gy < MAP_H) {
        p.x = gx;
        p.y = gy;
    }
    return p;
}

// ==================== view 实现区 ====================

/* 功能：判断鼠标左键是否点击在按钮内。 */
int Button_isClicked(const Button* btn, const ExMessage* msg) {
    if (msg->message == WM_LBUTTONDOWN &&
        msg->x >= btn->x && msg->x < (btn->x + btn->w) &&
        msg->y >= btn->y && msg->y < (btn->y + btn->h)) {
        return 1;
    }
    return 0;
}

/* 功能：绘制垂直渐变圆角矩形。 */
void GradientRoundRectV(int left, int top, int right, int bottom,
    int radius, COLORREF colorTop, COLORREF colorBottom)
{
    int height = bottom - top;
    int width = right - left;
    if (height <= 0 || width <= 0) return;

    if (radius > width / 2)  radius = width / 2;
    if (radius > height / 2) radius = height / 2;

    int r1 = GetRValue(colorTop), g1 = GetGValue(colorTop), b1 = GetBValue(colorTop);
    int r2 = GetRValue(colorBottom), g2 = GetGValue(colorBottom), b2 = GetBValue(colorBottom);

    for (int y = 0; y < height; y++)
    {
        double t = (double)y / (height - 1);
        int r = (int)(r1 + (r2 - r1) * t);
        int g = (int)(g1 + (g2 - g1) * t);
        int b = (int)(b1 + (b2 - b1) * t);

        int offset = 0;
        double dy = 0;
        if (y < radius) dy = radius - y;
        else if (height - 1 - y < radius) dy = radius - (height - 1 - y);

        if (dy > 0)
        {
            double dx = sqrt((double)radius * radius - dy * dy);
            offset = (int)(radius - dx + 0.5);
        }

        setlinecolor(RGB(r, g, b));
        line(left + offset, top + y, right - offset, top + y);
    }
}

/* 功能：绘制圆角边框。 */
void DrawRoundRectBorder(int left, int top, int right, int bottom,
    int radius, COLORREF borderColor, int lineWidth)
{
    setlinestyle(PS_SOLID, lineWidth);
    setlinecolor(borderColor);
    roundrect(left, top, right, bottom, radius * 2, radius * 2);
}

/* 功能：绘制按钮（渐变 + 边框 + 居中文字）。 */
void Button_draw(const Button* btn)
{
    int left = btn->x;
    int top = btn->y;
    int right = btn->x + btn->w;
    int bottom = btn->y + btn->h;
    int radius = 20;

    COLORREF colorTop = RGB(255, 202, 218);
    COLORREF colorBottom = RGB(251, 229, 233);
    COLORREF borderColor = RGB(226, 149, 186);
    COLORREF textColor = RGB(120, 30, 70);

    GradientRoundRectV(left, top, right, bottom, radius, colorTop, colorBottom);
    DrawRoundRectBorder(left, top, right, bottom, radius, borderColor, 2);

    if (btn->text != NULL)
    {
        setbkmode(TRANSPARENT);
        settextcolor(textColor);

        int fontSize = (int)(btn->h * 0.4);
        settextstyle(fontSize, 0, _T("微软雅黑"));

        int textW = textwidth(btn->text);
        int textH = textheight(btn->text);

        int tx = left + (btn->w - textW) / 2;
        int ty = top + (btn->h - textH) / 2;
        outtextxy(tx, ty, btn->text);
    }
}

/* 功能：主菜单输入处理。 */
GameState Menu_update(Game* g, ExMessage* msg) {
    int btnX1[6] = { 520, 520, 520, 520, 520, 1070 };
    int btnY1[6] = { 270, 360, 450, 540, 630,   85 };
    int btnX2[6] = { 760, 760, 760, 760, 760, 1210 };
    int btnY2[6] = { 335, 425, 515, 605, 695,  135 };

    if (!g || !msg) return STATE_MENU;

    g->mouseX = msg->x;
    g->mouseY = msg->y;

    int hit = -1;
    for (int i = 0; i < 6; i++) {
        if (msg->x >= btnX1[i] && msg->x <= btnX2[i] &&
            msg->y >= btnY1[i] && msg->y <= btnY2[i]) {
            hit = i;
            break;
        }
    }

    if (msg->message == WM_LBUTTONDOWN && hit >= 0) {
        switch (hit) {
        case 0: g->isEndlessMode = 0; return STATE_LEVEL_SELECT;
        case 1:
            if (g->unlockedCount >= 5) {
                LoadEndless(g);
                return STATE_ENDLESS;
            }
            return STATE_MENU;
        case 2: return STATE_GALLERY;
        case 3: return STATE_SETTINGS;
        case 4: return STATE_TEAM;
        case 5: SaveGame(g); return STATE_EXIT;
        }
    }
    return STATE_MENU;
}

/* 功能：绘制主菜单。 */
void DrawMenu(const Game* g) {
    int btnX1[6] = { 520, 520, 520, 520, 520, 1070 };
    int btnY1[6] = { 270, 360, 450, 540, 630,   85 };
    int btnX2[6] = { 760, 760, 760, 760, 760, 1210 };
    int btnY2[6] = { 335, 425, 515, 605, 695,  135 };

    settextcolor(RGB(220, 80, 120));
    settextstyle(28, 0, L"微软雅黑");
    setbkmode(TRANSPARENT);
    const wchar_t* menuTexts[6] = { L"开始游戏", L"无尽模式", L"图鉴&玩法教学", L"设置", L"制作名单&声明", L"残忍退出" };

    cleardevice();
    putimage(0, 0, &im_menuBg);
    for (int i = 0; i < 6; i++) {
        bool hover = (g->mouseX >= btnX1[i] && g->mouseX <= btnX2[i] &&
            g->mouseY >= btnY1[i] && g->mouseY <= btnY2[i]);

        if (hover) {
            setfillcolor(RGB(255, 200, 220));
            setlinecolor(RGB(220, 80, 120));
            settextcolor(RGB(160, 30, 80));
        }
        else {
            setfillcolor(RGB(255, 240, 245));
            setlinecolor(RGB(230, 100, 140));
            settextcolor(RGB(220, 80, 120));
        }

        Button btn = { 0 };
        btn.x = btnX1[i];
        btn.y = btnY1[i];
        btn.w = btnX2[i] - btnX1[i];
        btn.h = btnY2[i] - btnY1[i];
        btn.text = menuTexts[i];
        Button_draw(&btn);

        if (hover) {
            setlinestyle(PS_SOLID, 3);
            rectangle(btnX1[i] - 2, btnY1[i] - 2,
                btnX2[i] + 2, btnY2[i] + 2);
            setlinestyle(PS_SOLID, 1);
        }
    }
}

/* 功能：关卡选择输入处理。 */
GameState LevelSelect_update(Game* g, ExMessage* msg) {
    int btnX1[5] = { 65,  310,  555,  800, 1045 };
    int btnY1 = 430;
    int btnX2[5] = { 235,  480,  725,  970, 1215 };
    int btnY2 = 500;

    int backX1 = 1105, backY1 = 650, backX2 = 1250, backY2 = 700;

    if (!g || !msg) return STATE_LEVEL_SELECT;

    g->mouseX = msg->x;
    g->mouseY = msg->y;

    if (msg->message == WM_LBUTTONDOWN) {
        int mx = msg->x;
        int my = msg->y;

        if (mx >= backX1 && mx <= backX2 && my >= backY1 && my <= backY2) {
            return STATE_MENU;
        }

        for (int i = 0; i < 5; i++) {
            if (mx >= btnX1[i] && mx <= btnX2[i] && my >= btnY1 && my <= btnY2) {
                if (i < g->unlockedCount) {
                    g->currentMap = i;
                    LoadLevel(g, i);
                    return STATE_STORY;
                    //return STATE_PLAYING;
                }
                return STATE_LEVEL_SELECT;
            }
        }
    }
    return STATE_LEVEL_SELECT;
}

/* 功能：绘制关卡选择界面。 */
void DrawLevelSelect(const Game* g) {
    Button btn = { 0 };
    int btnX1[5] = { 65, 310, 555, 800, 1045 };
    int btnY1 = 430;
    int btnX2[5] = { 235, 480, 725, 970, 1215 };
    int btnY2 = 500;
    int backX1 = 1105, backY1 = 650, backX2 = 1250, backY2 = 700;
    const wchar_t* levelNames[5] = { L"关卡1", L"关卡2", L"关卡3", L"关卡4", L"关卡5" };

    cleardevice();
    putimage(0, 0, &im_level_selectBg);
    settextstyle(28, 0, L"微软雅黑");
    setbkmode(TRANSPARENT);

    for (int i = 0; i < 5; i++) {
        btn.x = btnX1[i];
        btn.y = btnY1;
        btn.w = btnX2[i] - btnX1[i];
        btn.h = btnY2 - btnY1;
        btn.text = levelNames[i];
        Button_draw(&btn);
    }

    btn.x = backX1;
    btn.y = backY1;
    btn.w = backX2 - backX1;
    btn.h = backY2 - backY1;
    btn.text = L"返回主菜单";
    Button_draw(&btn);

    // 高亮
    for (int i = 0; i < 5; i++) {
        if (g->mouseX >= btnX1[i] && g->mouseX <= btnX2[i] &&
            g->mouseY >= btnY1 && g->mouseY <= btnY2) {
            setlinestyle(PS_SOLID, 4);
            setlinecolor(RGB(255, 202, 218));
            rectangle(btnX1[i], btnY1, btnX2[i], btnY2);
        }
    }
    if (g->mouseX >= backX1 && g->mouseX <= backX2 &&
        g->mouseY >= backY1 && g->mouseY <= backY2) {
        setlinestyle(PS_SOLID, 4);
        setlinecolor(RGB(255, 202, 218));
        rectangle(backX1, backY1, backX2, backY2);
    }
}

/* 功能：团队介绍界面输入处理。 */
GameState Team_update(Game* g, ExMessage* msg) {
    Button btn = { 540, 670, 200, 50, _T("返回主菜单") };
    if (!Button_isClicked(&btn, msg)) {
        return STATE_TEAM;
    }
    else {
        return STATE_MENU;
    }
}

/* 功能：绘制团队介绍界面。 */
void DrawTeam(const Game* g) {
    cleardevice();
    putimage(0, 0, &Detail);
    Button btn = { 540, 670, 200, 50, _T("返回主菜单") };
    Button_draw(&btn);
    if (g->mouseX >= btn.x && g->mouseX <= btn.x + btn.w &&
        g->mouseY >= btn.y && g->mouseY <= btn.y + btn.h) {
        setlinestyle(PS_SOLID, 4);
        setlinecolor(RGB(255, 202, 218));
        rectangle(btn.x, btn.y, btn.x + btn.w, btn.y + btn.h);
    }
}

/* 功能：暂停菜单输入处理。 */
GameState Pause_update(Game* g, ExMessage* msg) {
    int level = g->currentMap;
    Button btn1 = { 440, 250, 400, 90, _T("继续游戏") };
    Button btn2 = { 440, 350, 400, 90, _T("重新开始") };
    Button btn3 = { 440, 450, 400, 90, _T("返回主菜单") };

    if (Button_isClicked(&btn1, msg) ||
        (msg->message == WM_KEYDOWN && msg->vkcode == VK_SPACE)) {
        if (g->isEndlessMode) return STATE_ENDLESS;
        else return STATE_PLAYING;
    }
    else if (Button_isClicked(&btn2, msg)) {
        if (g->isEndlessMode) {
            LoadEndless(g);
            return STATE_ENDLESS;
        }
        LoadLevel(g, level);
        return STATE_PLAYING;
    }
    else if (Button_isClicked(&btn3, msg)) {
        g->isEndlessMode = 0;
        return STATE_MENU;
    }
    return STATE_PAUSED;
}

/* 功能：绘制暂停菜单。 */
void DrawPause(const Game* g) {
    cleardevice();
    putimage(0, 0, &im_pauseBg);
    Button btn1 = { 440, 250, 400, 90, _T("继续游戏") };
    Button_draw(&btn1);
    Button btn2 = { 440, 350, 400, 90, _T("重新开始") };
    Button_draw(&btn2);
    Button btn3 = { 440, 450, 400, 90, _T("返回主菜单") };
    Button_draw(&btn3);

    Button* btns[3] = { &btn1, &btn2, &btn3 };
    for (int i = 0; i < 3; i++) {
        int x = btns[i]->x, y = btns[i]->y;
        int w = btns[i]->w, h = btns[i]->h;
        if (g->mouseX >= x && g->mouseX <= x + w &&
            g->mouseY >= y && g->mouseY <= y + h) {
            setlinestyle(PS_SOLID, 4);
            setlinecolor(RGB(255, 202, 218));
            rectangle(x, y, x + w, y + h);
        }
    }
}

/* 功能：游戏主界面输入处理。 */
GameState GameView_update(Game* g, ExMessage* msg) {
    if (!g || !msg) return STATE_PLAYING;

    if (msg->message == WM_KEYDOWN) {
        if (msg->vkcode == VK_SPACE)  return STATE_PAUSED;
        if (msg->vkcode == VK_ESCAPE) return STATE_MENU;
        return STATE_PLAYING;
    }

    if (msg->message == WM_LBUTTONDOWN) {

        if (msg->x > 0 && msg->x < 100 && msg->y > 50 && msg->y < 150) {
            return STATE_PAUSED;
        }

        if (msg->x > 1150 && msg->x < 1280 && msg->y > 200 && msg->y < 300) {
            if (IsTowerUnlocked(g, Hinata))
                g->selectedTowerType = (g->selectedTowerType == Hinata) ? -1 : Hinata;
            return STATE_PLAYING;
        }
        else if (msg->x > 1150 && msg->x < 1280 && msg->y > 300 && msg->y < 400) {
            if (IsTowerUnlocked(g, Chino))
                g->selectedTowerType = (g->selectedTowerType == Chino) ? -1 : Chino;
            return STATE_PLAYING;
        }
        else if (msg->x > 1150 && msg->x < 1280 && msg->y > 400 && msg->y < 500) {
            if (IsTowerUnlocked(g, Kanna))
                g->selectedTowerType = (g->selectedTowerType == Kanna) ? -1 : Kanna;
            return STATE_PLAYING;
        }

        if (g->selectedTowerType != -1) {
            Point p = pixelToGrid(msg->x, msg->y);
            if (p.x == -1) return STATE_PLAYING;
            if (CanPlaceAt(g, p.x, p.y)) {
                PlaceTower(g, p.x, p.y, g->selectedTowerType);
            }
        }
        return STATE_PLAYING;
    }

    if (msg->message == WM_RBUTTONDOWN) {
        Point p = pixelToGrid(msg->x, msg->y);
        if (p.x == -1) return STATE_PLAYING;
        RemoveTower(g, p.x, p.y);
        return STATE_PLAYING;
    }

    return STATE_PLAYING;
}

/* 功能：渲染游戏主界面。 */
void GameRender(const Game* g) {
    cleardevice();
    putimage(0, 0, &im_mapBg);
    putimage(1150, 0, &Show_info);
    DrawUI(g);
    putimage(0, 50, &Bt_pause);

    if (IsTowerUnlocked(g, Hinata)) putimage(1150, 200, &Bt_Hinata);
    if (IsTowerUnlocked(g, Chino))  putimage(1150, 300, &Bt_Chino);
    if (IsTowerUnlocked(g, Kanna))  putimage(1150, 400, &Bt_Kanna);

    if (g->selectedTowerType == Hinata) {
        setlinestyle(PS_SOLID, 4);
        setlinecolor(RED);
        rectangle(1150, 200, 1280, 300);
    }
    else if (g->selectedTowerType == Chino) {
        setlinestyle(PS_SOLID, 4);
        setlinecolor(RED);
        rectangle(1150, 300, 1280, 400);
    }
    else if (g->selectedTowerType == Kanna) {
        setlinestyle(PS_SOLID, 4);
        setlinecolor(RED);
        rectangle(1150, 400, 1280, 500);
    }

    // 已建塔
    for (int i = 0; i < g->towerCount; i++) {
        const Tower* t = &g->towers[i];
        if (!t->placed) continue;

        int px = t->x - TILE_SIZE_X / 2;
        int py = t->y - TILE_SIZE_Y / 2;
        int rx = t->x;
        int ry = t->y;

        bool attacking = false;
        if (t->attackInterval > 0.0f) {
            float showRatio = 0.3f;
            float threshold = t->attackInterval * (1.0f - showRatio);
            if (t->cooldown > threshold) attacking = true;
        }

        switch (t->type) {
        case Hinata:
            if (attacking) {
                setlinecolor(RGB(236, 163, 76));
                setlinestyle(PS_SOLID, 2);
                circle(rx, ry, g->towerConfigs[Hinata].rangeGrid * TILE_SIZE_X / 2);
                setlinestyle(PS_SOLID, 1);
                putimage(px, py, &im_towerHinata_atk_mask, SRCAND);
                putimage(px, py, &im_towerHinata_atk_color, SRCPAINT);
            }
            else {
                putimage(px, py, &im_towerHinata_mask, SRCAND);
                putimage(px, py, &im_towerHinata_color, SRCPAINT);
            }
            break;
        case Chino:
            if (attacking) {
                setlinecolor(RGB(170, 225, 246));
                setlinestyle(PS_SOLID, 1);
                circle(rx, ry, g->towerConfigs[Chino].rangeGrid * TILE_SIZE_X / 2);
                setlinestyle(PS_SOLID, 1);
                putimage(px, py, &im_towerChino_atk_mask, SRCAND);
                putimage(px, py, &im_towerChino_atk_color, SRCPAINT);
            }
            else {
                putimage(px, py, &im_towerChino_mask, SRCAND);
                putimage(px, py, &im_towerChino_color, SRCPAINT);
            }
            break;
        case Kanna:
            if (attacking) {
                setlinecolor(RGB(255, 192, 203));
                setlinestyle(PS_SOLID, 3);
                circle(rx, ry, g->towerConfigs[Kanna].rangeGrid * TILE_SIZE_X / 2);
                setlinestyle(PS_SOLID, 1);
                putimage(px, py, &im_towerKanna_atk_mask, SRCAND);
                putimage(px, py, &im_towerKanna_atk_color, SRCPAINT);
            }
            else {
                putimage(px, py, &im_towerKanna_mask, SRCAND);
                putimage(px, py, &im_towerKanna_color, SRCPAINT);
            }
            break;
        }
    }

    // 敌人 + 血条
    for (int i = 0; i < g->enemyCount; i++) {
        const Enemy* e = &g->enemies[i];
        if (!e->alive) continue;

        int px = (int)e->x - TILE_SIZE_X / 2;
        int py = (int)e->y - TILE_SIZE_Y / 2;

        if (e->isBoss) {
            py = (int)e->y - TILE_SIZE_Y;
            putimage(px, py, &im_boss_mask, SRCAND);
            putimage(px, py, &im_boss_color, SRCPAINT);
        }
        else {
            switch (e->type) {
            case 0:
                putimage(px, py, &im_enemyYh_mask, SRCAND);
                putimage(px, py, &im_enemyYh_color, SRCPAINT);
                break;
            case 1:
                putimage(px, py, &im_enemyAw_mask, SRCAND);
                putimage(px, py, &im_enemyAw_color, SRCPAINT);
                break;
            case 2:
                putimage(px, py, &im_enemyDd_mask, SRCAND);
                putimage(px, py, &im_enemyDd_color, SRCPAINT);
                break;
            }
        }

        int barW;
        int barH = 4;

        if (e->isBoss) {
            barW = 120;   // BOSS 血条宽度，可自己调大，比如 120、150
        }
        else {
            barW = TILE_SIZE_X - 10;   // 普通敌人仍然是 40
        }

        int barX;
        if (e->isBoss) {
            barX = (int)e->x + 50 - barW / 2;
        }
        else {
            barX = (int)e->x - barW / 2;
        }
        int barY = py - 8;

        setfillcolor(RGB(60, 60, 60));
        solidrectangle(barX, barY, barX + barW, barY + barH);

        if (e->maxHp > 0 && e->hp > 0) {
            int hpW = (int)((float)e->hp / e->maxHp * barW);
            if (hpW > barW) hpW = barW;
            setfillcolor(RGB(230, 60, 60));
            solidrectangle(barX, barY, barX + hpW, barY + barH);
        }
    }

    if (g->baseFlashTimer > 0.0f) {
        setlinecolor(RGB(255, 0, 0));
        setlinestyle(PS_SOLID, 4);
        circle(g->base.x, g->base.y, g->base.radius + 4);
        setlinestyle(PS_SOLID, 1);
    }
}






/* 功能：取当前这段剧情（开始 / 结束）的总页数。
 * 说明：开始剧情用 a 系列，胜利结束剧情用 b 系列；
 *       页数随关卡(currentMap)与胜负(isWin)变化。 */
static int StoryPageCount(const Game* g) {
    int isWin = (g->base.hp > 0 && g->enemiesRemaining == 0);
    switch (g->currentMap) {
    case 0: return isWin ? Count_1b : Count_1a;
    case 1: return isWin ? Count_2b : Count_2a;
    case 2: return isWin ? Count_3b : Count_3a;
    case 3: return isWin ? Count_4b : Count_4a;
    case 4: return isWin ? Count_5b : Count_5a;
    default: return 1;
    }
}
/***************修改*****************/

/* 功能：剧情对话层输入处理。
 * 原实现 bug：函数内又写了一句 int pageNow = 0;，这个局部变量把
 *   同名的全局变量遮蔽了，于是 pageNow += 1 只加到了局部变量上，
 *   函数一返回就失效，全局 pageNow 恒为 0 —— DrawStory 里 i==pageNow
 *   只有 i=0 成立，所以永远只显示第一张剧情图。
 * 修复：删掉局部同名变量，直接更新全局 pageNow；并补齐边界钳制与
 *   “翻过最后一页自动结束本段剧情”的逻辑。 */
GameState Story_update(Game* g, ExMessage* msg) {
    /***************修改*****************/
    if (!g || !msg) return STATE_STORY;

    int total = StoryPageCount(g);              // 本段剧情总页数
    Button btn = { 1000, 10, 250, 80, NULL };   // “跳过”按钮热区(与 DrawStory 一致)

    if (msg->message == WM_LBUTTONDOWN) {
        if (Button_isClicked(&btn, msg)) {
            /* 点“跳过”：立即结束本段剧情（胜/负都交给 CheckWinLose 判定） */
            if (CheckWinLose(g) == STATE_RESULT)
                return STATE_RESULT;
            else
                return STATE_PLAYING;
        }
        else {
            /* 点击画面其它任意位置：翻到下一张 */
            pageNow += 1;
            if (pageNow >= total) {
                /* 已经是最后一张，再点一次即结束本段剧情 */
                if (CheckWinLose(g) == STATE_RESULT)
                    return STATE_RESULT;
                else
                    return STATE_PLAYING;
            }
        }
    }
    /***************修改*****************/
    return STATE_STORY;
}

/* 功能：绘制剧情对话层。
 * 修复说明：原实现用
 *     for (i = 0; i < Count_X && i == pageNow; i++) { putimage(...); }
 * 这种写法有个致命缺陷：循环变量 i 从 0 起步，而判断条件里第一个就是
 * “i == pageNow”（即 0 == pageNow）。于是
 *     pageNow == 0 → i=0 成立，画一次；
 *     pageNow  > 0 → 第一次判断 0 == pageNow 就不成立，循环体一次都不进，
 *                    屏幕上只剩 cleardevice() 的清屏结果 = 全黑。
 * 所以“点一下翻页就黑屏”，并不是图片没加载，而是这张图根本没被画。
 * 现改为按 (关卡, 胜负) 取下第 pageNow 张剧情图直接渲染。 */
void DrawStory(const Game* g) {
    int isWin = (g->base.hp > 0 && g->enemiesRemaining == 0);
    /***************修改*****************/
    /* 翻页下标越界保护，避免异常时整屏空白 */
    int total = StoryPageCount(g);
    if (pageNow < 0) pageNow = 0;
    if (pageNow >= total) pageNow = total - 1;

    cleardevice();

    /* 按 (关卡, 胜负) 取当前页对应的剧情图直接绘制 */
    IMAGE* img = NULL;
    if (g->currentMap == 0)      img = isWin ? &b1[pageNow] : &a1[pageNow];
    else if (g->currentMap == 1) img = isWin ? &b2[pageNow] : &a2[pageNow];
    else if (g->currentMap == 2) img = isWin ? &b3[pageNow] : &a3[pageNow];
    else if (g->currentMap == 3) img = isWin ? &b4[pageNow] : &a4[pageNow];
    else if (g->currentMap == 4) img = isWin ? &b5[pageNow] : &a5[pageNow];

    if (img != NULL) putimage(0, 0, img);

    /* “跳过”按钮 + 鼠标悬停高亮（各关卡按钮位置一致，统一绘制一份） */
    Button btn = { 1000, 10, 250, 80, L"跳过" };
    Button_draw(&btn);
    if (g->mouseX >= btn.x && g->mouseX <= btn.x + btn.w &&
        g->mouseY >= btn.y && g->mouseY <= btn.y + btn.h) {
        setlinestyle(PS_SOLID, 4);
        setlinecolor(RGB(255, 202, 218));
        rectangle(btn.x, btn.y, btn.x + btn.w, btn.y + btn.h);
    }
    /***************修改*****************/
}

/* 功能：结算界面输入处理。 */
GameState Result_update(Game* g, ExMessage* msg) {
    if (!g || !msg) return STATE_RESULT;

    int isWin = (g->base.hp > 0);
    Button btn = { 440, 550, 400, 90, NULL };

    if (!Button_isClicked(&btn, msg)) {
        return STATE_RESULT;
    }

    if (isWin && HasNextLevel(g)) {
        LoadLevel(g, g->currentMap + 1);
        return STATE_STORY;
        //return STATE_PLAYING;
    }
    return STATE_MENU;
}

/* 功能：绘制结算界面。 */
void DrawResult(const Game* g) {
    cleardevice();
    putimage(0, 0, &im_resultBg);

    int isWin = (g->base.hp > 0);

    int boxL = 200, boxT = 200, boxR = 880, boxB = 520;
    COLORREF colorTop, colorBottom, borderColor, textColor;
    const TCHAR* boxText;

    if (isWin) {
        colorTop = RGB(255, 220, 235);
        colorBottom = RGB(240, 170, 200);
        borderColor = RGB(220, 120, 160);
        textColor = RGB(120, 30, 70);
        boxText = L"恭喜通关！";
    }
    else {
        colorTop = RGB(190, 190, 190);
        colorBottom = RGB(120, 120, 120);
        borderColor = RGB(115, 90, 140);
        textColor = RGB(228, 163, 201);
        boxText = L"真遗憾呐......杂鱼";
    }

    GradientRoundRectV(boxL, boxT, boxR, boxB, 20, colorTop, colorBottom);
    DrawRoundRectBorder(boxL, boxT, boxR, boxB, 20, borderColor, 2);

    setbkmode(TRANSPARENT);
    settextcolor(textColor);
    settextstyle(48, 0, _T("微软雅黑"));
    int tw = textwidth(boxText);
    int th = textheight(boxText);
    outtextxy(boxL + (boxR - boxL - tw) / 2, boxT + (boxB - boxT - th) / 2, boxText);

    Button btn;
    if (isWin && HasNextLevel(g)) {
        btn = { 440, 550, 400, 90, L"下一关" };
    }
    else {
        btn = { 440, 550, 400, 90, L"返回主菜单" };
    }
    Button_draw(&btn);
}

/* 功能：设置界面输入处理。 */
GameState Settings_update(Game* g, ExMessage* msg) {
    Button btnVolumeUp = { 440, 250, 400, 90, L"+" };
    Button btnVolumeDown = { 440, 350, 400, 90, L"-" };
    Button btnBack = { 440, 450, 400, 90, L"返回主菜单" };

    if (Button_isClicked(&btnVolumeUp, msg)) {
        if (g->volume < 100) {
            g->volume += 10;
            if (g->volume > 100) g->volume = 100;
            SetBGMVolume(g->volume);
        }
        return STATE_SETTINGS;
    }
    else if (Button_isClicked(&btnVolumeDown, msg)) {
        if (g->volume > 0) {
            g->volume -= 10;
            if (g->volume < 0) g->volume = 0;
            SetBGMVolume(g->volume);
        }
        return STATE_SETTINGS;
    }
    else if (Button_isClicked(&btnBack, msg)) {
        return STATE_MENU;
    }
    return STATE_SETTINGS;
}

/* 功能：绘制设置界面。 */
void DrawSettings(const Game* g) {
    cleardevice();
    putimage(0, 0, &im_setBg);
    Button btn1, btn2, btn3;
    btn1 = { 440, 250, 400, 90, L"音量+" };
    Button_draw(&btn1);
    btn2 = { 440, 350, 400, 90, L"音量-" };
    Button_draw(&btn2);
    btn3 = { 440, 450, 400, 90, L"返回主菜单" };
    Button_draw(&btn3);

    Button* btns[3] = { &btn1, &btn2, &btn3 };
    for (int i = 0; i < 3; i++) {
        int x = btns[i]->x, y = btns[i]->y;
        int w = btns[i]->w, h = btns[i]->h;
        if (g->mouseX >= x && g->mouseX <= x + w &&
            g->mouseY >= y && g->mouseY <= y + h) {
            setlinestyle(PS_SOLID, 4);
            setlinecolor(RGB(255, 202, 218));
            rectangle(x, y, x + w, y + h);
        }
    }
}


static int currentBgmRepeat = 0;
static ULONGLONG bgmLastCheck = 0;
#define BGM_CHECK_INTERVAL_MS 500

/* 功能：让当前 BGM 单曲循环（wav 播完 seek 回开头再 play）。 */
static void BgmKeepPlaying(void)
{
    if (currentBgmIndex == -1) return;
    if (currentBgmRepeat == 0) return;

    ULONGLONG now = GetTickCount64();
    if (now - bgmLastCheck < BGM_CHECK_INTERVAL_MS) return;
    bgmLastCheck = now;

    wchar_t mode[32] = { 0 };
    if (mciSendString(L"status nowplaying mode", mode, 32, NULL) != 0) return;
    if (wcscmp(mode, L"playing") == 0) return;

    mciSendString(L"seek nowplaying to start", NULL, 0, NULL);
    mciSendString(L"play nowplaying", NULL, 0, NULL);
}

/* 功能：播放指定 BGM。 */
void PlayBGM(int index, int repeat)
{
    if (index < 0 || index >= (int)(sizeof(bgm) / sizeof(bgm[0]))) return;
    if (index == currentBgmIndex) return;

    wchar_t cmd[256];
    if (currentBgmIndex != -1)
        mciSendString(L"close nowplaying", NULL, 0, NULL);
    swprintf(cmd, 256, L"open music\\%s alias nowplaying", bgm[index]);

    MCIERROR err = mciSendString(cmd, NULL, 0, NULL);

    mciSendString(L"play nowplaying", NULL, 0, NULL);
    currentBgmRepeat = repeat;
    bgmLastCheck = 0;
    currentBgmIndex = index;

    extern Game game;
    SetBGMVolume(game.volume);

    if (err != 0)
    {
        _tprintf(_T("[BGM] open failed: music\\%s , err=%lu\n"),
            bgm[index], (unsigned long)err);
    }
    mciSendString(L"play nowplaying", NULL, 0, NULL);
    currentBgmRepeat = repeat;
    bgmLastCheck = 0;
    currentBgmIndex = index;
}

/* 功能：停止 BGM。 */
void StopBGM()
{
    if (currentBgmIndex != -1)
    {
        mciSendString(L"stop nowplaying", NULL, 0, NULL);
        mciSendString(L"close nowplaying", NULL, 0, NULL);
        currentBgmIndex = -1;
    }
}

/* 功能：按当前状态切歌。 */
void UpdateBGM(Game* g)
{
    int isWin = (g->base.hp > 0 && g->enemiesRemaining == 0);
    if (g == NULL) return;

    if (g->soundOn == 0)
    {
        StopBGM();
        return;
    }

    switch (g->state)
    {
    case STATE_MENU:
    case STATE_LEVEL_SELECT:
        PlayBGM(0, 1);
        break;
    case STATE_PLAYING:
        PlayBGM(g->currentMap + 1, 1);
        break;
    case STATE_RESULT:
        PlayBGM(6, 0);
        break;
    case STATE_ENDLESS:
        PlayBGM(5, 1);
        break;
    case STATE_STORY:
        switch (g->currentMap)
        {
        case 0:isWin ? PlayBGM(8, 1) : PlayBGM(7, 1);break;
        case 1:isWin ? PlayBGM(10, 1) : PlayBGM(9, 1);break;
        case 2:isWin ? PlayBGM(12, 1) : PlayBGM(11, 1);break;
        case 3:isWin ? PlayBGM(14, 1) : PlayBGM(13, 1);break;
        case 4:isWin ? PlayBGM(16, 1) : PlayBGM(15, 1);break;
        }
        break;
    default:
        break;
    }

    BgmKeepPlaying();
}

// ==================== 图鉴界面 ====================

/* 功能：图鉴主界面输入处理。 */
GameState Gallery_update(Game* g, ExMessage* msg) {
    Button btn = { 1000, 10, 250, 80, NULL };
    Button F = { 250, 250, 350, 150, NULL };
    Button E = { 650, 250, 350, 150, NULL };

    if (!Button_isClicked(&btn, msg)) {
        if (Button_isClicked(&F, msg)) return STATE_GALLERY_FRIEND;
        if (Button_isClicked(&E, msg)) return STATE_GALLERY_ENEMY;
        return STATE_GALLERY;
    }
    else if (Button_isClicked(&btn, msg)) {
        return STATE_MENU;
    }
    return STATE_GALLERY;
}

/* 功能：绘制图鉴主界面。 */
void DrawGallery(const Game* g) {
    cleardevice();
    putimage(0, 0, &im_galBg);
    Button btn, F, E;
    Button* btns[3] = { &btn, &F, &E };
    btn = { 1000, 10, 250, 80, L"返回主菜单" };
    Button_draw(&btn);
    F = { 250, 250, 350, 150, L"友方图鉴" };
    Button_draw(&F);
    E = { 650, 250, 350, 150, L"敌方图鉴" };
    Button_draw(&E);
    for (int i = 0; i < 3; i++) {
        if (g->mouseX >= btns[i]->x && g->mouseX <= btns[i]->x + btns[i]->w &&
            g->mouseY >= btns[i]->y && g->mouseY <= btns[i]->y + btns[i]->h) {
            setlinestyle(PS_SOLID, 4);
            setlinecolor(RGB(255, 202, 218));
            rectangle(btns[i]->x, btns[i]->y,
                btns[i]->x + btns[i]->w, btns[i]->y + btns[i]->h);
        }
    }
}

/* 功能：友方图鉴输入处理。 */
GameState GalleryFriend_update(Game* g, ExMessage* msg) {
    Button btn = { 1000, 10, 250, 80, NULL };
    Button Hinata = { 20, 500, 250, 80, NULL };
    Button Chino = { 500, 500, 250, 80, NULL };
    Button Kanna = { 1000, 500, 250, 80, NULL };

    if (!Button_isClicked(&btn, msg)) {
        if (Button_isClicked(&Hinata, msg)) return STATE_HINATA;
        if (Button_isClicked(&Chino, msg))  return STATE_CHINO;
        if (Button_isClicked(&Kanna, msg))  return STATE_KANNA;
        return STATE_GALLERY_FRIEND;
    }
    else if (Button_isClicked(&btn, msg)) {
        return STATE_GALLERY;
    }
    return STATE_GALLERY_FRIEND;
}

/* 功能：绘制友方图鉴。 */
void DrawGalleryFriend(const Game* g) {
    cleardevice();
    putimage(0, 0, &im_Friend);
    Button btn, Hinata, Chino, Kanna;
    Button* btns[4] = { &btn, &Hinata, &Chino, &Kanna };
    btn = { 1000, 10, 250, 80, L"返回" };
    Button_draw(&btn);
    Hinata = { 20, 500, 250, 80, L"日向" };
    Button_draw(&Hinata);
    Chino = { 500, 500, 250, 80, L"智乃" };
    Button_draw(&Chino);
    Kanna = { 1000, 500, 250, 80, L"康娜" };
    Button_draw(&Kanna);
    for (int i = 0; i < 4; i++) {
        if (g->mouseX >= btns[i]->x && g->mouseX <= btns[i]->x + btns[i]->w &&
            g->mouseY >= btns[i]->y && g->mouseY <= btns[i]->y + btns[i]->h) {
            setlinestyle(PS_SOLID, 4);
            setlinecolor(RGB(255, 202, 218));
            rectangle(btns[i]->x, btns[i]->y,
                btns[i]->x + btns[i]->w, btns[i]->y + btns[i]->h);
        }
    }
}

/* 功能：敌方图鉴输入处理。 */
GameState GalleryEnemy_update(Game* g, ExMessage* msg) {
    Button btn = { 1000, 10, 250, 80, NULL };
    Button Yh = { 20, 520, 220, 80, NULL };
    Button Aw = { 340, 520, 220, 80, NULL };
    Button Dd = { 700, 520, 220, 80, NULL };
    Button Boss = { 1020, 520, 220, 80, NULL };

    if (!Button_isClicked(&btn, msg)) {
        if (Button_isClicked(&Yh, msg))   return STATE_YH;
        if (Button_isClicked(&Aw, msg))   return STATE_AW;
        if (Button_isClicked(&Dd, msg))   return STATE_DD;
        if (Button_isClicked(&Boss, msg)) return STATE_01;
        return STATE_GALLERY_ENEMY;
    }
    else if (Button_isClicked(&btn, msg)) {
        return STATE_GALLERY;
    }
    return STATE_GALLERY_ENEMY;
}

/* 功能：绘制敌方图鉴。 */
void DrawGalleryEnemy(const Game* g) {
    cleardevice();
    putimage(0, 0, &im_Enemy);
    Button btn, Yh, Aw, Dd, Boss;
    Button* btns[5] = { &btn, &Yh, &Aw, &Dd, &Boss };
    btn = { 1000, 10, 250, 80, L"返回" };
    Button_draw(&btn);
    Yh = { 20, 520, 220, 80, L"云海学姐" };
    Button_draw(&Yh);
    Aw = { 340, 520, 220, 80, L"阿伟学姐" };
    Button_draw(&Aw);
    Dd = { 700, 520, 220, 80, L"东东姐" };
    Button_draw(&Dd);
    Boss = { 1020, 520, 220, 80, L"[01]" };
    Button_draw(&Boss);
    for (int i = 0; i < 5; i++) {
        if (g->mouseX >= btns[i]->x && g->mouseX <= btns[i]->x + btns[i]->w &&
            g->mouseY >= btns[i]->y && g->mouseY <= btns[i]->y + btns[i]->h) {
            setlinestyle(PS_SOLID, 4);
            setlinecolor(RGB(255, 202, 218));
            rectangle(btns[i]->x, btns[i]->y,
                btns[i]->x + btns[i]->w, btns[i]->y + btns[i]->h);
        }
    }
}

// ==================== 图鉴角色详情 ====================

/* 功能：日向详情页输入处理。 */
GameState GalleryHinata_update(Game* g, ExMessage* msg) {
    Button btn = { 1000, 10, 250, 80, NULL };
    if (!Button_isClicked(&btn, msg)) return STATE_HINATA;
    else if (Button_isClicked(&btn, msg)) return STATE_GALLERY_FRIEND;
    return STATE_HINATA;
}

/* 功能：绘制日向详情页。 */
void DrawGalleryHinata(const Game* g) {
    cleardevice();
    putimage(0, 0, &im_Hinata);
    Button btn = { 1000, 10, 250, 80, L"返回" };
    Button_draw(&btn);
    if (g->mouseX >= btn.x && g->mouseX <= btn.x + btn.w &&
        g->mouseY >= btn.y && g->mouseY <= btn.y + btn.h) {
        setlinestyle(PS_SOLID, 4);
        setlinecolor(RGB(255, 202, 218));
        rectangle(btn.x, btn.y, btn.x + btn.w, btn.y + btn.h);
    }
}

/* 功能：智乃详情页输入处理。 */
GameState GalleryChino_update(Game* g, ExMessage* msg) {
    Button btn = { 1000, 10, 250, 80, NULL };
    if (!Button_isClicked(&btn, msg)) return STATE_CHINO;
    else if (Button_isClicked(&btn, msg)) return STATE_GALLERY_FRIEND;
    return STATE_CHINO;
}

/* 功能：绘制智乃详情页。 */
void DrawGalleryChino(const Game* g) {
    cleardevice();
    putimage(0, 0, &im_Chino);
    Button btn = { 1000, 10, 250, 80, L"返回" };
    Button_draw(&btn);
    if (g->mouseX >= btn.x && g->mouseX <= btn.x + btn.w &&
        g->mouseY >= btn.y && g->mouseY <= btn.y + btn.h) {
        setlinestyle(PS_SOLID, 4);
        setlinecolor(RGB(255, 202, 218));
        rectangle(btn.x, btn.y, btn.x + btn.w, btn.y + btn.h);
    }
}

/* 功能：康娜详情页输入处理。 */
GameState GalleryKanna_update(Game* g, ExMessage* msg) {
    Button btn = { 1000, 10, 250, 80, NULL };
    if (!Button_isClicked(&btn, msg)) return STATE_KANNA;
    else if (Button_isClicked(&btn, msg)) return STATE_GALLERY_FRIEND;
    return STATE_KANNA;
}

/* 功能：绘制康娜详情页。 */
void DrawGalleryKanna(const Game* g) {
    cleardevice();
    putimage(0, 0, &im_Kanna);
    Button btn = { 1000, 10, 250, 80, L"返回" };
    Button_draw(&btn);
    if (g->mouseX >= btn.x && g->mouseX <= btn.x + btn.w &&
        g->mouseY >= btn.y && g->mouseY <= btn.y + btn.h) {
        setlinestyle(PS_SOLID, 4);
        setlinecolor(RGB(255, 202, 218));
        rectangle(btn.x, btn.y, btn.x + btn.w, btn.y + btn.h);
    }
}

/* 功能：Yh 详情页输入处理。 */
GameState GalleryYh_update(Game* g, ExMessage* msg) {
    Button btn = { 1000, 10, 250, 80, NULL };
    if (!Button_isClicked(&btn, msg)) return STATE_YH;
    else if (Button_isClicked(&btn, msg)) return STATE_GALLERY_ENEMY;
    return STATE_YH;
}

/* 功能：绘制 Yh 详情页。 */
void DrawGalleryYh(const Game* g) {
    cleardevice();
    putimage(0, 0, &im_Yh);
    Button btn = { 1000, 10, 250, 80, L"返回" };
    Button_draw(&btn);
    if (g->mouseX >= btn.x && g->mouseX <= btn.x + btn.w &&
        g->mouseY >= btn.y && g->mouseY <= btn.y + btn.h) {
        setlinestyle(PS_SOLID, 4);
        setlinecolor(RGB(255, 202, 218));
        rectangle(btn.x, btn.y, btn.x + btn.w, btn.y + btn.h);
    }
}

/* 功能：Aw 详情页输入处理。 */
GameState GalleryAw_update(Game* g, ExMessage* msg) {
    Button btn = { 1000, 10, 250, 80, NULL };
    if (!Button_isClicked(&btn, msg)) return STATE_AW;
    else if (Button_isClicked(&btn, msg)) return STATE_GALLERY_ENEMY;
    return STATE_AW;
}

/* 功能：绘制 Aw 详情页。 */
void DrawGalleryAw(const Game* g) {
    cleardevice();
    putimage(0, 0, &im_Aw);
    Button btn = { 1000, 10, 250, 80, L"返回" };
    Button_draw(&btn);
    if (g->mouseX >= btn.x && g->mouseX <= btn.x + btn.w &&
        g->mouseY >= btn.y && g->mouseY <= btn.y + btn.h) {
        setlinestyle(PS_SOLID, 4);
        setlinecolor(RGB(255, 202, 218));
        rectangle(btn.x, btn.y, btn.x + btn.w, btn.y + btn.h);
    }
}

/* 功能：Dd 详情页输入处理。 */
GameState GalleryDd_update(Game* g, ExMessage* msg) {
    Button btn = { 1000, 10, 250, 80, NULL };
    if (!Button_isClicked(&btn, msg)) return STATE_DD;
    else if (Button_isClicked(&btn, msg)) return STATE_GALLERY_ENEMY;
    return STATE_DD;
}

/* 功能：绘制 Dd 详情页。 */
void DrawGalleryDd(const Game* g) {
    cleardevice();
    putimage(0, 0, &im_Dd);
    Button btn = { 1000, 10, 250, 80, L"返回" };
    Button_draw(&btn);
    if (g->mouseX >= btn.x && g->mouseX <= btn.x + btn.w &&
        g->mouseY >= btn.y && g->mouseY <= btn.y + btn.h) {
        setlinestyle(PS_SOLID, 4);
        setlinecolor(RGB(255, 202, 218));
        rectangle(btn.x, btn.y, btn.x + btn.w, btn.y + btn.h);
    }
}

/* 功能：[01] 详情页输入处理。 */
GameState Gallery01_update(Game* g, ExMessage* msg) {
    Button btn = { 1000, 10, 250, 80, NULL };
    if (!Button_isClicked(&btn, msg)) return STATE_01;
    else if (Button_isClicked(&btn, msg)) return STATE_GALLERY_ENEMY;
    return STATE_01;
}

/* 功能：绘制 [01] 详情页。 */
void DrawGallery01(const Game* g) {
    cleardevice();
    putimage(0, 0, &im_01);
    Button btn = { 1000, 10, 250, 80, L"返回" };
    Button_draw(&btn);
    if (g->mouseX >= btn.x && g->mouseX <= btn.x + btn.w &&
        g->mouseY >= btn.y && g->mouseY <= btn.y + btn.h) {
        setlinestyle(PS_SOLID, 4);
        setlinecolor(RGB(255, 202, 218));
        rectangle(btn.x, btn.y, btn.x + btn.w, btn.y + btn.h);
    }
}

/* 功能：绘制游戏内 UI（右上信息栏 + 基地血条）。 */
void DrawUI(const Game* g) {
    TCHAR buf[64];

    setbkmode(TRANSPARENT);
    settextcolor(RGB(80, 40, 60));
    settextstyle(18, 0, _T("微软雅黑"));

    _stprintf_s(buf, 64, _T("金币: %d"), g->money);
    outtextxy(1165, 30, buf);

    _stprintf_s(buf, 64, _T("波次: %d/%d"), g->waveIndex + 1, g->totalWaves);
    outtextxy(1165, 70, buf);

    _stprintf_s(buf, 64, _T("剩余: %d"), g->enemiesRemaining);
    outtextxy(1165, 110, buf);

    _stprintf_s(buf, 64, _T("关卡: %d"), g->currentMap + 1);
    outtextxy(1165, 150, buf);

    int hpBarW = 100;
    int hpBarH = 12;
    int hpBarX = g->base.x - hpBarW / 2;
    int hpBarY = g->base.y + g->base.radius + 8;

    setfillcolor(RGB(80, 80, 80));
    solidrectangle(hpBarX, hpBarY, hpBarX + hpBarW, hpBarY + hpBarH);

    if (g->base.maxHp > 0 && g->base.hp > 0) {
        int hpW = (int)((float)g->base.hp / g->base.maxHp * hpBarW);
        if (hpW > hpBarW) hpW = hpBarW;

        if (g->base.hp > g->base.maxHp / 2) {
            setfillcolor(RGB(100, 220, 100));
        }
        else {
            setfillcolor(RGB(230, 60, 60));
        }
        solidrectangle(hpBarX, hpBarY, hpBarX + hpW, hpBarY + hpBarH);
    }

    setlinecolor(RGB(40, 40, 40));
    setlinestyle(PS_SOLID, 1);
    rectangle(hpBarX, hpBarY, hpBarX + hpBarW, hpBarY + hpBarH);

    settextcolor(RGB(255, 255, 255));
    settextstyle(12, 0, _T("微软雅黑"));
    _stprintf_s(buf, 64, _T("%d/%d"), g->base.hp, g->base.maxHp);
    int tw = textwidth(buf);
    int th = textheight(buf);
    outtextxy(hpBarX + (hpBarW - tw) / 2, hpBarY + (hpBarH - th) / 2, buf);
}

/* 功能：无尽模式输入处理。 */
GameState EndlessView_update(Game* g, ExMessage* msg) {
    if (!g || !msg) return STATE_ENDLESS;

    if (msg->message == WM_KEYDOWN) {
        if (msg->vkcode == VK_SPACE)  return STATE_PAUSED;
        if (msg->vkcode == VK_ESCAPE) { g->isEndlessMode = 0; return STATE_MENU; }
        return STATE_ENDLESS;
    }

    if (msg->message == WM_LBUTTONDOWN) {

        if (msg->x > 0 && msg->x < 100 && msg->y > 50 && msg->y < 150) {
            return STATE_PAUSED;
        }

        if (msg->x > 1150 && msg->x < 1280 && msg->y > 200 && msg->y < 300) {
            if (IsTowerUnlocked(g, Hinata))
                g->selectedTowerType = (g->selectedTowerType == Hinata) ? -1 : Hinata;
            return STATE_ENDLESS;
        }
        else if (msg->x > 1150 && msg->x < 1280 && msg->y > 300 && msg->y < 400) {
            if (IsTowerUnlocked(g, Chino))
                g->selectedTowerType = (g->selectedTowerType == Chino) ? -1 : Chino;
            return STATE_ENDLESS;
        }
        else if (msg->x > 1150 && msg->x < 1280 && msg->y > 400 && msg->y < 500) {
            if (IsTowerUnlocked(g, Kanna))
                g->selectedTowerType = (g->selectedTowerType == Kanna) ? -1 : Kanna;
            return STATE_ENDLESS;
        }

        if (g->selectedTowerType != -1) {
            Point p = pixelToGrid(msg->x, msg->y);
            if (p.x == -1) return STATE_ENDLESS;
            if (CanPlaceAt(g, p.x, p.y)) {
                PlaceTower(g, p.x, p.y, g->selectedTowerType);
            }
        }
        return STATE_ENDLESS;
    }

    if (msg->message == WM_RBUTTONDOWN) {
        Point p = pixelToGrid(msg->x, msg->y);
        if (p.x == -1) return STATE_ENDLESS;
        RemoveTower(g, p.x, p.y);
        return STATE_ENDLESS;
    }
    return STATE_ENDLESS;
}

/* 功能：绘制无尽模式界面。 */
void DrawEndless(const Game* g) {
    cleardevice();
    putimage(0, 0, &im_mapBg);
    putimage(1150, 0, &Show_info);

    // 已建塔
    for (int i = 0; i < g->towerCount; i++) {
        const Tower* t = &g->towers[i];
        if (!t->placed) continue;
        int px = t->x - TILE_SIZE_X / 2;
        int py = t->y - TILE_SIZE_Y / 2;
        int rx = t->x;
        int ry = t->y;
        bool attacking = false;
        if (t->attackInterval > 0.0f) {
            float threshold = t->attackInterval * 0.7f;
            if (t->cooldown > threshold) attacking = true;
        }
        switch (t->type) {
        case Hinata:
            if (attacking) {
                setlinecolor(RGB(255, 228, 235));
                setlinestyle(PS_SOLID, 2);
                circle(rx, ry, g->towerConfigs[Hinata].rangeGrid * TILE_SIZE_X / 2);
                setlinestyle(PS_SOLID, 1);
                putimage(px, py, &im_towerHinata_atk_mask, SRCAND);
                putimage(px, py, &im_towerHinata_atk_color, SRCPAINT);
            }
            else {
                putimage(px, py, &im_towerHinata_mask, SRCAND);
                putimage(px, py, &im_towerHinata_color, SRCPAINT);
            }
            break;
        case Chino:
            if (attacking) {
                putimage(px, py, &im_towerChino_atk_mask, SRCAND);
                putimage(px, py, &im_towerChino_atk_color, SRCPAINT);
            }
            else {
                putimage(px, py, &im_towerChino_mask, SRCAND);
                putimage(px, py, &im_towerChino_color, SRCPAINT);
            }
            break;
        case Kanna:
            if (attacking) {
                setlinecolor(RGB(255, 192, 203));
                setlinestyle(PS_SOLID, 3);
                circle(rx, ry, g->towerConfigs[Kanna].rangeGrid * TILE_SIZE_X / 2);
                putimage(px, py, &im_towerKanna_atk_mask, SRCAND);
                putimage(px, py, &im_towerKanna_atk_color, SRCPAINT);
            }
            else {
                putimage(px, py, &im_towerKanna_mask, SRCAND);
                putimage(px, py, &im_towerKanna_color, SRCPAINT);
            }
            break;
        }
    }

    // 敌人 + 血条
    for (int i = 0; i < g->enemyCount; i++) {
        const Enemy* e = &g->enemies[i];
        if (!e->alive) continue;
        int px = (int)e->x - TILE_SIZE_X / 2;
        int py = (int)e->y - TILE_SIZE_Y / 2;
        if (e->isBoss) {
            putimage(px, py, &im_boss_mask, SRCAND);
            putimage(px, py, &im_boss_color, SRCPAINT);
        }
        else {
            switch (e->type) {
            case 0: putimage(px, py, &im_enemyYh_mask, SRCAND); putimage(px, py, &im_enemyYh_color, SRCPAINT); break;
            case 1: putimage(px, py, &im_enemyAw_mask, SRCAND); putimage(px, py, &im_enemyAw_color, SRCPAINT); break;
            case 2: putimage(px, py, &im_enemyDd_mask, SRCAND); putimage(px, py, &im_enemyDd_color, SRCPAINT); break;
            }
        }
        int barW = TILE_SIZE_X - 10;
        int barH = 4;
        int barX = (int)e->x - barW / 2;
        int barY = py - 8;
        setfillcolor(RGB(60, 60, 60));
        solidrectangle(barX, barY, barX + barW, barY + barH);
        if (e->maxHp > 0 && e->hp > 0) {
            int hpW = (int)((float)e->hp / e->maxHp * barW);
            if (hpW > barW) hpW = barW;
            setfillcolor(RGB(230, 60, 60));
            solidrectangle(barX, barY, barX + hpW, barY + barH);
        }
    }

    if (g->baseFlashTimer > 0.0f) {
        setlinecolor(RGB(255, 0, 0));
        setlinestyle(PS_SOLID, 4);
        circle(g->base.x, g->base.y, g->base.radius + 4);
        setlinestyle(PS_SOLID, 1);
    }

    // UI 文字
    TCHAR buf[64];
    setbkmode(TRANSPARENT);
    settextcolor(RGB(80, 40, 60));
    settextstyle(18, 0, _T("微软雅黑"));

    _stprintf_s(buf, 64, _T("金币: %d"), g->money);
    outtextxy(1165, 30, buf);
    _stprintf_s(buf, 64, _T("波数: %d"), g->endlessWave);
    outtextxy(1165, 70, buf);
    _stprintf_s(buf, 64, _T("最高: %d"), g->endlessBestWave);
    outtextxy(1165, 110, buf);

    //基地血条
    int hpBarW = 100, hpBarH = 12;
    int hpBarX = g->base.x - hpBarW+50;
    int hpBarY = g->base.y + g->base.radius + 8;
    setfillcolor(RGB(80, 80, 80));
    solidrectangle(hpBarX, hpBarY, hpBarX + hpBarW, hpBarY + hpBarH);
    if (g->base.maxHp > 0 && g->base.hp > 0) {
        int hpW = (int)((float)g->base.hp / g->base.maxHp * hpBarW);
        setfillcolor(g->base.hp > g->base.maxHp / 2 ? RGB(100, 220, 100) : RGB(230, 60, 60));
        solidrectangle(hpBarX, hpBarY, hpBarX + hpW, hpBarY + hpBarH);
    }

    putimage(0, 50, &Bt_pause);
    if (IsTowerUnlocked(g, Hinata)) putimage(1150, 200, &Bt_Hinata);
    if (IsTowerUnlocked(g, Chino))  putimage(1150, 300, &Bt_Chino);
    if (IsTowerUnlocked(g, Kanna))  putimage(1150, 400, &Bt_Kanna);

    if (g->selectedTowerType == Hinata) {
        setlinestyle(PS_SOLID, 4);
        setlinecolor(RED);
        rectangle(1150, 200, 1280, 300);
    }
    else if (g->selectedTowerType == Chino) {
        setlinestyle(PS_SOLID, 4);
        setlinecolor(RED);
        rectangle(1150, 300, 1280, 400);
    }
    else if (g->selectedTowerType == Kanna) {
        setlinestyle(PS_SOLID, 4);
        setlinecolor(RED);
        rectangle(1150, 400, 1280, 500);
    }
}