#ifndef _AI_H_
#define _AI_H_

#include "game.h"

// AI状态
typedef enum {
    AI_DEFENSE,      // 防守模式
    AI_ATTACK,       // 进攻模式
    AI_RETREAT,      // 撤退模式
    AI_CLEAR         // 解围模式
} AIState;

// AI结构体
typedef struct {
    AIState state;
    float last_think_time;
    float strategy_x, strategy_y; // 策略目标位置
    int stuck_counter; // 卡住计数器
} AIController;

// AI初始化
void ai_init(void);

// AI更新
void ai_update(float delta_time);

// 设置AI是否启用
void ai_set_enabled(int enabled);

#endif /* _AI_H_ */
