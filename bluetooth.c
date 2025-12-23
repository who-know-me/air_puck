#include "bluetooth.h"
#include "../common/common.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int bluetooth_fd = -1;
static BluetoothCallback bluetooth_callback = NULL;
static int is_connected = 0;

// ?????????
int bluetooth_init(const char* device) {
    bluetooth_fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (bluetooth_fd < 0) {
        printf("Failed to open Bluetooth device: %s\n", device);
        return -1;
    }

    // ???????????
    myWrite_nonblock(bluetooth_fd, "CONNECTED\n", 10);
    is_connected = 1;

    return bluetooth_fd;
}

// ???????????
void bluetooth_set_callback(BluetoothCallback callback) {
    bluetooth_callback = callback;
}

// ????????
void bluetooth_send(const char* data) {
    if (bluetooth_fd > 0 && is_connected) {
        myWrite_nonblock(bluetooth_fd, data, strlen(data));
    }
}

// ???????????
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

// ?????????
int bluetooth_is_connected(void) {
    return is_connected;
}

// ??????????
void bluetooth_set_connected(int connected) {
    is_connected = connected;
}