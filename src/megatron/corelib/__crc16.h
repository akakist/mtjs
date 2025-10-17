#pragma once
#ifndef _MSC_VER
#include <inttypes.h>
#endif


#include <stddef.h>
#include <stdint.h>

/** CRC table for the CRC-16. The poly is 0x8005 (x^16 + x^15 + x^2 + 1) */
uint16_t
__crc16(uint16_t crc, const void *buf, size_t size);

