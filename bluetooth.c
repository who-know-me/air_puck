#include "bluetooth.h"
#include "../common/common.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int bluetooth_fd = -1;
static BluetoothCallback bluetooth_callback = NULL;
static int is_connected = 0;

// 蓝牙初始化
int bluetooth_init(const char* device) {
    bluetooth_fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (bluetooth_fd < 0) {
        printf("Failed to open Bluetooth device: %s\n", device);
        return -1;
    }

    // 发送连接确认
    myWrite_nonblock(bluetooth_fd, "CONNECTED\n", 10);
    is_connected = 1;

    return bluetooth_fd;
}

// 设置蓝牙回调
void bluetooth_set_callback(BluetoothCallback callback) {
    bluetooth_callback = callback;
}

// 发送数据
void bluetooth_send(const char* data) {
    if (bluetooth_fd > 0 && is_connected) {
        myWrite_nonblock(bluetooth_fd, data, strlen(data));
    }
}

// 处理蓝牙事件
void bluetooth_process(void) {
    if (bluetooth_fd < 0 || !bluetooth_callback) return;

    char buffer[128];
    int n = myRead_nonblock(bluetooth_fd, buffer, sizeof(buffer) - 1);

    if (n <= 0) {
        printf("Bluetooth disconnected\n");
        is_connected = 0;
        return;
    }

    buffer[n] = '\0';
    bluetooth_callback(buffer);
}

// 获取连接状态
int bluetooth_is_connected(void) {
    return is_connected;
}

// 设置连接状态
void bluetooth_set_connected(int connected) {
    is_connected = connected;
}