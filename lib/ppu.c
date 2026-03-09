#include <lcd.h>
#include <ppu.h>
#include <ppu_sm.h>
#include <string.h>

static ppu_context ctx;

ppu_context *ppu_get_context() { return &ctx; }

void ppu_init() {
  ctx.current_frame = 0;
  ctx.line_ticks = 0;
  ctx.video_buffer = malloc(YRES * XRES * sizeof(u32));

  ctx.pfc.line_x = 0;
  ctx.pfc.pushed_x = 0;
  ctx.pfc.fetch_x = 0;
  ctx.pfc.pixel_fifo.size = 0;
  ctx.pfc.pixel_fifo.head = ctx.pfc.pixel_fifo.tail = NULL;
  ctx.pfc.cur_fetch_state = FS_TILE;

  ctx.line_sprites = 0;
  ctx.fetched_entry_count = 0;
  ctx.window_line = 0;

  lcd_init();
  LCDS_MODE_SET(MODE_OAM);

  memset(ctx.oam_ram, 0, sizeof(ctx.oam_ram));
  memset(ctx.vram, 0, sizeof(ctx.vram));
  memset(ctx.video_buffer, 0, YRES * XRES * sizeof(u32));
  ctx.vbk = 0;
}

void ppu_tick() {
  ctx.line_ticks++;

  switch (LCDS_MODE) {
  case MODE_OAM:
    ppu_mode_oam();
    break;
  case MODE_XFER:
    ppu_mode_xfer();
    break;
  case MODE_VBLANK:
    ppu_mode_vblank();
    break;
  case MODE_HBLANK:
    ppu_mode_hblank();
    break;
  }
}

void ppu_oam_write(u16 address, u8 value) {
  if (address >= 0xFE00) {
    address -= 0xFE00;
  }

  u8 *p = (u8 *)ctx.oam_ram;
  p[address] = value;
}

u8 ppu_oam_read(u16 address) {
  if (address >= 0xFE00) {
    address -= 0xFE00;
  }

  u8 *p = (u8 *)ctx.oam_ram;
  return p[address];
}

void ppu_vram_write(u16 address, u8 value) {
  ctx.vram[ctx.vbk & 1][address - 0x8000] = value;
}

u8 ppu_vram_read(u16 address) {
  return ctx.vram[ctx.vbk & 1][address - 0x8000];
}

u32 ppu_get_color(u8 *palette_ram, u8 pal_idx, u8 color_idx) {
  u16 r16 = palette_ram[pal_idx * 8 + color_idx * 2];
  u16 g16 = palette_ram[pal_idx * 8 + color_idx * 2 + 1];
  u16 color = (g16 << 8) | r16;

  u8 r = (color & 0x1F);
  u8 g = (color >> 5) & 0x1F;
  u8 b = (color >> 10) & 0x1F;

  // Convert 5-bit to 8-bit
  u32 r8 = (r << 3) | (r >> 2);
  u32 g8 = (g << 3) | (g >> 2);
  u32 b8 = (b << 3) | (b >> 2);

  return 0xFF000000 | (r8 << 16) | (g8 << 8) | b8;
}

void ppu_bcpd_write(u8 value) {
  u8 idx = ctx.bcps & 0x3F;
  ctx.bg_palette_ram[idx] = value;
  if (ctx.bcps & 0x80) {
    ctx.bcps = (ctx.bcps & 0x80) | ((idx + 1) & 0x3F);
  }
}

u8 ppu_bcpd_read() { return ctx.bg_palette_ram[ctx.bcps & 0x3F]; }

void ppu_ocpd_write(u8 value) {
  u8 idx = ctx.ocps & 0x3F;
  ctx.obj_palette_ram[idx] = value;
  if (ctx.ocps & 0x80) {
    ctx.ocps = (ctx.ocps & 0x80) | ((idx + 1) & 0x3F);
  }
}

u8 ppu_ocpd_read() { return ctx.obj_palette_ram[ctx.ocps & 0x3F]; }
