#include "graphics.h"
#include "common.h"
#include <stdio.h>

// 图形初始化
void graphics_init(void) {
    fb_init("/dev/fb0");
    font_init("./font.ttc");
}

// 绘制游戏
void graphics_draw(void) {
    // 清屏
    fb_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BACKGROUND);

    // 绘制球场边框
    fb_draw_border(field_x, field_y, field_width, field_height, COLOR_FIELD);

    // 绘制中线
    fb_draw_line(screen_center_x, field_y, screen_center_x, field_y + field_height, COLOR_CENTER_LINE);

    // 绘制中圈
    fb_draw_circle(screen_center_x, screen_center_y, 60, COLOR_CENTER_LINE);
    fb_draw_circle(screen_center_x, screen_center_y, 59, COLOR_FIELD);

    // 绘制球门
    fb_draw_border(goal1.x, goal1.y, goal1.width, goal1.height, goal1.color);
    for (int i = 1; i < goal1.height - 1; i += 2) {
        fb_draw_line(goal1.x + 1, goal1.y + i,
            goal1.x + goal1.width - 1, goal1.y + i,
            FB_COLOR(0x66, 0x66, 0x66));
    }

    fb_draw_border(goal2.x, goal2.y, goal2.width, goal2.height, goal2.color);
    for (int i = 1; i < goal2.height - 1; i += 2) {
        fb_draw_line(goal2.x + 1, goal2.y + i,
            goal2.x + goal2.width - 1, goal2.y + i,
            FB_COLOR(0x66, 0x66, 0x66));
    }

    // 绘制玩家
    fb_draw_circle((int)player1.x, (int)player1.y, player1.radius, player1.color);
    fb_draw_circle((int)player2.x, (int)player2.y, player2.radius, player2.color);

    // 绘制冰球
    fb_draw_circle((int)puck.x, (int)puck.y, puck.radius, puck.color);

    // 绘制分数
    char score_text[32];
    sprintf(score_text, "P1: %d", player1.score);
    fb_draw_text(50, 30, score_text, 24, COLOR_TEXT);

    sprintf(score_text, "P2: %d", player2.score);
    fb_draw_text(SCREEN_WIDTH - 100, 30, score_text, 24, COLOR_TEXT);

    fb_update();
}

// 显示进球动画
void graphics_show_goal_animation(void) {
    fb_draw_text(screen_center_x - 60, screen_center_y - 30, "GOAL!", 48, COLOR_TEXT);
    fb_update();
}

// 显示胜利信息
void graphics_show_win_message(int winner) {
    if (winner == 1) {
        fb_draw_text(screen_center_x - 100, screen_center_y - 50,
            "Player 1 Wins!", 36, COLOR_PLAYER1);
    }
    else {
        fb_draw_text(screen_center_x - 100, screen_center_y - 50,
            "Player 2 Wins!", 36, COLOR_PLAYER2);
    }
    fb_draw_text(screen_center_x - 80, screen_center_y + 10,
        "Tap to restart", 24, COLOR_TEXT);
    fb_update();
}