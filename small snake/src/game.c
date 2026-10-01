#include "game.h"
#include "deque.h"
#include "hashset.h"
#include "foodpool.h"

#include <stdlib.h>
#include <time.h>

/*
 * game.c —— 游戏逻辑层
 *
 * 三个结构始终同步维护，各有各的职责：
 *   body     双端队列，存蛇身每一节的格子号，头插尾删
 *   occupied 哈希集合，存蛇身占据的全部格子，用于 O(1) 碰撞检测
 *   pool     空闲格集合，存当前没被占的格子，用于 O(1) 生成食物
 *
 * 同一份坐标数据同时喂给这三个结构，所以"占一格/让一格"这类操作
 * 统一走 occupy_cell / release_cell，避免漏改某一边导致状态不一致。
 */

#define INIT_SNAKE_LEN 3

struct Game {
    Deque     *body;
    Direction  dir;
    Pos        food;
    HashSet   *occupied;
    FoodPool  *pool;
    GameState  state;
    int        score;
    GameStats  stats;
};

/* ---------------- 内部辅助 ---------------- */

/* 算出新蛇头格子；撞墙返回 ERR */
static int next_head_pos(const Game *g, Pos *out)
{
    Pos head = 0;
    int x, y;

    if (deque_front(g->body, &head) != OK) {
        return ERR;
    }

    x = POS_X(head);
    y = POS_Y(head);
    switch (g->dir) {
        case DIR_UP:    y--; break;
        case DIR_DOWN:  y++; break;
        case DIR_LEFT:  x--; break;
        case DIR_RIGHT: x++; break;
        default: return ERR;
    }

    if (x < 0 || x >= MAP_W || y < 0 || y >= MAP_H) {
        return ERR;
    }

    *out = POS_MAKE(x, y);
    return OK;
}

/* 查询格子是否被蛇身占据，并累加 collision_checks */
static int is_occupied(Game *g, Pos pos)
{
    if (g == NULL) {
        return 0;
    }

    if (g->occupied == NULL) {
        return 0;
    }

    /* 统计口径：本次查询在哈希表里探测了几个节点 */
    {
        HashSetStats before = hs_stats(g->occupied);
        int hit = hs_contains(g->occupied, pos);
        HashSetStats after  = hs_stats(g->occupied);
        g->stats.collision_checks += (after.probes - before.probes);
        return hit;
    }
}

static int spawn_food(Game *g)
{
    if (g == NULL || g->pool == NULL) {
        return ERR;
    }
    if (deque_size(g->body) >= POS_COUNT) {
        return ERR_EMPTY;   /* 蛇已经填满整张地图，无处可放 */
    }
    return fp_pick_random(g->pool, &g->food);
}

static int occupy_cell(Game *g, Pos cell)
{
    if (hs_insert(g->occupied, cell) != OK) {
        return ERR;
    }
    if (fp_remove(g->pool, cell) != OK) {
        return ERR;
    }
    return OK;
}

static int release_cell(Game *g, Pos cell)
{
    if (hs_erase(g->occupied, cell) != OK) {
        return ERR;
    }
    if (fp_add(g->pool, cell) != OK) {
        return ERR;
    }
    return OK;
}

typedef struct {
    int want;
    int ok;
    Pos found;
} CellQuery;

static void cell_visit(Pos value, int index, void *ctx)
{
    CellQuery *q = (CellQuery *)ctx;
    if (index == q->want) {
        q->found = value;
        q->ok    = 1;
    }
}

/* ---------------- 生命周期 ---------------- */

Game *game_create(void)
{
    Game *g = (Game *)malloc(sizeof(Game));
    if (g == NULL) {
        return NULL;
    }

    srand((unsigned int)time(NULL));

    g->body     = deque_create();
    g->occupied = hs_create(64);
    g->pool     = fp_create();
    if (g->body == NULL || g->occupied == NULL || g->pool == NULL) {
        game_destroy(g);
        return NULL;
    }

    game_reset(g);
    return g;
}

void game_destroy(Game *g)
{
    if (g == NULL) {
        return;
    }
    deque_destroy(g->body);
    hs_destroy(g->occupied);
    fp_destroy(g->pool);
    free(g);
}

