#pragma once

#include <stdint.h>

uint32_t GetTexBufferSize(uint16_t wd, uint16_t ht, uint32_t fmt,
                          uint8_t mipmap, uint8_t maxlod);
