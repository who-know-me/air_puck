#include "bluetooth.h"
#include "../common/common.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>

#define RFCOMM_TIMEOUT_MS 30000



/* ============================================================
 * Internal state
 * ============================================================ */

static int bt_fd = -1;
static int bt_connected = 0;
static int bt_role = -1; 

/* ============================================================
 * Init / status
 * ============================================================ */


static int wait_for_rfcomm(const char *dev, int timeout_ms)
{
    struct stat st;
    int elapsed = 0;

    while (elapsed < timeout_ms) {
        if (stat(dev, &st) == 0)
            return 0;   // device exists

        usleep(100 * 1000); // 100 ms
        elapsed += 100;
    }
    return -1;
}

int bluetooth_init(int role)
{
    char cmd[128];
    char dst_mac[32] = "D0:A4:6F:CF:B6:40";  //TODO: for test, dstmac is predefined
    const char *dev = "/dev/rfcomm0";

    bt_role = role;

    /* Clean up any previous rfcomm binding */
    system("rfcomm release rfcomm0 >/dev/null 2>&1");

    if (bt_role == 1) {
        /* CLIENT MODE */
        printf("Enter destination MAC (e.g. 00:11:22:33:44:55): \n");


        /*
        if (scanf("%31s", dst_mac) != 1) {
            fprintf(stderr, "Failed to read MAC address\n");
            return -1;
        }
        */

        printf("dest MAC %s\n", dst_mac);

        snprintf(cmd, sizeof(cmd),
                 "rfcomm -r connect rfcomm0 %s 2 &", dst_mac);

    } else if (bt_role == 0) {
        /* HOST MODE */
        snprintf(cmd, sizeof(cmd),
                 "rfcomm listen rfcomm0 2 &");

    } else {
        fprintf(stderr, "invalid bt_role\n");
        return -1;
    }

    if (system(cmd) != 0) {
        fprintf(stderr, "rfcomm command failed\n");
        return -1;
    }

    /* Wait until /dev/rfcomm0 appears */
    if (wait_for_rfcomm(dev, RFCOMM_TIMEOUT_MS) < 0) {
        fprintf(stderr, "rfcomm device not ready\n");
        return -1;
    }

    /* Open RFCOMM device */
    bt_fd = open(dev, O_RDWR | O_NOCTTY);
    if (bt_fd < 0) {
        perror("bluetooth open");
        return -1;
    }

    bt_connected = 1;
    return bt_fd;
}


/*
int bluetooth_init(const char* device)
{
    bt_fd = open(device, O_RDWR | O_NOCTTY);
    if (bt_fd < 0) {
        perror("bluetooth open");
        return -1;
    }

    bt_connected = 1;
    return bt_fd;
}
*/

int bluetooth_is_connected(void)
{
    return bt_connected;
}

/* ============================================================
 * Blocking handshake
 * ============================================================ */

int bluetooth_handshake_host(uint32_t start_frame)
{
    BtHello hello;

    hello.type        = BT_MSG_HELLO;
    hello.seed        = (uint32_t)time(NULL);
    hello.start_frame = start_frame;
    hello.input_delay = INPUT_DELAY;

    if (write(bt_fd, &hello, sizeof(hello)) != sizeof(hello)) {
        perror("bt write hello");
        return -1;
    }

    BtAck ack;
    if (read(bt_fd, &ack, sizeof(ack)) != sizeof(ack)) {
        perror("bt read ack");
        return -1;
    }

    if (ack.type != BT_MSG_ACK) {
        printf("invalid ACK\n");
        return -1;
    }

    /* shared seed */
    srand(hello.seed);

    printf("[BT] handshake OK (host)\n");
    return 0;
}

int bluetooth_handshake_client(uint32_t* out_start_frame)
{
    BtHello hello;

    if (read(bt_fd, &hello, sizeof(hello)) != sizeof(hello)) {
        perror("bt read hello");
        return -1;
    }

    if (hello.type != BT_MSG_HELLO) {
        printf("invalid HELLO\n");
        return -1;
    }

    if (hello.input_delay != INPUT_DELAY) {
        printf("input delay mismatch\n");
        return -1;
    }

    BtAck ack;
    ack.type = BT_MSG_ACK;
    write(bt_fd, &ack, sizeof(ack));

    srand(hello.seed);
    *out_start_frame = hello.start_frame;
    
    printf("[BT] handshake OK (client)\n");
    return 0;
}

/* ============================================================
 * In-game input exchange
 * ============================================================ */

void bluetooth_send_input(int frame_id, Input in)
{
    if (!bt_connected) return;

    BtInput pkt;
    pkt.type     = BT_MSG_INPUT;
    pkt.frame_id = frame_id;
    pkt.xin      = SCREEN_WIDTH - in.xin;       // project to p2 halfcourt
    pkt.yin      = in.yin;

    //printf("sending input: frame id %d, xy %f %f\n", frame_id, in.xin, in.yin);

    write(bt_fd, &pkt, sizeof(pkt));
}

/* ============================================================
 * Non-blocking receive (called from task manager)
 * ============================================================ */

void bluetooth_process(int fd)
{
    if(fd != bt_fd){
        printf("[bt process]fd mismatch\n");
        return;
    }
    if (!bt_connected) return;

    BtInput pkt;
    ssize_t n;

    while (1) {
        n = read(bt_fd, &pkt, sizeof(pkt));
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return;
            bt_connected = 0;
            return;
        }

        if (n == 0) {
            bt_connected = 0;
            return;
        }

        if (n != sizeof(pkt))
            return;

        if (pkt.type == BT_MSG_INPUT) {
            Input in;
            in.xin = pkt.xin;
            in.yin = pkt.yin;
            printf("saving  input: frame id %d xy %f %f\n", pkt.frame_id, in.xin, in.yin);
            save_remote_input(pkt.frame_id, in);
        }
    }
}
