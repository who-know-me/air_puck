#ifndef _BLUETOOTH_H_
#define _BLUETOOTH_H_

#include "game.h"

// 蓝牙数据回调函数类型
typedef void (*BluetoothCallback)(const char* data);

// 蓝牙初始化
int bluetooth_init(const char* device);

// 设置蓝牙回调
void bluetooth_set_callback(BluetoothCallback callback);

// 发送数据
void bluetooth_send(const char* data);

// 处理蓝牙事件
void bluetooth_process(void);

// 获取连接状态
int bluetooth_is_connected(void);

// 设置连接状态
void bluetooth_set_connected(int connected);

#endif /* _BLUETOOTH_H_ */
