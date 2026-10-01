#ifndef COMMON_H
#define COMMON_H

/*
 * ── extern "C" 保护（下面这一段，所有头文件里都要有）──
 * 我们的 render.cpp 是 C++ 文件（EasyX 只支持 C++），它会包含本项目的头文件。
 * 如果不加这层保护，C++ 编译器会把这些函数名按 C++ 的规则"改名"（name mangling），
 * 而 .c 文件编译出来的符号是没改过名的，两边对不上，
 * 链接时就会报"无法解析的外部符号 game_update"之类的错误。
 * 包在 extern "C" 里，等于告诉 C++ 编译器："这些按 C 的规矩来，别改名。"
 */
#ifdef __cplusplus
extern "C" {
#endif

/*
 * common.h —— 全项目公共定义
 *
 * 这里放三样东西：地图尺寸、坐标编码、统一的返回值约定。
 * 所有模块都必须通过这里的宏来换算坐标，不要各自写各自的公式，
 * 否则哈希表的键和渲染层的坐标一旦对不上，调试会非常痛苦。
 */

/* ---------- 地图尺寸 ---------- */
/* 改这两个宏即可调整地图大小，其余代码无需改动 */
#define MAP_W   40              /* 地图宽（格） */
#define MAP_H   40              /* 地图高（格） */
#define CELL_PX 20              /* 每格边长（像素），仅渲染层使用 */
#define GRID_ORIGIN_X 40        /* 网格左上角在窗口中的像素偏移 */
#define GRID_ORIGIN_Y 40

/* ---------- 坐标编码 ---------- */
/*
 * 二维坐标 (x, y) 压缩成一维整数 pos = y * MAP_W + x。
 * 这个整数是贯穿全项目的统一表示：
 *   - 蛇身链表里存的是它
 *   - 哈希表的键是它
 *   - 空闲格表的下标也是它（下标恰好等于格子编号，省掉一次映射）
 * 好处：一次整数比较就完成判等，比比较结构体快得多，也让哈希更简单。
 */
typedef int Pos;

#define POS_MAKE(x, y)   ((y) * MAP_W + (x))
#define POS_X(p)         ((p) % MAP_W)
#define POS_Y(p)         ((p) / MAP_W)
#define POS_COUNT        (MAP_W * MAP_H)
#define POS_VALID(p)     ((p) >= 0 && (p) < POS_COUNT)

/* ---------- 返回值约定 ---------- */
/*
 * 全项目统一：0 表示成功，负数表示失败/异常。
 * 需要返回多个值时，用"出参指针 + 返回状态码"的方式，不要用全局变量。
 */
#define OK          0
#define ERR        (-1)
#define ERR_NOTFOUND (-2)
#define ERR_DUPLICATE (-3)
#define ERR_EMPTY    (-4)

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif /* COMMON_H */
