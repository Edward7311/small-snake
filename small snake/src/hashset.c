#include "hashset.h"

#include <stdlib.h>

/*
 * hashset.c —— 哈希集合（链地址法）
 *
 * 哈希函数：
 *   格子编号 pos = y * 40 + x，相邻格子只差 1，直接 key % capacity
 *   在 capacity 与 MAP_W 有公因子时会把同一列打进同一个桶。
 *   因此先用乘法散列把低位差异打散，再对桶数取模。
 *
 * 冲突处理：链地址法。元素数不超过 1600，装载因子 > 0.75 时桶数翻倍重哈希。
 * 均摊插入仍为 O(1)。
 *
 * 统计：hs_contains 如实累加 lookups / probes，供 bench.c 出对比表。
 */

#define HS_MIN_CAP     8
#define HS_LOAD_NUM    3    /* 装载因子阈值 3/4 = 0.75 */
#define HS_LOAD_DEN    4

typedef struct HNode {
    Pos           key;
    struct HNode *next;
} HNode;

struct HashSet {
    HNode             **buckets;
    int                 capacity;
    int                 size;
    HashSetStats        stats;
};

/* ---------------- 哈希函数 ---------------- */

/*
 * Murmur / splitmix 风格的 32 位混合：把相邻格子编号的低位差放大到高位，
 * 避免 "key % 桶数" 在桶数与 40 有公因子时把整列撞进同一个桶。
 * 返回哈希值本身，调用方再对 capacity 取模得到桶下标。
 */
unsigned int hs_hash(Pos key)
{
    unsigned int x = (unsigned int)key;

    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

static unsigned int bucket_of(const HashSet *hs, Pos key)
{
    return hs_hash(key) % (unsigned int)hs->capacity;
}

static void free_all_nodes(HashSet *hs)
{
    int i;

    if (hs == NULL || hs->buckets == NULL) {
        return;
    }
    for (i = 0; i < hs->capacity; i++) {
        HNode *cur = hs->buckets[i];
        while (cur != NULL) {
            HNode *next = cur->next;
            free(cur);
            cur = next;
        }
        hs->buckets[i] = NULL;
    }
    hs->size = 0;
}

/* 桶数改为 new_capacity，把已有节点重新挂到新桶上 */
static int hs_rehash(HashSet *hs, int new_capacity)
{
    HNode **old_buckets;
    int     old_cap;
    HNode **neu;
    int     i;

    if (hs == NULL || new_capacity < HS_MIN_CAP) {
        return ERR;
    }

    neu = (HNode **)calloc((size_t)new_capacity, sizeof(HNode *));
    if (neu == NULL) {
        return ERR;
    }

    old_buckets = hs->buckets;
    old_cap     = hs->capacity;
    hs->buckets  = neu;
    hs->capacity = new_capacity;

    for (i = 0; i < old_cap; i++) {
        HNode *n = old_buckets[i];
        while (n != NULL) {
            HNode *next = n->next;
            unsigned int b = bucket_of(hs, n->key);
            n->next = hs->buckets[b];
            hs->buckets[b] = n;
            n = next;
        }
    }

    free(old_buckets);
    hs->stats.rehashes++;
    return OK;
}

/* ---------------- 生命周期 ---------------- */

HashSet *hs_create(int capacity)
{
    HashSet *hs;

    if (capacity < HS_MIN_CAP) {
        capacity = HS_MIN_CAP;
    }

    hs = (HashSet *)malloc(sizeof(HashSet));
    if (hs == NULL) {
        return NULL;
    }

    hs->buckets = (HNode **)calloc((size_t)capacity, sizeof(HNode *));
    if (hs->buckets == NULL) {
        free(hs);
        return NULL;
    }

    hs->capacity = capacity;
    hs->size     = 0;
    hs->stats.lookups    = 0;
    hs->stats.probes     = 0;
    hs->stats.collisions = 0;
    hs->stats.rehashes   = 0;
    return hs;
}

void hs_destroy(HashSet *hs)
{
    if (hs == NULL) {
        return;
    }
    free_all_nodes(hs);
    free(hs->buckets);
    free(hs);
}

void hs_clear(HashSet *hs)
{
    if (hs == NULL) {
        return;
    }
    free_all_nodes(hs);
}

/* ---------------- 基本操作 ---------------- */

int hs_insert(HashSet *hs, Pos key)
{
    unsigned int b;
    HNode *cur;
    HNode *n;

    if (hs == NULL) {
        return ERR;
    }

    b = bucket_of(hs, key);
    for (cur = hs->buckets[b]; cur != NULL; cur = cur->next) {
        if (cur->key == key) {
            return ERR_DUPLICATE;
        }
    }

    if (hs->buckets[b] != NULL) {
        hs->stats.collisions++;
    }

    n = (HNode *)malloc(sizeof(HNode));
    if (n == NULL) {
        return ERR;
    }
    n->key  = key;
    n->next = hs->buckets[b];
    hs->buckets[b] = n;
    hs->size++;

    if (hs->size * HS_LOAD_DEN > hs->capacity * HS_LOAD_NUM) {
        (void)hs_rehash(hs, hs->capacity * 2);
    }
    return OK;
}

int hs_erase(HashSet *hs, Pos key)
{
    unsigned int b;
    HNode *cur;
    HNode *prev;

    if (hs == NULL) {
        return ERR;
    }

    b    = bucket_of(hs, key);
    prev = NULL;
    for (cur = hs->buckets[b]; cur != NULL; prev = cur, cur = cur->next) {
        if (cur->key == key) {
            if (prev == NULL) {
                hs->buckets[b] = cur->next;
            } else {
                prev->next = cur->next;
            }
            free(cur);
            hs->size--;
            return OK;
        }
    }
    return ERR_NOTFOUND;
}

int hs_contains(HashSet *hs, Pos key)
{
    unsigned int b;
    HNode *cur;

    if (hs == NULL) {
        return 0;
    }

    hs->stats.lookups++;
    b = bucket_of(hs, key);
    for (cur = hs->buckets[b]; cur != NULL; cur = cur->next) {
        hs->stats.probes++;
        if (cur->key == key) {
            return 1;
        }
    }
    return 0;
}

/* ---------------- 状态查询 ---------------- */

int hs_size(const HashSet *hs)
{
    return (hs == NULL) ? 0 : hs->size;
}

double hs_load_factor(const HashSet *hs)
{
    if (hs == NULL || hs->capacity == 0) {
        return 0.0;
    }
    return (double)hs->size / (double)hs->capacity;
}

/* ---------------- 统计 ---------------- */

void hs_reset_stats(HashSet *hs)
{
    if (hs == NULL) {
        return;
    }
    hs->stats.lookups    = 0;
    hs->stats.probes     = 0;
    hs->stats.collisions = 0;
    hs->stats.rehashes   = 0;
}

HashSetStats hs_stats(const HashSet *hs)
{
    HashSetStats zero = {0, 0, 0, 0};
    return (hs == NULL) ? zero : hs->stats;
}
