#ifndef _GAME_H_
#define _GAME_H_

#include "../common/common.h"

// 游戏常量
#define PLAYER_RADIUS       30
#define PUCK_RADIUS         20
#define GOAL_WIDTH          150
#define GOAL_HEIGHT         30
#define GOAL_DEPTH          10
#define FIELD_PADDING       50
#define FRICTION            0.98f
#define MAX_SPEED           25.0f
#define COLLISION_DAMPING   0.8f
#define AI_THINK_INTERVAL   0.5f  // AI思考间隔（秒）

// 颜色定义
#define COLOR_BACKGROUND    FB_COLOR(0x33, 0x66, 0x99)  // 蓝色背景
#define COLOR_PLAYER1       FB_COLOR(0xFF, 0x00, 0x00)  // 红色 - 玩家1
#define COLOR_PLAYER2       FB_COLOR(0x00, 0xFF, 0x00)  // 绿色 - 玩家2
#define COLOR_PUCK          FB_COLOR(0xFF, 0xFF, 0xFF)  // 白色 - 冰球
#define COLOR_FIELD         FB_COLOR(0x66, 0xCC, 0xFF)  // 浅蓝 - 球场
#define COLOR_GOAL          FB_COLOR(0x99, 0x99, 0x99)  // 灰色 - 球门
#define COLOR_CENTER_LINE   FB_COLOR(0xFF, 0xFF, 0xFF)  // 白色 - 中线
#define COLOR_TEXT          FB_COLOR(0xFF, 0xFF, 0xFF)  // 白色文字

// 游戏状态
typedef enum {
    GAME_WAITING,
    GAME_PLAYING,
    GAME_PAUSED
} GameState;

// 玩家结构体
typedef struct {
    float x, y;           // 当前位置
    float target_x, target_y; // 目标位置
    float vx, vy;         // 速度
    int score;
    int radius;
    int color;
} Player;

// 冰球结构体
typedef struct {
    float x, y;
    float vx, vy;
    int radius;
    int color;
} Puck;

// 球门结构体
typedef struct {
    float x, y;
    int width, height;
    int depth;
    int color;
} Goal;

// 游戏全局变量声明
extern GameState game_state;
extern Player player1, player2;
extern Puck puck;
extern Goal goal1, goal2;
extern int screen_center_x, screen_center_y;
extern int field_width, field_height;
extern int field_x, field_y;

// 游戏初始化
void game_init(void);

// 游戏逻辑更新
void game_update(void);

// 游戏绘制
void game_draw(void);

// 碰撞检测
int check_collision(float x1, float y1, float r1, float x2, float y2, float r2);

// 碰撞处理
void handle_collision(Player *player, Puck *puck);

// 重置游戏状态（进球后）
void reset_after_goal(void);

#endif /* _GAME_H_ */
