#ifndef DEQUE_H
#define DEQUE_H

/* 保持 C 链接约定，详见 common.h 里的说明 */
#ifdef __cplusplus
extern "C" {
#endif

/*
 * deque.h —— 双端队列（双向链表实现），用于存放蛇身
 *
 * 【为什么用它】
 * 贪吃蛇每走一步只有两个动作：在头部插入一节、在尾部删除一节。
 *   数组实现：头部插入要整体搬移，O(n)；或改用环形数组，但要固定长度上限。
 *   双向链表：头尾插入/删除都是 O(1)，且长度不设上限。
 *
 * 【朴素版对照】
 * deque_contains() 是 O(n) 的线性查找，正是"朴素碰撞检测"的实现方式。
 * 阶段 2 里请保留它，与 hashset 的 O(1) 方案做对比实验，用它出报告数据。
 *
 * 【顺带复用】
 * 撤销功能可以直接用 push_back / pop_back 把它当栈用，不必再写一个 Stack 结构。
 */

#include "common.h"

typedef struct Deque Deque;

/* 遍历回调：value 为元素值，index 为从队头数起的序号（0 = 蛇头） */
typedef void (*DequeVisitFn)(Pos value, int index, void *ctx);

/* ---------- 生命周期 ---------- */
Deque *deque_create(void);                   /* 成功返回句柄，失败返回 NULL */
void   deque_destroy(Deque *dq);             /* 传 NULL 安全 */

/* ---------- 状态查询 ---------- */
int deque_empty(const Deque *dq);            /* 空返回 1，否则 0；dq 为 NULL 视为空 */
int deque_size(const Deque *dq);             /* 元素个数；dq 为 NULL 返回 0 */

/* ---------- 插入（返回 OK / ERR） ---------- */
int deque_push_front(Deque *dq, Pos v);      /* 头部插入 —— 蛇前进时用 */
int deque_push_back(Deque *dq, Pos v);       /* 尾部插入 —— 撤销回退、当栈用时用 */

/* ---------- 删除（返回 OK / ERR_EMPTY） ---------- */
/* 删除的元素通过出参 out 返回；out 允许传 NULL，表示不关心被删的值 */
int deque_pop_front(Deque *dq, Pos *out);
int deque_pop_back(Deque *dq, Pos *out);

/* ---------- 读取端点（只读，不删除）---------- */
int deque_front(const Deque *dq, Pos *out);  /* 取蛇头 */
int deque_back(const Deque *dq, Pos *out);   /* 取蛇尾 */

/* ---------- 查找 ---------- */
/*
 * 线性查找，最坏 O(n)。
 * 这是"朴素版碰撞检测"的接口：判断蛇头新位置是否撞到自己的身体。
 * 保留它是为了与 hashset 版本做实验对比，不是最终方案。
 */
int deque_contains(const Deque *dq, Pos v);  /* 含返回 1，不含返回 0 */

/* ---------- 遍历（O(n)，供渲染层使用）---------- */
/* 渲染层通过它读取整条蛇，无需了解内部是链表还是数组 */
void deque_foreach(const Deque *dq, DequeVisitFn fn, void *ctx);

/* ---------- 清空 ---------- */
void deque_clear(Deque *dq);                 /* 元素全部释放，队列变为空 */

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif /* DEQUE_H */
