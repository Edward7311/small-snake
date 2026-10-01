#include "game.h"
#include "deque.h"
#include "hashset.h"
#include "foodpool.h"

#include <stdio.h>
#include <stdlib.h>

static int fail = 0;

static void expect(int cond, const char *msg)
{
    if (!cond) {
        printf("FAIL: %s\n", msg);
        fail++;
    }
}

static void test_foodpool(void)
{
    FoodPool *fp = fp_create();
    int i;
    Pos p;

    expect(fp != NULL, "fp_create");
    expect(fp_count(fp) == POS_COUNT, "fp_reset count");

    for (i = 0; i < 100; i++) {
        expect(fp_remove(fp, i) == OK, "fp_remove occupied");
        expect(fp_is_free(fp, i) == 0, "removed not free");
    }
    expect(fp_count(fp) == POS_COUNT - 100, "count after 100 remove");
    expect(fp_remove(fp, 0) == ERR_NOTFOUND, "double remove");

    for (i = 0; i < 100; i++) {
        expect(fp_add(fp, i) == OK, "fp_add back");
    }
    expect(fp_count(fp) == POS_COUNT, "count restored");
    expect(fp_add(fp, 0) == ERR_DUPLICATE, "double add");

    fp_reset_stats(fp);
    expect(fp_pick_random(fp, &p) == OK, "pick");
    expect(fp_pick_attempts(fp) == 1, "attempts == 1");
    expect(fp_is_free(fp, p) == 1, "picked cell still free (not removed)");

    fp_destroy(fp);
}

static void test_hashset(void)
{
    HashSet *hs = hs_create(64);
    int i;

    expect(hs != NULL, "hs_create");
    for (i = 0; i < 1000; i++) {
        expect(hs_insert(hs, i) == OK, "insert");
    }
    expect(hs_size(hs) == 1000, "size 1000");
    expect(hs_insert(hs, 10) == ERR_DUPLICATE, "duplicate");
    for (i = 0; i < 1000; i++) {
        expect(hs_contains(hs, i) == 1, "contains inserted");
    }
    expect(hs_contains(hs, 1001) == 0, "missing");
    for (i = 0; i < 500; i++) {
        expect(hs_erase(hs, i) == OK, "erase");
    }
    expect(hs_size(hs) == 500, "size 500");
    expect(hs_contains(hs, 0) == 0, "erased gone");
    expect(hs_contains(hs, 500) == 1, "kept");
    expect(hs_erase(hs, 0) == ERR_NOTFOUND, "erase missing");
    expect(hs_load_factor(hs) > 0.0, "load factor");
    hs_destroy(hs);
}

/* 食物不能落在蛇身上（说明 pool 与 body 没有失去同步） */
static int food_on_snake(const Game *g)
{
    Pos food;
    int i;

    if (game_food(g, &food) != OK) {
        return 1;
    }
    for (i = 0; i < game_snake_len(g); i++) {
        Pos c;
        if (game_snake_cell(g, i, &c) == OK && c == food) {
            return 1;
        }
    }
    return 0;
}

/* 蛇身里不能出现重复格子（说明"先让尾再占头"的顺序没有被写反） */
static int has_duplicate_cell(const Game *g)
{
    int i, j;

    for (i = 0; i < game_snake_len(g); i++) {
        Pos a;
        if (game_snake_cell(g, i, &a) != OK) {
            return 1;
        }
        for (j = i + 1; j < game_snake_len(g); j++) {
            Pos b;
            if (game_snake_cell(g, j, &b) != OK) {
                return 1;
            }
            if (a == b) {
                return 1;
            }
        }
    }
    return 0;
}

/* 开局状态：蛇长合理、食物不在蛇身上、禁止反向前进 */
static void test_game_init(void)
{
    Game *g = game_create();

    expect(g != NULL, "game_create");
    expect(game_state(g) == GS_READY, "initial state READY");
    expect(game_snake_len(g) > 0, "initial snake not empty");
    expect(game_snake_len(g) <= POS_COUNT, "initial snake fits map");
    expect(game_score(g) == 0, "initial score 0");
    expect(!food_on_snake(g), "initial food off snake");

    /* 初始朝右，按下左键应被拒绝且不改变状态 */
    expect(game_set_direction(g, DIR_LEFT) == ERR, "reject opposite direction");
    expect(game_set_direction(g, DIR_DOWN) == OK, "accept perpendicular direction");

    game_destroy(g);
}

/* 连续推进若干步，全程检查两条不变量 */
static void test_game_steps(void)
{
    Game *g = game_create();
    int i;
    int steps = 0;

    expect(g != NULL, "game_create");
    expect(game_start(g) == (int)GS_RUNNING, "start");

    for (i = 0; i < 200; i++) {
        if (i == 10) game_set_direction(g, DIR_DOWN);
        if (i == 20) game_set_direction(g, DIR_LEFT);
        if (i == 30) game_set_direction(g, DIR_UP);
        if (i == 40) game_set_direction(g, DIR_RIGHT);

        if (game_update(g) != OK) {
            break;      /* 撞墙或撞自己，正常结束 */
        }
        steps++;

        expect(!food_on_snake(g), "food never on snake");
        expect(game_snake_len(g) <= POS_COUNT, "snake fits map");
        if (game_snake_len(g) > 1) {
            expect(!has_duplicate_cell(g), "no duplicated body cell");
        }
    }

    expect(steps > 0, "advanced at least one step");
    expect(game_score(g) >= 0, "score never negative");

    game_destroy(g);
}

/* 一直往上走必须撞北墙：从 y=MAP_H/2 出发，只可能成功 MAP_H/2 步 */
static void test_game_wall(void)
{
    Game *g = game_create();
    int n = 0;

    expect(g != NULL, "game_create");
    game_start(g);
    game_set_direction(g, DIR_UP);

    while (game_update(g) == OK) {
        n++;
        if (n > POS_COUNT) {
            break;
        }
    }

    expect(n == MAP_H / 2, "reached north wall exactly at row 0");
    expect(game_state(g) == GS_OVER, "state becomes OVER");
    expect(game_update(g) == OK, "update after OVER does nothing");

    /* 重开必须把状态和统计都清干净 */
    game_reset(g);
    expect(game_state(g) == GS_READY, "reset back to READY");
    expect(game_score(g) == 0, "reset score");
    expect(game_stats(g).steps == 0, "reset stats");
    expect(!food_on_snake(g), "reset food off snake");

    game_destroy(g);
}

int main(void)
{
    test_foodpool();
    test_hashset();
    test_game_init();
    test_game_steps();
    test_game_wall();
    if (fail == 0) {
        printf("ALL LOGIC TESTS PASSED\n");
        return 0;
    }
    printf("%d assertion(s) failed\n", fail);
    return 1;
}
