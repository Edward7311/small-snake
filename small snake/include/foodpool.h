#ifndef FOODPOOL_H
#define FOODPOOL_H

/* 保持 C 链接约定，详见 common.h 里的说明 */
#ifdef __cplusplus
extern "C" {
#endif

/*
 * foodpool.h —— 空闲格集合，用于 O(1) 生成食物
 *
 * 【为什么用它】
 * 朴素做法：随机取一个格子，如果上面有蛇就重试。
 *   蛇长占地图 10% 时平均试 1.1 次，没什么感觉；
 *   蛇长占 90% 时平均要试 10 次，占 99% 时要试 100 次 —— 这是长蛇卡顿的根源。
 *
 * 【本模块的实现思路（数组 + 下标互指 + 交换删除）】
 *   1. cells[]  ：把所有"当前空闲"的格子编号紧凑地存在数组前 count 个位置里。
 *   2. slot[]   ：slot[格子编号] = 该格子在 cells[] 中的下标（不在则为 -1）。
 *   3. 取食物   ：在 [0, count) 里随机取一个下标，O(1)，永不重试。
 *   4. 移除格子 ：取出 cells[slot[p]] 后，把数组末尾元素换到该位置，O(1)。
 *   5. 归还格子 ：追加到数组末尾，O(1)。
 * 三个数组都是长度 POS_COUNT（1600）的一维数组，空间开销极小。
 *
 * 【备选方案】
 * 也可以用并查集维护"连续空闲区间"，思路不同但同样正确；
 * 若组内有同学第一部分实现的是并查集，可以作为替代实现写进报告做对比。
 *
 * 【用法约定】
 *   蛇吃掉食物时 → 该格变成蛇身 → fp_remove()
 *   蛇尾离开某格时 → 该格恢复空闲 → fp_add()
 *   开局/重开      → fp_reset()
 * 调用方必须维护好这组对应关系，否则空闲集会与实际地图失配。
 */

#include "common.h"

typedef struct FoodPool FoodPool;

/* ---------- 生命周期 ---------- */
FoodPool *fp_create(void);                   /* 失败返回 NULL */
void      fp_destroy(FoodPool *fp);

/* ---------- 维护空闲集 ---------- */
void fp_reset(FoodPool *fp);                 /* 重置为"全部格子空闲"（开局/重开用） */
int  fp_remove(FoodPool *fp, Pos cell);      /* 该格被占据，从空闲集移除；OK / ERR_NOTFOUND */
int  fp_add(FoodPool *fp, Pos cell);         /* 该格恢复空闲，归还到空闲集；OK / ERR_DUPLICATE */

/* ---------- 查询与取用 ---------- */
int fp_count(const FoodPool *fp);            /* 当前空闲格总数 */
int fp_is_free(FoodPool *fp, Pos cell);      /* 1 空闲，0 已被占 */

/*
 * 随机取一个空闲格，O(1)，不会失败也不需要重试。
 * 结果通过出参 out 返回。
 * 返回 OK；若已无空格（地图被蛇填满），返回 ERR_EMPTY。
 */
int fp_pick_random(FoodPool *fp, Pos *out);

/* ---------- 统计（供对比实验用） ---------- */
/* 累计的"随机取格尝试次数"，用来和朴素版的 retry 次数做对照 */
unsigned long long fp_pick_attempts(const FoodPool *fp);
void               fp_reset_stats(FoodPool *fp);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif /* FOODPOOL_H */
