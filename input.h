#ifndef _INPUT_H_
#define _INPUT_H_

#include "game.h"

// 触摸事件回调函数类型
typedef void (*TouchCallback)(int x, int y, int type, int finger);

// 输入初始化
int input_init(const char* touch_device);

// 设置触摸回调
void input_set_touch_callback(TouchCallback callback);

// 处理输入事件
void input_process(void);

#endif /* _INPUT_H_ */
