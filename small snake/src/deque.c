#include "deque.h"

#include <stdlib.h>

/*
 * deque.c —— 双端队列（双向链表实现）
 *
 * 这是全项目的代码风格参考样本，其余模块请照这个格式写：
 *   1. 文件头注释说明"为什么这么实现"和复杂度
 *   2. 每个函数上面一行注释说明参数含义与返回值的可能取值
 *   3. 边界情况（空队列、NULL 指针）一律显式处理，不做隐式假设
 *   4. 内存分配失败必须回传错误，不要直接崩
 *
 * 复杂度：push_front / push_back / pop_front / pop_back 全部 O(1)
 *         contains / foreach 为 O(n)
 *         每个节点额外一个指针的开销：2 * sizeof(void*)
 */

/* 链表节点：存一个格子编号，加前后指针 */
typedef struct DNode {
    Pos           value;
    struct DNode *prev;
    struct DNode *next;
} DNode;

struct Deque {
    DNode *head;   /* 队头 —— 蛇头 */
    DNode *tail;   /* 队尾 —— 蛇尾 */
    int    size;   /* 元素个数，避免每次求长度都遍历一遍 */
};

/* ---------------- 内部辅助 ---------------- */

/* 分配一个节点，失败返回 NULL */
static DNode *node_new(Pos value)
{
    DNode *n = (DNode *)malloc(sizeof(DNode));
    if (n == NULL) {
        return NULL;
    }
    n->value = value;
    n->prev  = NULL;
    n->next  = NULL;
    return n;
}

/* 释放整条链上的所有节点，不释放队列结构本身 */
static void free_all_nodes(Deque *dq)
{
    DNode *cur = dq->head;
    while (cur != NULL) {
        DNode *next = cur->next;   /* 先存下后继再释放当前节点 */
        free(cur);
        cur = next;
    }
    dq->head = NULL;
    dq->tail = NULL;
    dq->size = 0;
}

/* ---------------- 生命周期 ---------------- */

/* 创建空队列；失败返回 NULL */
Deque *deque_create(void)
{
    Deque *dq = (Deque *)malloc(sizeof(Deque));
    if (dq == NULL) {
        return NULL;
    }
    dq->head = NULL;
    dq->tail = NULL;
    dq->size = 0;
    return dq;
}

/* 销毁队列；传入 NULL 是安全的 */
void deque_destroy(Deque *dq)
{
    if (dq == NULL) {
        return;
    }
    free_all_nodes(dq);
    free(dq);
}

/* ---------------- 状态查询 ---------------- */

/* 空返回 1；dq 为 NULL 也视为空 */
int deque_empty(const Deque *dq)
{
    return (dq == NULL || dq->size == 0) ? 1 : 0;
}

/* 返回元素个数；dq 为 NULL 返回 0 */
int deque_size(const Deque *dq)
{
    return (dq == NULL) ? 0 : dq->size;
}

/* ---------------- 插入 ---------------- */

/* 头部插入；成功返回 OK，分配失败返回 ERR */
int deque_push_front(Deque *dq, Pos v)
{
    DNode *n;

    if (dq == NULL) {
        return ERR;
    }

    n = node_new(v);
    if (n == NULL) {
        return ERR;
    }

    n->next = dq->head;
    n->prev = NULL;

    if (dq->head != NULL) {
        dq->head->prev = n;
    }
    dq->head = n;

    if (dq->tail == NULL) {   /* 原本是空队列，新节点同时是队尾 */
        dq->tail = n;
    }

    dq->size++;
    return OK;
}

/* 尾部插入；成功返回 OK，分配失败返回 ERR */
int deque_push_back(Deque *dq, Pos v)
{
    DNode *n;

    if (dq == NULL) {
        return ERR;
    }

    n = node_new(v);
    if (n == NULL) {
        return ERR;
    }

    n->prev = dq->tail;
    n->next = NULL;

    if (dq->tail != NULL) {
        dq->tail->next = n;
    }
    dq->tail = n;

    if (dq->head == NULL) {   /* 原本是空队列，新节点同时是队头 */
        dq->head = n;
    }

    dq->size++;
    return OK;
}

/* ---------------- 删除 ---------------- */

/* 删除队头；成功返回 OK 并把值写入 out（out 可为 NULL），空队列返回 ERR_EMPTY */
int deque_pop_front(Deque *dq, Pos *out)
{
    DNode *victim;

    if (dq == NULL || dq->head == NULL) {
        return ERR_EMPTY;
    }

    victim = dq->head;
    if (out != NULL) {
        *out = victim->value;
    }

    dq->head = victim->next;
    if (dq->head != NULL) {
        dq->head->prev = NULL;
    } else {
        dq->tail = NULL;      /* 删空了，队尾也要清 */
    }

    free(victim);
    dq->size--;
    return OK;
}

/* 删除队尾；成功返回 OK 并把值写入 out（out 可为 NULL），空队列返回 ERR_EMPTY */
int deque_pop_back(Deque *dq, Pos *out)
{
    DNode *victim;

    if (dq == NULL || dq->tail == NULL) {
        return ERR_EMPTY;
    }

    victim = dq->tail;
    if (out != NULL) {
        *out = victim->value;
    }

    dq->tail = victim->prev;
    if (dq->tail != NULL) {
        dq->tail->next = NULL;
    } else {
        dq->head = NULL;
    }

    free(victim);
    dq->size--;
    return OK;
}

/* ---------------- 读取端点 ---------------- */

/* 取队头值但不删除；成功 OK，空队列 ERR_EMPTY */
int deque_front(const Deque *dq, Pos *out)
{
    if (dq == NULL || dq->head == NULL || out == NULL) {
        return ERR_EMPTY;
    }
    *out = dq->head->value;
    return OK;
}

/* 取队尾值但不删除；成功 OK，空队列 ERR_EMPTY */
int deque_back(const Deque *dq, Pos *out)
{
    if (dq == NULL || dq->tail == NULL || out == NULL) {
        return ERR_EMPTY;
    }
    *out = dq->tail->value;
    return OK;
}

/* ---------------- 查找 ---------------- */

/*
 * 线性查找，最坏 O(n)。
 * 【注意】这是"朴素版碰撞检测"用的接口，保留它是为了做对比实验，
 * 优化版请改用 hashset。千万不要在优化版的每帧逻辑里调用它。
 */
int deque_contains(const Deque *dq, Pos v)
{
    const DNode *cur;

    if (dq == NULL) {
        return 0;
    }

    for (cur = dq->head; cur != NULL; cur = cur->next) {
        if (cur->value == v) {
            return 1;
        }
    }
    return 0;
}

/* ---------------- 遍历 ---------------- */

/* 从头到尾依次调用 fn；fn 为 NULL 时什么也不做 */
void deque_foreach(const Deque *dq, DequeVisitFn fn, void *ctx)
{
    const DNode *cur;
    int          index = 0;

    if (dq == NULL || fn == NULL) {
        return;
    }

    for (cur = dq->head; cur != NULL; cur = cur->next) {
        fn(cur->value, index, ctx);
        index++;
    }
}

/* ---------------- 清空 ---------------- */

/* 释放全部节点，队列变为空；dq 为 NULL 时什么也不做 */
void deque_clear(Deque *dq)
{
    if (dq == NULL) {
        return;
    }
    free_all_nodes(dq);
}
