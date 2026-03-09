#include <bus.h>
#include <cart.h>
#include <lcd.h>
#include <ppu.h>
#include <stdlib.h>

bool window_visible() {
  return LCDC_WIN_ENABLE && lcd_get_context()->win_x >= 0 &&
         lcd_get_context()->win_x <= 166 && lcd_get_context()->win_y >= 0 &&
         lcd_get_context()->win_y < YRES;
}

void pixel_fifo_push(u32 value) {
  fifo_entry *next = malloc(sizeof(fifo_entry));
  next->next = NULL;
  next->value = value;

  if (!ppu_get_context()->pfc.pixel_fifo.head) {
    // first entry...
    ppu_get_context()->pfc.pixel_fifo.head =
        ppu_get_context()->pfc.pixel_fifo.tail = next;
  } else {
    ppu_get_context()->pfc.pixel_fifo.tail->next = next;
    ppu_get_context()->pfc.pixel_fifo.tail = next;
  }

  ppu_get_context()->pfc.pixel_fifo.size++;
}

u32 pixel_fifo_pop() {
  if (ppu_get_context()->pfc.pixel_fifo.size <= 0) {
    fprintf(stderr, "ERR IN PIXEL FIFO!\n");
    exit(-8);
  }

  fifo_entry *popped = ppu_get_context()->pfc.pixel_fifo.head;
  ppu_get_context()->pfc.pixel_fifo.head = popped->next;
  ppu_get_context()->pfc.pixel_fifo.size--;

  u32 val = popped->value;
  free(popped);

  return val;
}

u32 fetch_sprite_pixels(int bit, u32 color, u8 bg_color) {
  ppu_context *ctx = ppu_get_context();
  for (int i = 0; i < ctx->fetched_entry_count; i++) {
    int sp_x =
        (ctx->fetched_entries[i].x - 8) + ((lcd_get_context()->scroll_x % 8));

    if (sp_x + 8 < ctx->pfc.fifo_x) {
      continue;
    }

    int offset = ctx->pfc.fifo_x - sp_x;

    if (offset < 0 || offset > 7) {
      continue;
    }

    bit = (7 - offset);

    if (ctx->fetched_entries[i].f_x_flip) {
      bit = offset;
    }

    u8 hi = !!(ctx->pfc.fetch_entry_data[i * 2] & (1 << bit));
    u8 lo = !!(ctx->pfc.fetch_entry_data[(i * 2) + 1] & (1 << bit)) << 1;

    bool bg_priority = ctx->fetched_entries[i].f_bgp;

    if (!(hi | lo)) {
      continue;
    }

    if (cart_is_cgb()) {
      u8 pal = ctx->fetched_entries[i].f_cgb_pn;
      color = ppu_get_color(ctx->obj_palette_ram, pal, hi | lo);
      if (hi | lo)
        break;
    } else {
      if (!bg_priority || bg_color == 0) {
        color = (ctx->fetched_entries[i].f_pn)
                    ? lcd_get_context()->sp2_colors[hi | lo]
                    : lcd_get_context()->sp1_colors[hi | lo];

        if (hi | lo) {
          break;
        }
      }
    }
  }

  return color;
}

bool pipeline_fifo_add() {
  if (ppu_get_context()->pfc.pixel_fifo.size > 8) {
    // fifo is full!
    return false;
  }

  int x =
      ppu_get_context()->pfc.fetch_x - (8 - (lcd_get_context()->scroll_x % 8));

  for (int i = 0; i < 8; i++) {
    int bit = 7 - i;
    u8 attrs = ppu_get_context()->pfc.bgw_fetch_attrs;

    if (cart_is_cgb() && (attrs & (1 << 5))) { // Horizontal Flip
      bit = i;
    }

    u8 hi = !!(ppu_get_context()->pfc.bgw_fetch_data[1] & (1 << bit));
    u8 lo = !!(ppu_get_context()->pfc.bgw_fetch_data[2] & (1 << bit)) << 1;
    u32 color = lcd_get_context()->bg_colors[hi | lo];

    if (cart_is_cgb()) {
      u8 pal = attrs & 0x7;
      color = ppu_get_color(ppu_get_context()->bg_palette_ram, pal, hi | lo);
    }

    if (!LCDC_BGW_ENABLE) {
      color = lcd_get_context()->bg_colors[0];
    }

    if (LCDC_OBJ_ENABLE) {
      color = fetch_sprite_pixels(bit, color, hi | lo);
    }

    if (x >= 0) {
      pixel_fifo_push(color);
      ppu_get_context()->pfc.fifo_x++;
    }
  }

  return true;
}

