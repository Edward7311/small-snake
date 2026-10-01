#ifndef GAME_H
#define GAME_H

/* 保持 C 链接约定，详见 common.h 里的说明 */
#ifdef __cplusplus
extern "C" {
#endif

/*
 * game.h —— 游戏逻辑层
 *
 * 【重要约定】本文件以及 game.c 里不允许出现任何绘图相关的代码，
 * 不允许 include graphics.h。逻辑层只负责改数据，渲染层只负责读数据画图。
 * 这样做的收益有三个：
 *   1. bench.c 能在不开窗口的情况下跑几十万步，输出对比数据；
 *   2. 将来把 EasyX 换成 Qt，只需要重写 render.c；
 *   3. 逻辑层可以脱离图形环境做单元测试。
 *
 * 【关于数据结构的选择】
 *   碰撞检测 —— 哈希集合 hs_contains，平均 O(1)
 *                （朴素做法是遍历蛇身线性查找 O(n)，对照实现留在 bench.c 里）
 *   食物生成 —— 空闲格集合 fp_pick_random，O(1)，永不重试
 *                （朴素做法是"随机试到不撞为止"，对照实现留在 bench.c 里）
 *   蛇身     —— 双端队列（双向链表），头插尾删均 O(1)
 *
 * 两处朴素实现已从游戏逻辑中移除，只保留在 bench.c 中作为对比基准。
 */

#include "common.h"

/* ---------- 枚举 ---------- */
typedef enum {
    GS_READY = 0,      /* 等待开始 */
    GS_RUNNING,        /* 运行中 */
    GS_PAUSED,         /* 暂停 */
    GS_OVER            /* 游戏结束 */
} GameState;

typedef enum {
    DIR_UP = 0,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Direction;

/* 本局累计统计量，供 UI 展示与实验对照 */
typedef struct {
    unsigned long long collision_checks;  /* 累计碰撞比较次数（哈希表探测次数） */
    unsigned long long steps;             /* 累计步数 */
} GameStats;

typedef struct Game Game;

/* ---------- 生命周期 ---------- */
Game *game_create(void);                 /* 失败返回 NULL */
void  game_destroy(Game *g);
void  game_reset(Game *g);               /* 重开一局，统计量清零，状态回到 GS_READY */

/* ---------- 推进与操作 ---------- */

/*
 * 前进一步：蛇头按当前方向移动一格，判断吃食物/撞墙/撞自己。
 * 返回 OK 表示正常推进，返回 ERR 表示本步导致游戏结束（状态转为 GS_OVER）。
 * 调用方负责控制调用频率（见 main.c 的固定步进间隔），本函数不自带计时。
 */
int game_update(Game *g);

/*
 * 设置前进方向。
 * 若与当前方向相反（例如正在向上却按了下），直接忽略并返回 ERR，无副作用。
 * 这是防止"蛇原地掉头撞到自己脖子"的关键，务必在实现里处理好。
 */
int game_set_direction(Game *g, Direction d);

/*
 * 切换暂停/继续：GS_RUNNING <-> GS_PAUSED。
 * 返回切换后的状态；在其他状态下调用返回当前状态，不做改变。
 */
GameState game_toggle_pause(Game *g);

int game_start(Game *g);                 /* GS_READY -> GS_RUNNING */

/* ---------- 只读查询接口（供渲染层使用）---------- */
/* 这一组都不改变游戏状态，渲染层只能通过它们读数据，不能直接碰内部结构 */

GameState game_state(const Game *g);
int       game_score(const Game *g);
int       game_snake_len(const Game *g);

/*
 * 读取蛇身第 index 节（0 = 蛇头）的格子编号。
 * 注意：这个函数每次调用都要从头遍历链表，本身是 O(n)。
 * 它适合"偶尔取一节"的场景（比如取蛇头）；如果你要遍历整条蛇，
 * 请改用下面的 game_foreach_snake，否则会写成 O(n^2)：
 * 蛇长 1000 时，每帧 1000 次调用 = 100 万次遍历，白白浪费。
 */
int game_snake_cell(const Game *g, int index, Pos *out);

/* 蛇身遍历回调：cell 为格子编号，index 从 0（蛇头）递增 */
typedef void (*SnakeVisitFn)(Pos cell, int index, void *ctx);

/*
 * 一次遍历整条蛇，逐个回调，整体 O(n)。
 * 渲染层画蛇请用这个，不要用 game_snake_cell 循环。
 */
void game_foreach_snake(const Game *g, SnakeVisitFn fn, void *ctx);

int game_food(const Game *g, Pos *out);   /* 读取当前食物位置 */

/* ---------- 统计接口（供 UI 展示与实验对照使用）---------- */
GameStats game_stats(const Game *g);
void      game_reset_stats(Game *g);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif /* GAME_H */
