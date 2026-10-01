#ifndef HASHSET_H
#define HASHSET_H

/* 保持 C 链接约定，详见 common.h 里的说明 */
#ifdef __cplusplus
extern "C" {
#endif

/*
 * hashset.h —— 哈希集合（链地址法解决冲突），用于 O(1) 碰撞检测
 *
 * 【为什么用它】
 * 判断"蛇头的新位置是否撞到自己身上"，本质是集合的成员查询。
 *   朴素做法：遍历整条蛇逐个比较，O(n)，蛇长 1000 时每帧就是 1000 次比较。
 *   哈希集合：把格子编号直接哈希，平均 O(1)。
 *
 * 【答辩重点：哈希函数与冲突处理】
 * 请自己设计哈希函数，不要直接 return key % capacity 了事。
 * 哈希函数的构造、容量的选取、冲突处理的策略，是这份作业里最值得展开写的部分，
 * 也是老师在答辩时最可能追问的地方。
 *
 * 【统计接口】
 * 下面那组 hs_stats 是专门为对比实验准备的：它会累计"探测次数"，
 * 也就是朴素版口径下的"比较次数"。用它跑报告里的效率对比表。
 */

#include "common.h"

typedef struct HashSet HashSet;

/* 运行期统计量，供 bench.c 输出对比数据 */
typedef struct {
    unsigned long long lookups;      /* hs_contains 被调用的次数 */
    unsigned long long probes;       /* 累计探测到的桶/链表节点比较次数 */
    unsigned long long collisions;   /* 插入时发生冲突（目标桶非空）的次数 */
    unsigned long long rehashes;     /* 扩容次数 */
} HashSetStats;

/* ---------- 生命周期 ---------- */
/*
 * capacity：初始桶数。建议给质数，或给 2 的幂并在哈希函数里做位运算。
 * 内部会在装载因子过高时自动扩容（并计入 stats.rehashes）。
 */
HashSet *hs_create(int capacity);            /* 失败返回 NULL */
void     hs_destroy(HashSet *hs);

/* ---------- 基本操作 ---------- */
int hs_insert(HashSet *hs, Pos key);         /* OK / ERR_DUPLICATE / ERR */
int hs_erase(HashSet *hs, Pos key);          /* OK / ERR_NOTFOUND */
int hs_contains(HashSet *hs, Pos key);       /* 1 存在，0 不存在 */

/* ---------- 状态查询 ---------- */
int    hs_size(const HashSet *hs);           /* 元素个数 */
double hs_load_factor(const HashSet *hs);    /* 装载因子 = 元素数 / 桶数 */
void   hs_clear(HashSet *hs);                /* 清空元素但保留已分配的桶 */

/* ---------- 统计（供对比实验用） ---------- */
/*
 * 注意：hs_contains 会更新统计量，所以这两个函数接收非 const 指针。
 * 这是为了方便实验统计而做的妥协，报告里可以提一句。
 */
void         hs_reset_stats(HashSet *hs);
HashSetStats hs_stats(const HashSet *hs);

/* ---------- 暴露哈希函数（供报告分析与单元测试单独调用） ---------- */
unsigned int hs_hash(Pos key);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif /* HASHSET_H */
