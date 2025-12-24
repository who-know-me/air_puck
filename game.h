#ifndef _GAME_H_
#define _GAME_H_

#include "../common/common.h"

// ��Ϸ����
#define PLAYER_RADIUS       30
#define PUCK_RADIUS         20
#define GOAL_WIDTH          150
#define GOAL_HEIGHT         30
#define GOAL_DEPTH          10
#define FIELD_PADDING       50
#define FRICTION            0.98f
#define MAX_SPEED           25.0f
#define COLLISION_DAMPING   0.8f
#define AI_THINK_INTERVAL   0.5f  // AI˼��������룩

#define FRAME_RATE 60       // 60 frames per second
#define INPUT_DELAY 3       //delay input for 3 frames
#define START_FRAME 20

// ��ɫ����
#define COLOR_BACKGROUND    FB_COLOR(0x33, 0x66, 0x99)  // ��ɫ����
#define COLOR_PLAYER1       FB_COLOR(0xFF, 0x00, 0x00)  // ��ɫ - ���1
#define COLOR_PLAYER2       FB_COLOR(0x00, 0xFF, 0x00)  // ��ɫ - ���2
#define COLOR_PUCK          FB_COLOR(0xFF, 0xFF, 0xFF)  // ��ɫ - ����
#define COLOR_FIELD         FB_COLOR(0x66, 0xCC, 0xFF)  // ǳ�� - ��
#define COLOR_GOAL          FB_COLOR(0x99, 0x99, 0x99)  // ��ɫ - ����
#define COLOR_CENTER_LINE   FB_COLOR(0xFF, 0xFF, 0xFF)  // ��ɫ - ����
#define COLOR_TEXT          FB_COLOR(0xFF, 0xFF, 0xFF)  // ��ɫ����

typedef enum {
    GAME_WAITING,
    GAME_PLAYING,
    GAME_PAUSED
} GameState;

typedef struct {
    float x, y;           // ��ǰλ��
    float target_x, target_y; // Ŀ��λ��
    float vx, vy;         // �ٶ�
    int score;
    int radius;
    int color;
} Player;

typedef struct {
    float x, y;
    float vx, vy;
    int radius;
    int color;
} Puck;


typedef struct {
    float xin;
    float yin;
}Input, *InputPtr; 

// ���Žṹ��
typedef struct {
    float x, y;
    int width, height;
    int depth;
    int color;
} Goal;

extern GameState game_state;
extern Player player1, player2;
extern Puck puck;
extern Goal goal1, goal2;
extern int screen_center_x, screen_center_y;
extern int field_width, field_height;
extern int field_x, field_y;

extern int frame_id, start_frame;
extern Input local_input_buffer[INPUT_DELAY], remote_input_buffer[INPUT_DELAY]; 


void game_init(void);

void game_update(void);

void game_wait(void);

void handle_boundaries(void); 

void check_goals(void);
	
void game_draw(void);

int check_collision(float x1, float y1, float r1, float x2, float y2, float r2);

void handle_collision(Player *player, Puck *puck);

void reset_after_goal(void);

void save_local_input(int frame_id, Input localin);
void save_remote_input(int frame_id, Input remotein);

Input load_local_input(int frame_id);
Input load_remote_input(int frame_id);

#endif /* _GAME_H_ */
