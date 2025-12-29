#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../common/common.h"
#include "game.h"
#include "ai.h"
#include "draw.h"
#include "input.h"
#include "bluetooth.h"

// 全局变量
static int is_bluetooth_mode = 0;

// 触摸事件回调
static void on_touch(int x, int y, int type, int finger) {
    switch (type) {
    case TOUCH_PRESS:
        printf("TOUCH_PRESS");
    case TOUCH_MOVE:
        if (finger == 0 && x < screen_center_x) {
            // 直接设置目标位置，不要添加延迟或平滑
            player1.target_x = x;
            player1.target_y = y;

            // 如果蓝牙已连接，发送位置信息
            if (is_bluetooth_mode) {
                char buffer[32];
                snprintf(buffer, sizeof(buffer), "POS:%.1f,%.1f\n", (float)x, (float)y);
                bluetooth_send(buffer);
            }
        }
        break;

    case TOUCH_RELEASE:
        if (finger == 0) {
            player1.target_x = player1.x;
            player1.target_y = player1.y;
        }
        break;

    case TOUCH_ERROR:
        printf("Touch device error\n");
        break;
    }
}

// 蓝牙数据回调
static void on_bluetooth_data(const char* data) {
    printf("Bluetooth received: %s\n", data);

    if (strstr(data, "POS:") == data) {
        // 解析位置信息
        float rx, ry;
        if (sscanf(data + 4, "%f,%f", &rx, &ry) == 2) {
            // 映射到对方半场
            float mapped_x = SCREEN_WIDTH - rx;
            player2.target_x = mapped_x;
            player2.target_y = ry;
        }
    }
    else if (strstr(data, "CONNECTED") != NULL) {
        printf("Bluetooth connected\n");
        bluetooth_set_connected(1);
        ai_set_enabled(0); // 蓝牙模式下禁用AI
        is_bluetooth_mode = 1;
    }
    else if (strstr(data, "GOAL:P1") == data) {
        player1.score++;
        printf("Remote goal! Player1 scores!\n");
    }
    else if (strstr(data, "GOAL:P2") == data) {
        player2.score++;
        printf("Remote goal! Player2 scores!\n");
    }
    else if (strstr(data, "SCORE:") == data) {
        int score1, score2;
        if (sscanf(data + 6, "%d,%d", &score1, &score2) == 2) {
            player1.score = score1;
            player2.score = score2;
        }
    }
}

// 游戏定时器回调
static void game_timer_cb(int period) {
    if (game_state == GAME_PLAYING) {
        game_update();
        draw_game();

        // 处理输入
        input_process();

        // 处理蓝牙
        if (is_bluetooth_mode) {
            bluetooth_process();
        }
    }
}

// 连接检查定时器
static void connection_timer_cb(int period) {
    // 同步比分（如果蓝牙连接）
    if (is_bluetooth_mode) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "SCORE:%d,%d\n", player1.score, player2.score);
        bluetooth_send(buffer);
    }
}

int main(int argc, char* argv[]) {
    // 初始化随机种子
    srand(time(NULL));

    // 初始化图形
    draw_init();

    // 初始化游戏
    game_init();

    // 初始化输入
    if (input_init("/dev/input/event2") > 0) {
        input_set_touch_callback(on_touch);
    }
    else {
        printf("input_init error\n");
    }

    // 尝试初始化蓝牙
    if (bluetooth_init("/dev/rfcomm0") > 0) {
        bluetooth_set_callback(on_bluetooth_data);
        is_bluetooth_mode = 1;
        ai_set_enabled(0); // 蓝牙模式下禁用AI
        printf("Running in Bluetooth mode\n");
    }
    else {
        printf("Running in local mode (AI opponent)\n");
    }

    // 绘制初始画面
    draw_game();

    // 添加定时器
    task_add_timer(16, game_timer_cb);      // ~60 FPS
    task_add_timer(1000, connection_timer_cb);  // 每秒检查连接

    // 进入主循环
    task_loop();

    return 0;
}