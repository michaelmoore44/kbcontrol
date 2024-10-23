#ifndef __CRC_H
#define __CRC_H

#include <stdio.h>
#include <stdint.h>

uint16_t crc_16 (uint16_t crc, const void *buffer, uint16_t len);

#endif
