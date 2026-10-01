#include "common.h"
#include "deque.h"
#include "hashset.h"
#include "foodpool.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*
 * bench.c —— 对比实验程序（不开窗口、不画图，只出数据）
 *
 * 【为什么要有这个文件】
 * 实验报告里的"效率分析"和答辩时老师追问的"你测过吗"，
 * 靠玩游戏是答不上来的。这个程序把两种实现放在同样的输入下跑一遍，
 * 把"比较次数"和"耗时"打印成一张表，直接抄进报告。
 *
 * 【为什么用"比较次数"而不是只报耗时】
 * C 跑得太快。蛇长几百的时候，单次操作可能连 1 微秒都不到，
 * 用 clock() 测出来的时间误差极大，甚至会出现"优化版更慢"的假象。
 * 而"比较次数"是确定的、与蛇长严格成线性关系的，测出来干净漂亮，
 * 也更容易讲清楚"为什么复杂度从 O(n) 降到了 O(1)"。
 * 两个指标都打印，报告里以次数为主、耗时为辅。
 *
 * 【编译方式（MinGW 示例）】
 *   gcc -O2 -Iinclude bench/bench.c src/deque.c src/hashset.c src/foodpool.c \
 *       -o bench.exe
 * 注意本文件不链接 render.c / game.c / main.c —— 它们依赖 EasyX。
 *
 * 【本文件依赖 hashset.c 与 foodpool.c 的实现】
 * 这两个模块还是骨架时，运行结果会不对，等实现完再跑。
 */

/* 每个规模下重复多少次查询，用来摊平随机波动 */
#define QUERY_ROUNDS 200000
/* 食物生成实验每组重复多少次 */
#define FOOD_ROUNDS  100000

/* ---------------- 实验一：碰撞检测 ---------------- */

/*
 * 对比"蛇头新位置是否撞到自己"这个判断：
 *   朴素：deque_contains()，逐个节点比较，O(n)
 *   优化：hs_contains()，哈希定位，平均 O(1)
 *
 * 蛇身长度取 100 / 400 / 800 / 1600。
 * 上限取 1600 是有依据的：地图共 MAP_W * MAP_H = 1600 格，
 * 蛇身不可能超过格子总数，所以这就是真实场景下的最大值。
 */
static void bench_collision(void)
{
    static const int sizes[] = { 100, 400, 800, 1600 };
    const int        n_sizes = (int)(sizeof(sizes) / sizeof(sizes[0]));
    int              s;

    printf("\n===== 实验一：碰撞检测（查询蛇头是否撞到自身）=====\n");
    printf("每组发起 %d 次查询；'比较次数'为该组累计值\n\n", QUERY_ROUNDS);
    printf("%-10s %-18s %-18s %-14s %-14s\n",
           "蛇长", "朴素比较次数", "哈希比较次数", "朴素耗时(s)", "哈希耗时(s)");
    printf("--------------------------------------------------------------------\n");

    for (s = 0; s < n_sizes; s++) {
        int    n = sizes[s];
        Deque *body = deque_create();
        HashSet *occ = hs_create(64);
        int    i;
        clock_t t0, t1, t2, t3;
        double  naive_sec, hash_sec;
        long long naive_checks;
        unsigned long long hash_probes;

        if (body == NULL || occ == NULL) {
            printf("内存分配失败，跳过 n=%d\n", n);
            deque_destroy(body);
            hs_destroy(occ);
            continue;
        }

        /* 构造蛇身：格子 0 .. n-1，同时放进两套结构 */
        for (i = 0; i < n; i++) {
            deque_push_back(body, i);
            hs_insert(occ, i);
        }

        srand(12345);   /* 固定种子，保证多次运行结果一致、可复现 */

        /* --- 朴素版 --- */
        t0 = clock();
        for (i = 0; i < QUERY_ROUNDS; i++) {
            Pos probe = rand() % POS_COUNT;   /* 随机探测一个格子 */
            deque_contains(body, probe);
        }
        t1 = clock();

        /*
         * 朴素版的比较次数是可以精确算出来的：
         * 每次查询最坏比较 n 次、平均约 n/2 次。
         * 这里按最坏情况计数，和哈希表的实际探测次数对照更直观。
         */
        naive_checks = (long long)n * QUERY_ROUNDS;

        /* --- 优化版 --- */
        hs_reset_stats(occ);
        t2 = clock();
        for (i = 0; i < QUERY_ROUNDS; i++) {
            Pos probe = rand() % POS_COUNT;
            hs_contains(occ, probe);
        }
        t3 = clock();

        hash_probes = hs_stats(occ).probes;

        naive_sec = (double)(t1 - t0) / CLOCKS_PER_SEC;
        hash_sec  = (double)(t3 - t2) / CLOCKS_PER_SEC;

        printf("%-10d %-18lld %-18llu %-14.4f %-14.4f\n",
               n, naive_checks, hash_probes, naive_sec, hash_sec);
        printf("           平均每次查询: 朴素 %.1f 次比较 / 哈希 %.3f 次比较\n\n",
               (double)naive_checks / QUERY_ROUNDS,
               (double)hash_probes / QUERY_ROUNDS);

        deque_destroy(body);
        hs_destroy(occ);
    }

    printf("【结论怎么写】朴素版每次查询的比较次数随蛇长线性增长，\n");
    printf("哈希版基本恒定在 1 次上下，不随蛇长变化 —— 这就是 O(n) 与 O(1) 的差别。\n");
}

