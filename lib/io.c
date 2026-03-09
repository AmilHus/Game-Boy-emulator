#include <apu.h>
#include <cpu.h>
#include <dma.h>
#include <gamepad.h>
#include <io.h>
#include <lcd.h>
#include <ppu.h>
#include <ram.h>
#include <timer.h>

static char serial_data[2];

u8 io_read(u16 address) {
  if (address == 0xFF00) {
    return gamepad_get_output();
  }

  if (address == 0xFF01) {
    return serial_data[0];
  }

  if (address == 0xFF02) {
    return serial_data[1];
  }

  if (BETWEEN(address, 0xFF04, 0xFF07)) {
    return timer_read(address);
  }

  if (address == 0xFF0F) {
    return cpu_get_int_flags();
  }

  if (BETWEEN(address, 0xFF10, 0xFF3F)) {
    return apu_read(address);
  }

  if (BETWEEN(address, 0xFF40, 0xFF4B)) {
    return lcd_read(address);
  }

  if (BETWEEN(address, 0xFF68, 0xFF6B)) {
    ppu_context *p_ctx = ppu_get_context();
    switch (address) {
    case 0xFF68:
      return p_ctx->bcps;
    case 0xFF69:
      return ppu_bcpd_read();
    case 0xFF6A:
      return p_ctx->ocps;
    case 0xFF6B:
      return ppu_ocpd_read();
    }
  }

  if (address == 0xFF4F) {
    return ppu_get_context()->vbk | 0xFE;
  }

  if (BETWEEN(address, 0xFF51, 0xFF55)) {
    return dma_cgb_read(address);
  }

  if (address == 0xFF70) {
    return wram_get_svbk() | 0xF8;
  }

  // printf("UNSUPPORTED bus_read(%04X)\n", address);
  return 0;
}

void io_write(u16 address, u8 value) {
  if (address == 0xFF00) {
    gamepad_set_sel(value);
    return;
  }

  if (address == 0xFF01) {
    serial_data[0] = value;
    return;
  }

  if (address == 0xFF02) {
    serial_data[1] = value;
    return;
  }

  if (BETWEEN(address, 0xFF04, 0xFF07)) {
    timer_write(address, value);
    return;
  }

  if (address == 0xFF0F) {
    cpu_set_int_flags(value);
    return;
  }

  if (BETWEEN(address, 0xFF10, 0xFF3F)) {
    apu_write(address, value);
    return;
  }

  if (BETWEEN(address, 0xFF40, 0xFF4B)) {
    lcd_write(address, value);
    return;
  }

  if (BETWEEN(address, 0xFF68, 0xFF6B)) {
    ppu_context *p_ctx = ppu_get_context();
    switch (address) {
    case 0xFF68:
      p_ctx->bcps = value;
      break;
    case 0xFF69:
      ppu_bcpd_write(value);
      break;
    case 0xFF6A:
      p_ctx->ocps = value;
      break;
    case 0xFF6B:
      ppu_ocpd_write(value);
      break;
    }
    return;
  }

  if (address == 0xFF4F) {
    ppu_get_context()->vbk = value & 1;
    return;
  }

  if (BETWEEN(address, 0xFF51, 0xFF55)) {
    dma_cgb_write(address, value);
    return;
  }

  if (address == 0xFF70) {
    wram_set_svbk(value);
    return;
  }

  // printf("UNSUPPORTED bus_write(%04X)\n", address);
}