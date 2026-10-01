#include "foodpool.h"

#include <stdlib.h>

/*
 * foodpool.c —— 空闲格集合（数组 + 下标互指 + 交换删除）
 *
 * 三个操作全部 O(1)：
 *   fp_remove  交换删除，把末尾元素填到被删坑里
 *   fp_add     追加到 cells 末尾
 *   fp_pick_random  在 [0, count) 里随机取下标，永不重试
 *
 * 空间：两个长度为 POS_COUNT 的数组，与蛇长无关。
 */

struct FoodPool {
    int *cells;                /* 前 count 个位置存放当前空闲的格子编号 */
    int *slot;                 /* slot[格子编号] = 它在 cells 中的下标；不在则为 -1 */
    int  count;                /* 当前空闲格数量 */
    unsigned long long attempts; /* 累计取格次数，供对比实验使用 */
};

/* ---------------- 生命周期 ---------------- */

/* 分配 cells / slot，失败返回 NULL */
FoodPool *fp_create(void)
{
    FoodPool *fp = (FoodPool *)malloc(sizeof(FoodPool));
    if (fp == NULL) {
        return NULL;
    }

    fp->cells = (int *)malloc(sizeof(int) * POS_COUNT);
    fp->slot  = (int *)malloc(sizeof(int) * POS_COUNT);
    if (fp->cells == NULL || fp->slot == NULL) {
        free(fp->cells);
        free(fp->slot);
        free(fp);
        return NULL;
    }

    fp_reset(fp);
    return fp;
}

/* 释放两个数组与结构体；传入 NULL 安全 */
void fp_destroy(FoodPool *fp)
{
    if (fp == NULL) {
        return;
    }
    free(fp->cells);
    free(fp->slot);
    free(fp);
}

/* ---------------- 维护空闲集 ---------------- */

/* 重置为全部格子空闲，attempts 清零 */
void fp_reset(FoodPool *fp)
{
    int i;

    if (fp == NULL) {
        return;
    }

    for (i = 0; i < POS_COUNT; i++) {
        fp->cells[i] = i;
        fp->slot[i]  = i;
    }
    fp->count     = POS_COUNT;
    fp->attempts  = 0;
}

/* 该格被占据，从空闲集移除；不空闲则 ERR_NOTFOUND */
int fp_remove(FoodPool *fp, Pos cell)
{
    int i;
    int last;

    if (fp == NULL || !POS_VALID(cell)) {
        return ERR_NOTFOUND;
    }
    if (fp->slot[cell] < 0) {
        return ERR_NOTFOUND;
    }

    i    = fp->slot[cell];
    last = fp->cells[fp->count - 1];
    fp->cells[i]  = last;
    fp->slot[last] = i;
    fp->count--;
    fp->slot[cell] = -1;
    return OK;
}

/* 该格恢复空闲；已空闲则 ERR_DUPLICATE */
int fp_add(FoodPool *fp, Pos cell)
{
    if (fp == NULL || !POS_VALID(cell)) {
        return ERR;
    }
    if (fp->slot[cell] >= 0) {
        return ERR_DUPLICATE;
    }

    fp->cells[fp->count] = cell;
    fp->slot[cell]       = fp->count;
    fp->count++;
    return OK;
}

/* ---------------- 查询与取用 ---------------- */

/* 返回当前空闲格总数；fp 为 NULL 返回 0 */
int fp_count(const FoodPool *fp)
{
    return (fp == NULL) ? 0 : fp->count;
}

/* 空闲返回 1；越界或已被占返回 0 */
int fp_is_free(FoodPool *fp, Pos cell)
{
    if (fp == NULL || !POS_VALID(cell)) {
        return 0;
    }
    return (fp->slot[cell] >= 0) ? 1 : 0;
}

/* 从空闲集随机取一格，O(1)；attempts 加一 */
int fp_pick_random(FoodPool *fp, Pos *out)
{
    int i;

    if (fp == NULL || out == NULL) {
        return ERR;
    }
    if (fp->count <= 0) {
        return ERR_EMPTY;
    }

    fp->attempts++;
    i = rand() % fp->count;
    *out = fp->cells[i];
    return OK;
}

/* ---------------- 统计 ---------------- */

unsigned long long fp_pick_attempts(const FoodPool *fp)
{
    return (fp == NULL) ? 0ULL : fp->attempts;
}

void fp_reset_stats(FoodPool *fp)
{
    if (fp == NULL) {
        return;
    }
    fp->attempts = 0;
}
