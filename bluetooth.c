#include "bluetooth.h"
#include "../common/common.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>



/* ============================================================
 * Internal state
 * ============================================================ */

static int bt_fd = -1;
static int bt_connected = 0;
static int bt_role = -1; 

/* ============================================================
 * Init / status
 * ============================================================ */

int bluetooth_init(int role)
{
    char device[64] = {0};
    char dst_mac[32] = {0};

    bt_role = role;

    if (bt_role == 1) {
        /* CLIENT MODE */
        printf("Enter destination MAC (e.g. 00:11:22:33:44:55): ");
        fflush(stdout);

        if (scanf("%31s", dst_mac) != 1) {
            fprintf(stderr, "Failed to read MAC address\n");
            return -1;
        }

        /* Bind rfcomm device to remote MAC */
        snprintf(device, sizeof(device),
                 "rfcomm connect rfcomm0 %s 1", dst_mac);

        if (system(device) != 0) {
            fprintf(stderr, "rfcomm connect failed\n");
            return -1;
        }

        strcpy(device, "/dev/rfcomm0");
    } else if(bt_role = 0){
        /* HOST MODE */
        snprintf(device, sizeof(device),
                 "rfcomm listen rfcomm0 1 &");

        if (system(device) != 0) {
            fprintf(stderr, "rfcomm listen failed\n");
            return -1;
        }

        strcpy(device, "/dev/rfcomm0");
    } else{
        printf("invalid bt_role\n");
        return -1;
    }

    /* Open RFCOMM device */
    bt_fd = open(device, O_RDWR | O_NOCTTY);
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
    pkt.xin      = in.xin;
    pkt.yin      = in.yin;

    write(bt_fd, &pkt, sizeof(pkt));
}

/* ============================================================
 * Non-blocking receive (called from task manager)
 * ============================================================ */

void bluetooth_process(void)
{
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
            save_remote_input(pkt.frame_id, in);
        }
    }
}
