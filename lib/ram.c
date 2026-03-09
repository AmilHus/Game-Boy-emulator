#include <ram.h>

typedef struct {
  u8 wram[8][0x1000];
  u8 hram[0x80];
  u8 svbk;
} ram_context;

static ram_context ctx;

u8 wram_read(u16 address) {
  address -= 0xC000;

  if (address < 0x1000) {
    return ctx.wram[0][address];
  }

  u8 bank = ctx.svbk & 0x7;
  if (bank == 0)
    bank = 1;

  return ctx.wram[bank][address - 0x1000];
}

void wram_write(u16 address, u8 value) {
  address -= 0xC000;

  if (address < 0x1000) {
    ctx.wram[0][address] = value;
    return;
  }

  u8 bank = ctx.svbk & 0x7;
  if (bank == 0)
    bank = 1;

  ctx.wram[bank][address - 0x1000] = value;
}

u8 hram_read(u16 address) {
  address -= 0xFF80;

  return ctx.hram[address];
}

void hram_write(u16 address, u8 value) {
  address -= 0xFF80;

  ctx.hram[address] = value;
}

u8 wram_get_svbk() { return ctx.svbk; }

void wram_set_svbk(u8 value) { ctx.svbk = value & 0x7; }