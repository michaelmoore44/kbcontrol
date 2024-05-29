

#ifndef SPLIT_H
#define SPLIT_H

#include "pico/util/queue.h"

void split_init(void);
void split_task(void);

queue_t* split_get_keys_buf(void);

void split_tx_msg(uint8_t* msg, uint8_t msg_len);
void split_tx_keys(uint8_t* keys, uint8_t msg_len);

#endif //end SPLIT_H

