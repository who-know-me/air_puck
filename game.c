#include "game.h"
#include "ai.h"
#include "draw.h"
#include <math.h>
#include "bluetooth.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

// 游戏全局变量定义
GameState game_state = GAME_PLAYING;
Player player1, player2;
Puck puck;
Goal goal1, goal2;
int screen_center_x, screen_center_y;
int field_width, field_height;
int field_x, field_y;

int frame_id = 0; 
Input local_input_buffer[INPUT_DELAY], remote_input_buffer[INPUT_DELAY]; 


// 获取当前时间（秒）
static float get_current_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static float last_time = 0;

// 游戏初始化
void game_init(void) {
    // 计算球场区域
    screen_center_x = SCREEN_WIDTH / 2;
    screen_center_y = SCREEN_HEIGHT / 2;
    field_width = SCREEN_WIDTH - FIELD_PADDING * 2;
    field_height = SCREEN_HEIGHT - FIELD_PADDING * 2;
    field_x = FIELD_PADDING;
    field_y = FIELD_PADDING;

    // initialize frameid
    frame_id = 0;

    // 初始化玩家1 (左侧)
    player1.x = field_x + field_width * 0.25f;
    player1.y = screen_center_y;
    Input p1_init; p1_init.xin = player1.x; p1_init.yin = player1.y;
    for(int i = 0; i<INPUT_DELAY; i++){
        save_local_input(i, p1_init);
    }
    player1.target_x = load_local_input(frame_id).xin;
    player1.target_y = load_local_input(frame_id).yin;
    player1.vx = player1.vy = 0;
    player1.score = 0;
    player1.radius = PLAYER_RADIUS;
    player1.color = COLOR_PLAYER1;

    // 初始化玩家2 (右侧)
    player2.x = field_x + field_width * 0.75f;
    player2.y = screen_center_y;
    Input p2_init; p2_init.xin = player2.x; p2_init.yin = player2.y;
    for(int i = 0; i<INPUT_DELAY; i++){
        save_remote_input(i, p2_init);
    }
    player2.target_x = load_remote_input(frame_id).xin;
    player2.target_y = load_remote_input(frame_id).yin;
    player2.vx = player2.vy = 0;
    player2.score = 0;
    player2.radius = PLAYER_RADIUS;
    player2.color = COLOR_PLAYER2;

    // 初始化冰球
    puck.x = screen_center_x;
    puck.y = screen_center_y;
    puck.vx = 3.0f;  // 给冰球一个初始速度
    puck.vy = 2.0f;
    puck.radius = PUCK_RADIUS;
    puck.color = COLOR_PUCK;

    // 初始化球门
    goal1.x = field_x - GOAL_DEPTH;
    goal1.y = screen_center_y - GOAL_WIDTH / 2;
    goal1.width = GOAL_DEPTH;
    goal1.height = GOAL_WIDTH;
    goal1.color = COLOR_GOAL;

    goal2.x = field_x + field_width;
    goal2.y = screen_center_y - GOAL_WIDTH / 2;
    goal2.width = GOAL_DEPTH;
    goal2.height = GOAL_WIDTH;
    goal2.color = COLOR_GOAL;

    // 初始化AI
    ai_init();

    last_time = get_current_time();
}

// 碰撞检测
int check_collision(float x1, float y1, float r1, float x2, float y2, float r2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float distance = sqrtf(dx * dx + dy * dy);
    return distance < (r1 + r2);
}

// 处理物理碰撞
void handle_collision(Player* player, Puck* puck) {
    // 计算碰撞向量
    float dx = puck->x - player->x;
    float dy = puck->y - player->y;
    float distance = sqrtf(dx * dx + dy * dy);

    if (distance == 0) return;

    // 归一化
    dx /= distance;
    dy /= distance;

    // 最小距离
    float min_distance = player->radius + puck->radius;

    // 如果重叠，推开
    if (distance < min_distance) {
        float overlap = min_distance - distance;
        puck->x += dx * overlap * 0.5f;
        puck->y += dy * overlap * 0.5f;
        player->x -= dx * overlap * 0.5f;
        player->y -= dy * overlap * 0.5f;
    }

    // 计算相对速度
    float relative_vx = puck->vx - player->vx;
    float relative_vy = puck->vy - player->vy;

    // 沿碰撞法线的速度分量
    float velocity_along_normal = relative_vx * dx + relative_vy * dy;

    // 如果物体正在分离，不处理
    if (velocity_along_normal > 0) return;

    // 计算冲量
    float impulse = -(1.0f + COLLISION_DAMPING) * velocity_along_normal;
    impulse /= (1.0f / puck->radius + 1.0f / player->radius);

    // 应用冲量
    float ix = impulse * dx;
    float iy = impulse * dy;

    puck->vx += ix / puck->radius;
    puck->vy += iy / puck->radius;
    player->vx -= ix / player->radius;
    player->vy -= iy / player->radius;
}

