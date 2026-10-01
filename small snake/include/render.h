#ifndef RENDER_H
#define RENDER_H

/* 保持 C 链接约定，详见 common.h 里的说明 */
#ifdef __cplusplus
extern "C" {
#endif

/*
 * render.h —— 渲染层（唯一允许 include <graphics.h> 的模块）
 *
 * 【设计原则】渲染层是"只读"的：它拿着一个 const Game*，
 * 通过 game.h 提供的只读查询接口把状态画出来，绝不修改任何游戏数据。
 * 换图形库（EasyX -> Qt）时只需要重写 render.c，游戏逻辑一行都不用动。
 *
 * 【EasyX 三个必须注意的点】
 *   1. 双缓冲防闪烁：循环里用 BeginBatchDraw() / FlushBatchDraw()，
 *      退出时 EndBatchDraw()。不加这段画面会明显抖动。
 *   2. 坐标换算统一用下面的 render_cell_rect()，不要在别处散落像素魔数。
 *   3. 文字提示用 render_text()，不要直接调 outtextxy，
 *      方便以后统一换字体、换编码。
 */

#include "common.h"
#include "game.h"

/* ---------- 生命周期 ---------- */
/*
 * 创建窗口并初始化绘图环境。
 * 窗口尺寸 = 地图尺寸 * 每格像素 + 四周留白，见 common.h 的偏移宏。
 * 成功返回 OK，失败返回 ERR。
 */
int  render_init(void);
void render_close(void);

/* ---------- 每帧绘制 ---------- */
/*
 * 画出当前这一帧的全部内容：背景、食物、蛇身、分数、状态提示。
 * 内部自己调用 FlushBatchDraw()，调用方每帧调一次即可。
 *
 * 【画蛇身的正确姿势】用 game_foreach_snake() 一次遍历画完，
 * 不要写 for (i = 0; i < len; i++) game_snake_cell(...) —— 那是 O(n^2)。
 */
void render_frame(const Game *g);

/* ---------- 辅助绘制（内部使用，也可少量对外）---------- */
/* 把格子编号换算成屏幕像素矩形：左上角 (left, top)，边长 CELL_PX */
void render_cell_rect(Pos cell, int *left, int *top, int *size);

/* 在指定屏幕行居中输出一行文字，用于暂停/结束等提示 */
void render_text(const char *text, int screen_y, int font_size);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif /* RENDER_H */
