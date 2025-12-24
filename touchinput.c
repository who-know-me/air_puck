
#include "../common/common.h"
#include <stdio.h>
//#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <input.h>
#include "gameinput.h"
#include <unistd.h>
#include "game.h"
#include "bluetooth.h"


static struct finger_info{
	int x;
	int y;
	int event;
} infos[FINGER_NUM_MAX];
static int cur_slot = 0;

static Input 

int touch_init(char *dev)
{
	int fd = open(dev, O_RDONLY | O_NONBLOCK);
	if(fd < 0){
		printf("touch_init open %s error!errno = %d\n", dev, errno);
		return -1;
	}
	return fd;
}

/*return:
	TOUCH_NO_EVENT
	TOUCH_PRESS
	TOUCH_MOVE
	TOUCH_RELEASE
	TOUCH_ERROR
	x: [0 ~ SCREEN_WIDTH)
	y: [0 ~ SCREEN_HEIGHT)
	finger: 0,1,2,3,4
*/

#define ADJUST_X(n) ((n*SCREEN_WIDTH)>>12)  /* (n*screen_width/4096) */
#define ADJUST_Y(n) ((n*SCREEN_HEIGHT)>>12)  /* (n*screen_height/4096) */

int touch_read(int touch_fd, int *x, int *y, int *finger)
{
	struct input_event data;
	int n, ret;
	if((n = read(touch_fd, &data, sizeof(data))) != sizeof(data)){
		//printf("touch_read error %d, errno=%d\n", n, errno);
		return TOUCH_NO_EVENT;
	}
//	printf("event read: type-code-value = %d-%d-%d\n", data.type, data.code, data.value);
	switch(data.type)
	{
	case EV_ABS:
		switch(data.code)
		{
		case ABS_MT_SLOT:
			if(data.value >= 0 && data.value < FINGER_NUM_MAX) {
				int old = cur_slot;
				cur_slot = data.value;
				if(infos[old].event != TOUCH_NO_EVENT) {
					*x = infos[old].x;
					*y = infos[old].y;
					*finger = old;
					ret = infos[old].event;
					infos[old].event = TOUCH_NO_EVENT;
					return ret;
				}
			}
			break;
		case ABS_MT_TRACKING_ID:
			if(data.value == -1){
				*x = infos[cur_slot].x;
				*y = infos[cur_slot].y;
				*finger = cur_slot;
				infos[cur_slot].event = TOUCH_NO_EVENT;
				return TOUCH_RELEASE;
			}
			else{
				infos[cur_slot].event = TOUCH_PRESS;
			}
			break;
		case ABS_MT_POSITION_X:
			infos[cur_slot].x = ADJUST_X(data.value);
			if(infos[cur_slot].event != TOUCH_PRESS) {
				infos[cur_slot].event = TOUCH_MOVE;
			}
			break;
		case ABS_MT_POSITION_Y:
			infos[cur_slot].y = ADJUST_Y(data.value);
			if(infos[cur_slot].event != TOUCH_PRESS) {
				infos[cur_slot].event = TOUCH_MOVE;
			}
			break;
		}
		break;
	case EV_SYN:
		switch(data.code)
		{
		case SYN_REPORT:
			if(infos[cur_slot].event != TOUCH_NO_EVENT){
				*x = infos[cur_slot].x;
				*y = infos[cur_slot].y;
				*finger = cur_slot;
				ret = infos[cur_slot].event;
				infos[cur_slot].event = TOUCH_NO_EVENT;
				return ret;
			}
			break;
		}
		break;
	}
	return TOUCH_NO_EVENT;
}

static int touch_fd = -1;

int input_init(const char* touch_device) {
    touch_fd = touch_init(touch_device);
    return touch_fd;
}


// 触摸事件回调
void on_touch(int x, int y, int type, int finger) {
	static Input touchinput;
    switch (type) {
    case TOUCH_PRESS:
    case TOUCH_MOVE:
        if (finger == 0 && x < screen_center_x) {
            // 直接设置目标位置，不要添加延迟或平滑
			touchinput.xin = x; touchinput.yin = y;
			save_local_input(frame_id, touchinput);  //save input

            // 如果蓝牙已连接，发送位置信息
            if (is_bluetooth_mode) {
				bluetooth_send_input(frame_id, touchinput);
            }
        }
        break;

    case TOUCH_RELEASE:
        if (finger == 0) {
			touchinput.xin = player1.x; touchinput.yin = player1.y;  // auto stop when finger is released
			save_local_input(frame_id, touchinput);              
        }
        break;

    case TOUCH_ERROR:  // shouldn't happen in non-blocking read
        printf("Touch device error\n");
        break;
	case TOUCH_NO_EVENT: // copy last input
		touchinput.xin = load_local_input((frame_id -1 + FRAME_RATE) % FRAME_RATE).xin;
		touchinput.yin = load_local_input((frame_id -1 + FRAME_RATE) % FRAME_RATE).yin;
		save_local_input(frame_id, touchinput);
		if(is_bluetooth_mode){
			bluetooth_send_input(frame_id, touchinput);
		}
		break;
    }
}

/*this is called at the start of a frame to guaranee an available input for the frame*/
void input_process(void) {  
    if (touch_fd < 0){
		return;
	} 

    int type, x, y, finger;
    type = touch_read(touch_fd, &x, &y, &finger);
          								
    on_touch(x, y, type, finger);	// need at least one input for this frame .so call on_touch whatever type is
}

/*this is called whenever touch_fd is available to read new input for current frame*/
void input_callback(int fd) {
    if (fd != touch_fd ){
		printf("fd != touch_fd\n");
		return;
	} 

    int type, x, y, finger;
    type = touch_read(touch_fd, &x, &y, &finger);

    //printf("[input process]read touch finished\n");
    if (type != TOUCH_NO_EVENT) {  // no need to update input buffer when there's no event
        on_touch(x, y, type, finger);
    }
}

