#include "ai.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static AIController ai_controller;
static int ai_enabled = 1;

// 获取当前时间（秒）
static float get_current_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// AI初始化
void ai_init(void) {
    ai_controller.state = AI_DEFENSE;
    ai_controller.last_think_time = get_current_time();
    ai_controller.strategy_x = field_x + field_width * 0.75f;
    ai_controller.strategy_y = screen_center_y;
    ai_controller.stuck_counter = 0;
    ai_enabled = 1;
}

// 设置AI是否启用
void ai_set_enabled(int enabled) {
    ai_enabled = enabled;
}

// AI更新
void ai_update(float delta_time) {
    if (!ai_enabled) return;

    float current_time = get_current_time();

    // 每0.5秒重新思考一次
    if (current_time - ai_controller.last_think_time > AI_THINK_INTERVAL) {
        ai_controller.last_think_time = current_time;

        // 计算冰球到球门的距离
        float puck_to_player = sqrtf(powf(puck.x - player2.x, 2) + powf(puck.y - player2.y, 2));
        float puck_speed = sqrtf(puck.vx * puck.vx + puck.vy * puck.vy);

        // 判断球是否在AI后方
        int is_ball_behind = (puck.x < player2.x);

        // 判断是否卡住
        if (puck_to_player < 50 && puck_speed < 2.0f) {
            ai_controller.stuck_counter++;
        }
        else {
            ai_controller.stuck_counter = 0;
        }

        // 根据情况选择AI状态
        if (ai_controller.stuck_counter > 3) {
            // 被卡住了，切换到解围模式
            ai_controller.state = AI_CLEAR;
            ai_controller.strategy_x = puck.x;
            ai_controller.strategy_y = puck.y;
        }
        else if (is_ball_behind) {
            // 球在AI后方，需要转身去追球
            ai_controller.state = AI_RETREAT;
            ai_controller.strategy_x = puck.x - 30; // 在球后方一点
            ai_controller.strategy_y = puck.y;

            // 确保不会跑出己方半场
            if (ai_controller.strategy_x < screen_center_x + player2.radius) {
                ai_controller.strategy_x = screen_center_x + player2.radius;
            }
        }
        else if (puck.x < screen_center_x) {
            // 冰球在对方半场，防守模式
            ai_controller.state = AI_DEFENSE;
            ai_controller.strategy_x = goal2.x - 100;

            // 预测球的行进路线
            if (puck.vx > 0) {
                float predict_y = puck.y + puck.vy * (goal2.x - puck.x) / puck.vx;
                ai_controller.strategy_y = predict_y;
            }
            else {
                ai_controller.strategy_y = screen_center_y;
            }
        }
        else if (puck.vx > 5.0f && puck.x > screen_center_x + 150) {
            // 冰球快速冲向己方球门，紧急撤退
            ai_controller.state = AI_RETREAT;
            ai_controller.strategy_x = goal2.x - 50;
            ai_controller.strategy_y = puck.y;
        }
        else {
            // 进攻模式
            ai_controller.state = AI_ATTACK;

            // 计算球到AI的距离
            float dx_to_puck = puck.x - player2.x;
            float dy_to_puck = puck.y - player2.y;
            float distance_to_puck = sqrtf(dx_to_puck * dx_to_puck + dy_to_puck * dy_to_puck);

            if (distance_to_puck > 80) {
                // 离球较远，直接去球的位置
                ai_controller.strategy_x = puck.x;
                ai_controller.strategy_y = puck.y;
            }
            else {
                // 离球较近，计算击球方向
                float target_goal_x = goal1.x;
                float target_goal_y = screen_center_y;
                float dx_to_goal = target_goal_x - puck.x;
                float dy_to_goal = target_goal_y - puck.y;
                float total_distance = sqrtf(dx_to_goal * dx_to_goal + dy_to_goal * dy_to_goal);

                if (total_distance > 0) {
                    ai_controller.strategy_x = puck.x + (dx_to_goal / total_distance) * 40.0f;
                    ai_controller.strategy_y = puck.y + (dy_to_goal / total_distance) * 40.0f;
                }
                else {
                    ai_controller.strategy_x = puck.x + 40.0f;
                    ai_controller.strategy_y = puck.y;
                }
            }
        }

        // 限制策略位置在球场内
        if (ai_controller.strategy_x < screen_center_x + player2.radius) {
            ai_controller.strategy_x = screen_center_x + player2.radius;
        }
        if (ai_controller.strategy_x > field_x + field_width - player2.radius) {
            ai_controller.strategy_x = field_x + field_width - player2.radius;
        }
        if (ai_controller.strategy_y < field_y + player2.radius) {
            ai_controller.strategy_y = field_y + player2.radius;
        }
        if (ai_controller.strategy_y > field_y + field_height - player2.radius) {
            ai_controller.strategy_y = field_y + field_height - player2.radius;
        }
    }

    // 平滑移动到目标位置
    float dx = ai_controller.strategy_x - player2.x;
    float dy = ai_controller.strategy_y - player2.y;
    float distance = sqrtf(dx * dx + dy * dy);

    if (distance > 0) {
        dx /= distance;
        dy /= distance;

        // 根据状态调整速度
        float speed = 10.0f;
        if (ai_controller.state == AI_RETREAT || ai_controller.state == AI_CLEAR) {
            speed = 15.0f;
        }
        if (distance > 100) {
            speed = 12.0f;
        }

        float move_distance = fminf(distance, speed);
        player2.vx = dx * move_distance;
        player2.vy = dy * move_distance;
    }
    else {
        player2.vx *= 0.9f;
        player2.vy *= 0.9f;
    }
}