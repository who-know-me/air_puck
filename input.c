#include "input.h"
#include "common.h"
#include <stdio.h>
#include <unistd.h>

static int touch_fd = -1;
static TouchCallback touch_callback = NULL;

// 输入初始化
int input_init(const char* touch_device) {
    touch_fd = touch_init(touch_device);
    return touch_fd;
}

// 设置触摸回调
void input_set_touch_callback(TouchCallback callback) {
    touch_callback = callback;
}

// 处理输入事件
void input_process(void) {
    if (touch_fd < 0 || !touch_callback) return;

    int type, x, y, finger;
    type = touch_read(touch_fd, &x, &y, &finger);

    if (type != TOUCH_NO_EVENT) {
        touch_callback(x, y, type, finger);
    }
}