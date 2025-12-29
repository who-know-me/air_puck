#ifndef _DRAW_H_
#define _DRAW_H_

#include "game.h"
#include "fjy_draw.h"

// 图形初始化
void draw_init(void);

// 绘制游戏
void draw_game(void);

// 显示进球动画
void draw_show_goal_animation(void);

// 显示胜利信息
void draw_show_win_message(int winner);

#endif /* _DRAW_H_ */
