#ifndef _BLUETOOTH_H_
#define _BLUETOOTH_H_

#include <stdint.h>
#include "game.h"

#define BT_HOST 0
#define BT_CLIENT 1

/* ============================================================
 * Protocol definitions
 * ============================================================ */

enum {
    BT_MSG_HELLO = 1,
    BT_MSG_ACK   = 2,
    BT_MSG_INPUT = 3
};

typedef struct {
    uint8_t  type;
    uint32_t seed;
    uint32_t start_frame;
    uint32_t input_delay;
} __attribute__((packed)) BtHello;

typedef struct {
    uint8_t type;
} __attribute__((packed)) BtAck;

typedef struct {
    uint8_t  type;
    uint32_t frame_id;
    float    xin;
    float    yin;
} __attribute__((packed)) BtInput;




/* connection */
int bluetooth_init(int role);
int bluetooth_is_connected(void);

/* blocking handshake (call before task_loop) */
int bluetooth_handshake_host(uint32_t start_frame);
int bluetooth_handshake_client(uint32_t* out_start_frame);

/* in-game input exchange */
void bluetooth_send_input(int frame_id, Input in);
void bluetooth_process(void);   // call from file task callback

#endif /* _BLUETOOTH_H_ */
