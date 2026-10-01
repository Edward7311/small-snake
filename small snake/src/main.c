#include "game.h"
#include "render.h"

#include <windows.h>    /* GetTickCount64 / GetAsyncKeyState —— 计时与按键 */
#include <stdio.h>

/*
 * main.c —— 主循环
 *
 * 职责只有四件事：收键盘、按固定间隔推进逻辑、渲染、处理状态切换。
 * 游戏规则一律不写在这里，全部交给 game.c。
 *
 * 【为什么不用 Sleep 控制速度】
 * Sleep() 会让整个线程停下来，消息循环和按键检测都跟着被阻塞，
 * 表现出来就是"按了键要等半秒才响应"。正确做法是用时间戳累加：
 * 每帧只判断"距上次推进是否够了一个 STEP_MS"，不够就直接跳过，
 * 这样主循环每秒仍跑满，按键始终灵敏。
 *
 * 【固定步进间隔】
 * 蛇每 STEP_MS 毫秒走一格。初始给 150ms，之后可以随着分数提高而减小，
 * 让游戏越来越快 —— 这是贪吃蛇的标准做法，也是很好的报告素材。
 */

#define STEP_MS_INITIAL 150   /* 初始每格耗时（毫秒） */
#define STEP_MS_MIN      60   /* 最快不低于这个值 */
#define STEP_MS_DECREASE 8    /* 每吃到一个食物，间隔缩短这么多 */

/* ---------------- 键盘 ---------------- */

/* 读一次方向键；没有按下方向键返回 -1 */
static int read_direction_key(void)
{
    if (GetAsyncKeyState(VK_UP)    & 0x8000) { return DIR_UP; }
    if (GetAsyncKeyState(VK_DOWN)  & 0x8000) { return DIR_DOWN; }
    if (GetAsyncKeyState(VK_LEFT)  & 0x8000) { return DIR_LEFT; }
    if (GetAsyncKeyState(VK_RIGHT) & 0x8000) { return DIR_RIGHT; }
    /* 也可以顺手支持 WASD，答辩时老师未必用方向键 */
    if (GetAsyncKeyState('W') & 0x8000) { return DIR_UP; }
    if (GetAsyncKeyState('S') & 0x8000) { return DIR_DOWN; }
    if (GetAsyncKeyState('A') & 0x8000) { return DIR_LEFT; }
    if (GetAsyncKeyState('D') & 0x8000) { return DIR_RIGHT; }
    return -1;
}

/* 判断某个键"这一帧刚被按下"（用于空格/R 这类不需要连发的键） */
static int key_pressed_once(int vk, int *was_down)
{
    int now_down = (GetAsyncKeyState(vk) & 0x8000) ? 1 : 0;
    int fresh    = (now_down && !(*was_down));
    *was_down    = now_down;
    return fresh;
}

/* ---------------- 主循环 ---------------- */

int main(void)
{
    Game *g;
    ULONGLONG last_step_ms;
    int step_ms = STEP_MS_INITIAL;
    int was_space = 0, was_r = 0;

    g = game_create();
    if (g == NULL) {
        printf("game_create 失败：内存不足\n");
        return 1;
    }

    if (render_init() != OK) {
        printf("render_init 失败：请确认已正确安装 EasyX 并包含 graphics.h\n");
        game_destroy(g);
        return 1;
    }

    last_step_ms = GetTickCount64();

    for (;;) {
        ULONGLONG now;
        int dir;

        /* --- 1. 收键盘 --- */
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            break;   /* 退出 */
        }

        dir = read_direction_key();
        if (dir >= 0) {
            game_set_direction(g, (Direction)dir);
        }

        if (key_pressed_once(VK_SPACE, &was_space)) {
            /* 等待开始时按空格开跑，运行中按空格暂停，暂停中按空格继续 */
            if (game_state(g) == GS_READY) {
                game_start(g);
            } else {
                game_toggle_pause(g);
            }
        }
        if (key_pressed_once('R', &was_r)) {
            game_reset(g);
            step_ms = STEP_MS_INITIAL;
        }

        /* --- 2. 按固定间隔推进逻辑 --- */
        now = GetTickCount64();
        if (game_state(g) == GS_RUNNING && now - last_step_ms >= (ULONGLONG)step_ms) {
            last_step_ms = now;

            game_update(g);

            /* 吃到食物就加速一点，让游戏有难度曲线 */
            if (game_score(g) > 0 && step_ms > STEP_MS_MIN) {
                step_ms = STEP_MS_INITIAL - game_score(g) * STEP_MS_DECREASE;
                if (step_ms < STEP_MS_MIN) {
                    step_ms = STEP_MS_MIN;
                }
            }
        }

        /* --- 3. 渲染（每帧都画，不要只在逻辑推进时才画） --- */
        render_frame(g);

        /* --- 4. 让出一点 CPU，避免把单核跑满 --- */
        Sleep(1);
    }

    render_close();
    game_destroy(g);
    return 0;
}
