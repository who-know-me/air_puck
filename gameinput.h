#ifndef _INPUT_H_
#define _INPUT_H_

#include "game.h"

typedef void (*TouchCallback)(int x, int y, int type, int finger);

int input_init(const char* touch_device);


void input_process(void);
void input_callback(int fd);

void on_touch(int x, int y, int type, int finger);

#endif /* _INPUT_H_ */