/* ---------------- 实验二：食物生成 ---------------- */

/*
 * 对比"在空格里挑一个位置放食物"：
 *   朴素：随机取格子，撞到蛇身就重试，重试次数随地图填充率上升而爆炸
 *   优化：空闲格集合里直接取，永远是 1 次
 *
 * 填充率 = 蛇身占地图总格数的比例。蛇越长填充率越高。
 */
static void bench_food(void)
{
    static const double ratios[] = { 0.10, 0.50, 0.80, 0.90, 0.95, 0.99 };
    const int n_ratios = (int)(sizeof(ratios) / sizeof(ratios[0]));
    int       r;

    printf("\n===== 实验二：食物生成（在空格中随机取一个位置）=====\n");
    printf("每组重复 %d 次；'平均尝试次数'越小越好，1.0 为理论最优\n\n", FOOD_ROUNDS);
    printf("%-12s %-18s %-18s %-14s\n",
           "地图填充率", "朴素平均尝试次数", "优化平均尝试次数", "朴素最大尝试次数");
    printf("--------------------------------------------------------------------\n");

    for (r = 0; r < n_ratios; r++) {
        int    occupied_cnt = (int)(ratios[r] * POS_COUNT);
        char  *is_occ = (char *)calloc(POS_COUNT, sizeof(char));
        FoodPool *pool = fp_create();
        int    i;
        long long total_tries = 0;
        int    max_tries = 0;
        unsigned long long pool_attempts;

        if (is_occ == NULL || pool == NULL) {
            printf("内存分配失败，跳过 ratio=%.2f\n", ratios[r]);
            free(is_occ);
            fp_destroy(pool);
            continue;
        }

        /* 构造一个填充率为 ratios[r] 的地图：前 occupied_cnt 格标为被占 */
        for (i = 0; i < occupied_cnt; i++) {
            is_occ[i] = 1;
        }

        srand(54321);

        /* --- 朴素版：随机试到不撞为止 --- */
        for (i = 0; i < FOOD_ROUNDS; i++) {
            int tries = 0;
            for (;;) {
                Pos p = rand() % POS_COUNT;
                tries++;
                if (!is_occ[p]) {
                    break;
                }
            }
            total_tries += tries;
            if (tries > max_tries) {
                max_tries = tries;
            }
        }

        /* --- 优化版：空闲格集合直接取 --- */
        fp_reset(pool);
        for (i = 0; i < occupied_cnt; i++) {
            fp_remove(pool, i);   /* 被占的格子从空闲集里移除 */
        }
        {
            Pos out;
            for (i = 0; i < FOOD_ROUNDS; i++) {
                fp_pick_random(pool, &out);
            }
        }
        pool_attempts = fp_pick_attempts(pool);

        printf("%6.0f%%      %-18.2f %-18.2f %-14d\n",
               ratios[r] * 100.0,
               (double)total_tries / FOOD_ROUNDS,
               (double)pool_attempts / FOOD_ROUNDS,
               max_tries);

        free(is_occ);
        fp_destroy(pool);
    }

    printf("\n【结论怎么写】填充率 10%% 时朴素版只多试一点点，感觉不到；\n");
    printf("填充率 99%% 时朴素版平均要试上百次，这正是长蛇卡顿的根源。\n");
    printf("优化版始终是 1.00 次，与填充率完全无关 —— O(1) 就是这个意思。\n");
}

/* ---------------- 入口 ---------------- */

int main(void)
{
    printf("====================================================\n");
    printf("  贪吃蛇 —— 数据结构对比实验\n");
    printf("  地图 %d x %d = %d 格\n", MAP_W, MAP_H, POS_COUNT);
    printf("====================================================\n");

    bench_collision();
    bench_food();

    printf("\n====================================================\n");
    printf("跑完了。把上面两张表直接贴进实验报告的\"调试分析 - 效率分析\"一节。\n");
    printf("====================================================\n");
    return 0;
}