void pipeline_load_sprite_tile() {
  oam_line_entry *le = ppu_get_context()->line_sprites;

  while (le) {
    int sp_x = (le->entry.x - 8) + (lcd_get_context()->scroll_x % 8);

    if ((sp_x >= ppu_get_context()->pfc.fetch_x &&
         sp_x < ppu_get_context()->pfc.fetch_x + 8) ||
        ((sp_x + 8) >= ppu_get_context()->pfc.fetch_x &&
         (sp_x + 8) < ppu_get_context()->pfc.fetch_x + 8)) {
      // need to add entry
      ppu_get_context()
          ->fetched_entries[ppu_get_context()->fetched_entry_count++] =
          le->entry;
    }

    le = le->next;

    if (!le || ppu_get_context()->fetched_entry_count >= 3) {
      // max checking 3 sprites on pixels
      break;
    }
  }
}

void pipeline_load_sprite_data(u8 offset) {
  int cur_y = lcd_get_context()->ly;
  u8 sprite_height = LCDC_OBJ_HEIGHT;

  for (int i = 0; i < ppu_get_context()->fetched_entry_count; i++) {
    u8 ty = ((cur_y + 16) - ppu_get_context()->fetched_entries[i].y) * 2;

    if (ppu_get_context()->fetched_entries[i].f_y_flip) {
      // flipped upside down...
      ty = ((sprite_height * 2) - 2) - ty;
    }

    u8 tile_index = ppu_get_context()->fetched_entries[i].tile;

    if (sprite_height == 16) {
      tile_index &= ~(1); // remove last bit...
    }

    u8 bank = 0;
    if (cart_is_cgb()) {
      bank = ppu_get_context()->fetched_entries[i].f_cgb_vram_bank;
    }

    ppu_get_context()->pfc.fetch_entry_data[(i * 2) + offset] =
        ppu_get_context()->vram[bank][(tile_index * 16) + ty + offset];
  }
}

void pipeline_load_window_tile() {
  if (!window_visible()) {
    return;
  }

  u8 window_y = lcd_get_context()->win_y;

  if (ppu_get_context()->pfc.fetch_x + 7 >= lcd_get_context()->win_x &&
      ppu_get_context()->pfc.fetch_x + 7 <=
          lcd_get_context()->win_x + XRES + 14) {
    if (lcd_get_context()->ly >= window_y &&
        lcd_get_context()->ly < window_y + YRES) {
      u8 w_tile_y = ppu_get_context()->window_line / 8;

      ppu_get_context()->pfc.bgw_fetch_data[0] = bus_read(
          LCDC_WIN_MAP_AREA +
          ((ppu_get_context()->pfc.fetch_x + 7 - lcd_get_context()->win_x) /
           8) +
          (w_tile_y * 32));

      ppu_get_context()->pfc.tile_y = (ppu_get_context()->window_line % 8) * 2;

      if (cart_is_cgb()) {
        ppu_get_context()->pfc.bgw_fetch_attrs =
            ppu_get_context()->vram[1][LCDC_WIN_MAP_AREA - 0x8000 +
                                       ((ppu_get_context()->pfc.fetch_x + 7 -
                                         lcd_get_context()->win_x) /
                                        8) +
                                       (w_tile_y * 32)];
      }
    }
  }
}

