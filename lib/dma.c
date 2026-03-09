#include <bus.h>
#include <dma.h>
#include <ppu.h>

typedef struct {
  bool active;
  u8 byte;
  u8 value;
  u8 start_delay;

  // CGB DMA
  u16 src;
  u16 dest;
  u8 len;
  bool active_cgb;
} dma_context;

static dma_context ctx;

void dma_start(u8 start) {
  ctx.active = true;
  ctx.byte = 0;
  ctx.start_delay = 2;
  ctx.value = start;
}

void dma_tick() {
  if (!ctx.active) {
    return;
  }

  if (ctx.start_delay) {
    ctx.start_delay--;
    return;
  }

  ppu_oam_write(ctx.byte, bus_read((ctx.value * 0x100) + ctx.byte));
  ctx.byte++;
  ctx.active = ctx.byte < 0xA0;
}

bool dma_transferring() { return ctx.active || ctx.active_cgb; }

void dma_cgb_write(u16 address, u8 value) {
  switch (address) {
  case 0xFF51:
    ctx.src = (ctx.src & 0x00FF) | (value << 8);
    break;
  case 0xFF52:
    ctx.src = (ctx.src & 0xFF00) | (value & 0xF0);
    break;
  case 0xFF53:
    ctx.dest = 0x8000 | ((ctx.dest & 0x00FF) | ((value & 0x1F) << 8));
    break;
  case 0xFF54:
    ctx.dest = 0x8000 | ((ctx.dest & 0xFF00) | (value & 0xF0));
    break;
  case 0xFF55: {
    u8 len = (value & 0x7F) + 1;
    bool mode = value & 0x80;

    if (ctx.active_cgb && !mode) {
      ctx.active_cgb = false;
      ctx.len = 0xFF; // Specific behavior for cancelling HDMA
      return;
    }

    ctx.len = value & 0x7F;
    ctx.active_cgb = mode;

    if (!mode) {
      // GDMA - Immediate transfer
      for (int i = 0; i < len * 16; i++) {
        bus_write(ctx.dest + i, bus_read(ctx.src + i));
      }
      ctx.len = 0xFF; // Finished
      ctx.active_cgb = false;
    }
  } break;
  }
}

u8 dma_cgb_read(u16 address) {
  if (address == 0xFF55) {
    return (ctx.active_cgb ? 0 : 0x80) | ctx.len;
  }
  return 0xFF;
}