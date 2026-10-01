#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "render.h"

#include <graphics.h>
#include <stdio.h>
#include <windows.h>

/*
 * render.cpp —— 渲染层（全项目唯一包含 graphics.h 的文件）
 *
 * 只读 Game，通过查询接口画图。双缓冲：init 时 BeginBatchDraw，
 * 每帧 FlushBatchDraw，关闭时 EndBatchDraw。
 */

#define COLOR_BG         RGB(24, 26, 32)
#define COLOR_GRID       RGB(38, 41, 50)
#define COLOR_SNAKE_HEAD RGB(80, 220, 130)
#define COLOR_SNAKE_BODY RGB(46, 170, 100)
#define COLOR_FOOD       RGB(240, 90, 90)
#define COLOR_TEXT       RGB(230, 230, 235)
#define COLOR_HUD        RGB(180, 185, 195)
#define COLOR_OVERLAY    RGB(255, 220, 90)

#define HUD_H 36

typedef struct {
    int index;
} DrawCtx;

int render_init(void)
{
    int w = MAP_W * CELL_PX + 2 * GRID_ORIGIN_X;
    int h = MAP_H * CELL_PX + 2 * GRID_ORIGIN_Y + HUD_H;

    initgraph(w, h);
    setbkcolor(COLOR_BG);
    setbkmode(TRANSPARENT);
    cleardevice();
    BeginBatchDraw();
    return OK;
}

void render_close(void)
{
    EndBatchDraw();
    closegraph();
}

void render_cell_rect(Pos cell, int *left, int *top, int *size)
{
    if (left != NULL) { *left = GRID_ORIGIN_X + POS_X(cell) * CELL_PX; }
    if (top  != NULL) { *top  = GRID_ORIGIN_Y + POS_Y(cell) * CELL_PX; }
    if (size != NULL) { *size = CELL_PX; }
}

static void draw_grid(void)
{
    int x, y;
    int left = GRID_ORIGIN_X;
    int top  = GRID_ORIGIN_Y;
    int right  = GRID_ORIGIN_X + MAP_W * CELL_PX;
    int bottom = GRID_ORIGIN_Y + MAP_H * CELL_PX;

    setlinecolor(COLOR_GRID);
    for (x = 0; x <= MAP_W; x++) {
        int px = left + x * CELL_PX;
        line(px, top, px, bottom);
    }
    for (y = 0; y <= MAP_H; y++) {
        int py = top + y * CELL_PX;
        line(left, py, right, py);
    }
}

static void draw_snake_segment(Pos cell, int index, void *ctx)
{
    int left, top, size;
    (void)ctx;
    render_cell_rect(cell, &left, &top, &size);
    setfillcolor(index == 0 ? COLOR_SNAKE_HEAD : COLOR_SNAKE_BODY);
    solidrectangle(left + 1, top + 1, left + size - 2, top + size - 2);
}

/* EasyX 在 Unicode 工程里只接受 wchar_t*，逻辑层仍用 char*，这里做一次转换 */
static void to_wide(const char *src, wchar_t *dst, int cap)
{
    int n;

    if (dst == NULL || cap <= 0) {
        return;
    }
    dst[0] = L'\0';
    if (src == NULL) {
        return;
    }
    n = MultiByteToWideChar(CP_UTF8, 0, src, -1, dst, cap);
    if (n <= 0) {
        MultiByteToWideChar(CP_ACP, 0, src, -1, dst, cap);
    }
}

void render_text(const char *text, int screen_y, int font_size)
{
    wchar_t wtext[256];
    int w;
    int win_w;

    if (text == NULL) {
        return;
    }

    to_wide(text, wtext, 256);
    settextcolor(COLOR_OVERLAY);
    settextstyle(font_size, 0, L"Microsoft YaHei");
    w = textwidth(wtext);
    win_w = MAP_W * CELL_PX + 2 * GRID_ORIGIN_X;
    outtextxy((win_w - w) / 2, screen_y, wtext);
}

void render_frame(const Game *g)
{
    DrawCtx ctx;
    Pos food;
    GameStats st;
    char hud[160];
    int hud_y;

    if (g == NULL) {
        return;
    }

    ctx.index = 0;
    cleardevice();
    draw_grid();

    if (game_food(g, &food) == OK) {
        int left, top, size;
        render_cell_rect(food, &left, &top, &size);
        setfillcolor(COLOR_FOOD);
        solidrectangle(left + 3, top + 3, left + size - 4, top + size - 4);
    }

    game_foreach_snake(g, draw_snake_segment, &ctx);

    st = game_stats(g);
    sprintf(hud, "分数 %d  长度 %d  碰撞比较 %llu  步数 %llu   R重开  ESC退出",
            game_score(g),
            game_snake_len(g),
            st.collision_checks,
            st.steps);

    {
        wchar_t whud[256];
        to_wide(hud, whud, 256);
        settextcolor(COLOR_HUD);
        settextstyle(16, 0, L"Microsoft YaHei");
        hud_y = GRID_ORIGIN_Y + MAP_H * CELL_PX + 10;
        outtextxy(GRID_ORIGIN_X, hud_y, whud);
    }

    switch (game_state(g)) {
        case GS_READY:
            render_text("按空格开始", GRID_ORIGIN_Y + MAP_H * CELL_PX / 2 - 12, 28);
            break;
        case GS_PAUSED:
            render_text("已暂停", GRID_ORIGIN_Y + MAP_H * CELL_PX / 2 - 12, 28);
            break;
        case GS_OVER: {
            char over[64];
            sprintf(over, "游戏结束  得分: %d  按 R 重开", game_score(g));
            render_text(over, GRID_ORIGIN_Y + MAP_H * CELL_PX / 2 - 12, 28);
            break;
        }
        default:
            break;
    }

    FlushBatchDraw();
}
