#pragma once

#include <common.h>

void dma_start(u8 start);
void dma_tick();

bool dma_transferring();

void dma_cgb_write(u16 address, u8 value);
u8 dma_cgb_read(u16 address);