void game_reset(Game *g)
{
    int i;
    int hy;
    int hx;

    if (g == NULL) {
        return;
    }

    deque_clear(g->body);
    hs_clear(g->occupied);
    fp_reset(g->pool);

    /* 中间偏左横向摆开，队头是蛇头，朝右 */
    hy = MAP_H / 2;
    hx = MAP_W / 2;
    for (i = INIT_SNAKE_LEN - 1; i >= 0; i--) {
        Pos cell = POS_MAKE(hx - i, hy);
        deque_push_front(g->body, cell);
        occupy_cell(g, cell);
    }

    g->dir   = DIR_RIGHT;
    g->score = 0;
    g->state = GS_READY;
    g->stats.collision_checks = 0;
    g->stats.steps            = 0;
    hs_reset_stats(g->occupied);
    fp_reset_stats(g->pool);

    if (spawn_food(g) != OK) {
        g->food = POS_MAKE(hx + 2, hy);
    }
}

/* ---------------- 推进与操作 ---------------- */

int game_update(Game *g)
{
    Pos new_head;
    Pos tail;
    int will_eat;
    int hit;

    if (g == NULL || g->state != GS_RUNNING) {
        return OK;
    }

    if (next_head_pos(g, &new_head) != OK) {
        g->state = GS_OVER;
        return ERR;
    }

    will_eat = (new_head == g->food) ? 1 : 0;
    deque_back(g->body, &tail);

    hit = is_occupied(g, new_head);
    if (hit && !( !will_eat && new_head == tail )) {
        g->state = GS_OVER;
        return ERR;
    }

    if (will_eat) {
        if (deque_push_front(g->body, new_head) != OK) {
            g->state = GS_OVER;
            return ERR;
        }
        occupy_cell(g, new_head);
        g->score++;
        if (spawn_food(g) != OK) {
            g->state = GS_OVER;
            return ERR;
        }
    } else {
        Pos gone;
        /* 先让出蛇尾，再占新头：走到当前尾格时不会在哈希表里重复插入 */
        deque_pop_back(g->body, &gone);
        release_cell(g, gone);
        if (deque_push_front(g->body, new_head) != OK) {
            g->state = GS_OVER;
            return ERR;
        }
        occupy_cell(g, new_head);
    }

    g->stats.steps++;
    return OK;
}

static int is_opposite(Direction a, Direction b)
{
    return (a == DIR_UP    && b == DIR_DOWN)  ||
           (a == DIR_DOWN  && b == DIR_UP)    ||
           (a == DIR_LEFT  && b == DIR_RIGHT) ||
           (a == DIR_RIGHT && b == DIR_LEFT);
}

int game_set_direction(Game *g, Direction d)
{
    if (g == NULL) {
        return ERR;
    }
    if (g->state != GS_RUNNING && g->state != GS_READY) {
        return ERR;
    }
    if (is_opposite(g->dir, d)) {
        return ERR;
    }
    g->dir = d;
    return OK;
}

GameState game_toggle_pause(Game *g)
{
    if (g == NULL) {
        return GS_READY;
    }
    if (g->state == GS_RUNNING) {
        g->state = GS_PAUSED;
    } else if (g->state == GS_PAUSED) {
        g->state = GS_RUNNING;
    }
    return g->state;
}

int game_start(Game *g)
{
    if (g == NULL) {
        return ERR;
    }
    if (g->state == GS_READY) {
        g->state = GS_RUNNING;
    }
    return (int)g->state;
}

/* ---------------- 只读查询接口 ---------------- */

GameState game_state(const Game *g)
{
    return (g == NULL) ? GS_READY : g->state;
}

int game_score(const Game *g)
{
    return (g == NULL) ? 0 : g->score;
}

int game_snake_len(const Game *g)
{
    return (g == NULL) ? 0 : deque_size(g->body);
}

int game_snake_cell(const Game *g, int index, Pos *out)
{
    CellQuery q;

    if (g == NULL || out == NULL || index < 0 || index >= deque_size(g->body)) {
        return ERR;
    }

    q.want  = index;
    q.ok    = 0;
    q.found = 0;
    deque_foreach(g->body, cell_visit, &q);
    if (!q.ok) {
        return ERR;
    }
    *out = q.found;
    return OK;
}

void game_foreach_snake(const Game *g, SnakeVisitFn fn, void *ctx)
{
    if (g == NULL || fn == NULL) {
        return;
    }
    deque_foreach(g->body, (DequeVisitFn)fn, ctx);
}

int game_food(const Game *g, Pos *out)
{
    if (g == NULL || out == NULL) {
        return ERR;
    }
    *out = g->food;
    return OK;
}

GameStats game_stats(const Game *g)
{
    GameStats zero = {0, 0};
    return (g == NULL) ? zero : g->stats;
}

void game_reset_stats(Game *g)
{
    if (g == NULL) {
        return;
    }
    g->stats.collision_checks = 0;
    g->stats.steps            = 0;
    hs_reset_stats(g->occupied);
    fp_reset_stats(g->pool);
}