void pipeline_fetch() {
  switch (ppu_get_context()->pfc.cur_fetch_state) {
  case FS_TILE: {
    ppu_get_context()->fetched_entry_count = 0;
    ppu_get_context()->pfc.bgw_fetch_attrs = 0;

    if (LCDC_BGW_ENABLE) {
      ppu_get_context()->pfc.bgw_fetch_data[0] =
          bus_read(LCDC_BG_MAP_AREA + (ppu_get_context()->pfc.map_x / 8) +
                   (((ppu_get_context()->pfc.map_y / 8)) * 32));

      if (cart_is_cgb()) {
        ppu_get_context()->pfc.bgw_fetch_attrs =
            ppu_get_context()
                ->vram[1][LCDC_BG_MAP_AREA - 0x8000 +
                          (ppu_get_context()->pfc.map_x / 8) +
                          (((ppu_get_context()->pfc.map_y / 8)) * 32)];
      }

      pipeline_load_window_tile();
    }

    if (LCDC_OBJ_ENABLE && ppu_get_context()->line_sprites) {
      pipeline_load_sprite_tile();
    }

    ppu_get_context()->pfc.cur_fetch_state = FS_DATA0;
    ppu_get_context()->pfc.fetch_x += 8;
  } break;

  case FS_DATA0: {
    u8 bank = 0;
    if (cart_is_cgb()) {
      bank = (ppu_get_context()->pfc.bgw_fetch_attrs >> 3) & 1;
    }

    u16 tile_index = ppu_get_context()->pfc.bgw_fetch_data[0];
    u16 tile_addr;

    if (LCDC_BGW_DATA_AREA == 0x8800) {
      tile_addr = 0x1000 + (i8)tile_index * 16;
    } else {
      tile_addr = tile_index * 16;
    }

    u8 ty = ppu_get_context()->pfc.tile_y;
    if (cart_is_cgb() && (ppu_get_context()->pfc.bgw_fetch_attrs & (1 << 6))) {
      // Vertical Flip
      ty = (7 - (ty / 2)) * 2;
    }

    ppu_get_context()->pfc.bgw_fetch_data[1] =
        ppu_get_context()->vram[bank][tile_addr + ty];

    pipeline_load_sprite_data(0);

    ppu_get_context()->pfc.cur_fetch_state = FS_DATA1;
  } break;

  case FS_DATA1: {
    u8 bank = 0;
    if (cart_is_cgb()) {
      bank = (ppu_get_context()->pfc.bgw_fetch_attrs >> 3) & 1;
    }

    u16 tile_index = ppu_get_context()->pfc.bgw_fetch_data[0];
    u16 tile_addr;

    if (LCDC_BGW_DATA_AREA == 0x8800) {
      tile_addr = 0x1000 + (i8)tile_index * 16;
    } else {
      tile_addr = tile_index * 16;
    }

    u8 ty = ppu_get_context()->pfc.tile_y;
    if (cart_is_cgb() && (ppu_get_context()->pfc.bgw_fetch_attrs & (1 << 6))) {
      // Vertical Flip
      ty = (7 - (ty / 2)) * 2;
    }

    ppu_get_context()->pfc.bgw_fetch_data[2] =
        ppu_get_context()->vram[bank][tile_addr + ty + 1];

    pipeline_load_sprite_data(1);

    ppu_get_context()->pfc.cur_fetch_state = FS_IDLE;

  } break;

  case FS_IDLE: {
    ppu_get_context()->pfc.cur_fetch_state = FS_PUSH;
  } break;

  case FS_PUSH: {
    if (pipeline_fifo_add()) {
      ppu_get_context()->pfc.cur_fetch_state = FS_TILE;
    }

  } break;
  }
}

void pipeline_push_pixel() {
  if (ppu_get_context()->pfc.pixel_fifo.size > 8) {
    u32 pixel_data = pixel_fifo_pop();

    if (ppu_get_context()->pfc.line_x >= (lcd_get_context()->scroll_x % 8)) {
      ppu_get_context()->video_buffer[ppu_get_context()->pfc.pushed_x +
                                      (lcd_get_context()->ly * XRES)] =
          pixel_data;

      ppu_get_context()->pfc.pushed_x++;
    }

    ppu_get_context()->pfc.line_x++;
  }
}

void pipeline_process() {
  ppu_get_context()->pfc.map_y =
      (lcd_get_context()->ly + lcd_get_context()->scroll_y);
  ppu_get_context()->pfc.map_x =
      (ppu_get_context()->pfc.fetch_x + lcd_get_context()->scroll_x);
  ppu_get_context()->pfc.tile_y =
      ((lcd_get_context()->ly + lcd_get_context()->scroll_y) % 8) * 2;

  if (!(ppu_get_context()->line_ticks & 1)) {
    pipeline_fetch();
  }

  pipeline_push_pixel();
}

void pipeline_fifo_reset() {
  while (ppu_get_context()->pfc.pixel_fifo.size) {
    pixel_fifo_pop();
  }

  ppu_get_context()->pfc.pixel_fifo.head = 0;
}