// 重置游戏状态（进球后）
void reset_after_goal(void) {
    // 重置冰球位置
    puck.x = screen_center_x;
    puck.y = screen_center_y;
    puck.vx = 5.0f * (rand() % 2 ? 1 : -1);
    puck.vy = 5.0f * (rand() % 2 ? 1 : -1);

    // 重置玩家位置
    player1.x = field_x + field_width * 0.25f;
    player1.y = screen_center_y;
    player1.vx = player1.vy = 0;

    player2.x = field_x + field_width * 0.75f;
    player2.y = screen_center_y;
    player2.vx = player2.vy = 0;
}

// 更新游戏逻辑
void game_update(void) {

    frame_id  = (frame_id + 1) % FRAME_RATE;  // enter next frame

    float current_time = get_current_time();
    float delta_time = current_time - last_time;         //TODO: substitute this with a fix framerate
    last_time = current_time;
    // 限制delta_time，避免异常值
    if (delta_time > 0.1f) delta_time = 0.1f;

    // get historical input
    player1.target_x = load_local_input(frame_id).xin;
    player1.target_y = load_local_input(frame_id).yin;
    player2.target_x = load_remote_input(frame_id).xin;
    player2.target_y = load_remote_input(frame_id).yin;



    // 更新玩家1位置
    float dx = player1.target_x - player1.x;
    float dy = player1.target_y - player1.y;
    float distance = sqrtf(dx * dx + dy * dy);

    if (distance > 0) {
        dx /= distance;
        dy /= distance;
        float speed = fminf(distance, 15.0f);
        player1.vx = dx * speed;
        player1.vy = dy * speed;
    }
    else {
        player1.vx *= 0.9f;
        player1.vy *= 0.9f;
    }
    player1.x += player1.vx;
    player1.y += player1.vy;

    // 限制玩家在己方半场内
    if (player1.x > screen_center_x - player1.radius) {
        player1.x = screen_center_x - player1.radius;
        player1.vx = -player1.vx * 0.5f;
    }

    // 边界检查
    if (player1.x < field_x + player1.radius) {
        player1.x = field_x + player1.radius;
        player1.vx = -player1.vx * 0.5f;
    }
    if (player1.y < field_y + player1.radius) {
        player1.y = field_y + player1.radius;
        player1.vy = -player1.vy * 0.5f;
    }
    if (player1.y > field_y + field_height - player1.radius) {
        player1.y = field_y + field_height - player1.radius;
        player1.vy = -player1.vy * 0.5f;
    }

    // 更新AI控制（如果不是蓝牙模式）
    ai_update(delta_time);

    // 更新玩家2位置
    player2.x += player2.vx;
    player2.y += player2.vy;

    // 限制玩家2在己方半场内
    if (player2.x < screen_center_x + player2.radius) {
        player2.x = screen_center_x + player2.radius;
        player2.vx = -player2.vx * 0.5f;
    }

    // 边界检查
    if (player2.x > field_x + field_width - player2.radius) {
        player2.x = field_x + field_width - player2.radius;
        player2.vx = -player2.vx * 0.5f;
    }
    if (player2.y < field_y + player2.radius) {
        player2.y = field_y + player2.radius;
        player2.vy = -player2.vy * 0.5f;
    }
    if (player2.y > field_y + field_height - player2.radius) {
        player2.y = field_y + field_height - player2.radius;
        player2.vy = -player2.vy * 0.5f;
    }

    // 更新冰球位置
    puck.x += puck.vx;
    puck.y += puck.vy;

    // 应用摩擦力
    puck.vx *= FRICTION;
    puck.vy *= FRICTION;

    // 限制最大速度
    float puck_speed = sqrtf(puck.vx * puck.vx + puck.vy * puck.vy);
    if (puck_speed > MAX_SPEED) {
        puck.vx = puck.vx / puck_speed * MAX_SPEED;
        puck.vy = puck.vy / puck_speed * MAX_SPEED;
    }

    // 边界处理 - 现在函数已经声明了
    handle_boundaries();

    // 检查碰撞
    if (check_collision(player1.x, player1.y, player1.radius,
        puck.x, puck.y, puck.radius)) {
        handle_collision(&player1, &puck);
    }

    if (check_collision(player2.x, player2.y, player2.radius,
        puck.x, puck.y, puck.radius)) {
        handle_collision(&player2, &puck);
    }

    // 检查进球 - 现在函数已经声明了
    check_goals();

}

