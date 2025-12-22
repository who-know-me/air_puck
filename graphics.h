#ifndef _GRAPHICS_H_
#define _GRAPHICS_H_

#include "game.h"

// 图形初始化
void graphics_init(void);

// 绘制游戏
void graphics_draw(void);

// 显示进球动画
void graphics_show_goal_animation(void);

// 显示胜利信息
void graphics_show_win_message(int winner);

#endif /* _GRAPHICS_H_ */