/* waiting for the start frame ,only do input exchange and frame_id update*/
void game_wait(void){
    frame_id = (frame_id + 1) % FRAME_RATE;
    
    if(frame_id == START_FRAME - 1){  //start on next frame
        game_state = GAME_PLAYING;
    }
}
// 处理边界碰撞
void handle_boundaries(void) {
    // 边界反弹（排除球门区域）
    int is_near_goal1 = (puck.y > goal1.y && puck.y < goal1.y + goal1.height);
    int is_near_goal2 = (puck.y > goal2.y && puck.y < goal2.y + goal2.height);

    if (puck.x < field_x + puck.radius) {
        // 左侧边界
        if (!is_near_goal1) {
            // 不在球门区域，正常反弹
            puck.x = field_x + puck.radius;
            puck.vx = -puck.vx * 0.9f;
            puck.vy += (rand() % 10 - 5) * 0.1f;
        }
    }

    if (puck.x > field_x + field_width - puck.radius) {
        // 右侧边界
        if (!is_near_goal2) {
            // 不在球门区域，正常反弹
            puck.x = field_x + field_width - puck.radius;
            puck.vx = -puck.vx * 0.9f;
            puck.vy += (rand() % 10 - 5) * 0.1f;
        }
    }

    // 上下边界
    if (puck.y < field_y + puck.radius) {
        puck.y = field_y + puck.radius;
        puck.vy = -puck.vy * 0.9f;
        puck.vx += (rand() % 10 - 5) * 0.1f;
    }
    if (puck.y > field_y + field_height - puck.radius) {
        puck.y = field_y + field_height - puck.radius;
        puck.vy = -puck.vy * 0.9f;
        puck.vx += (rand() % 10 - 5) * 0.1f;
    }
}

// 检查进球
void check_goals(void) {
    int goal_margin = 20; // 进球判定容差

    // 检查左侧进球（球门1）
    if (puck.x < field_x - puck.radius - goal_margin) {
        // 完全越过左侧边界
        if (puck.y > goal1.y - goal_margin && puck.y < goal1.y + goal1.height + goal_margin) {
            // 进球有效！
            player2.score++;
            printf("Goal! Player2 scores! Score: %d - %d\n", player1.score, player2.score);
            reset_after_goal();
        }
    }

    // 检查右侧进球（球门2）
    if (puck.x > field_x + field_width + puck.radius + goal_margin) {
        // 完全越过右侧边界
        if (puck.y > goal2.y - goal_margin && puck.y < goal2.y + goal2.height + goal_margin) {
            // 进球有效！
            player1.score++;
            printf("Goal! Player1 scores! Score: %d - %d\n", player1.score, player2.score);
            reset_after_goal();
        }
    }
}


void save_local_input(int frame_id, Input localin)
{
    printf("saving  local input. frame id %5d x %5f y %5f", frame_id, localin.xin, localin.yin);
    local_input_buffer[frame_id%INPUT_DELAY] = localin;
}

void save_remote_input(int frame_id, Input remotein)
{
    printf("saving remote input. frame id %5d x %5f y %5f", frame_id, remotein.xin, remotein.yin);
    remote_input_buffer[frame_id%INPUT_DELAY] = remotein;
}

Input load_local_input(int frame_id)
{
    printf("loading  local input. frame id %5d x %5f y %5f", frame_id, local_input_buffer[frame_id%INPUT_DELAY].xin, local_input_buffer[frame_id%INPUT_DELAY].yin);
    return local_input_buffer[frame_id%INPUT_DELAY];
}

Input load_remote_input(int frame_id)
{
    printf("loading remote input. frame id %5d x %5f y %5f", frame_id, remote_input_buffer[frame_id%INPUT_DELAY].xin, remote_input_buffer[frame_id%INPUT_DELAY].yin);
    return remote_input_buffer[frame_id%INPUT_DELAY];
}