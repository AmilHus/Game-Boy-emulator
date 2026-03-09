#pragma once

#include <common.h>

typedef struct {
  u8 nr10, nr11, nr12, nr13, nr14;
  u8 nr21, nr22, nr23, nr24;
  u8 nr30, nr31, nr32, nr33, nr34;
  u8 nr41, nr42, nr43, nr44;
  u8 nr50, nr51, nr52;

  u8 wave_ram[16];

  u32 frame_sequencer;
  u8 master_on;

  struct {
    u8 enabled;
    u16 frequency;
    u16 timer;
    u8 volume;
    u8 duty;
    u8 duty_step;
    u16 length_counter;
    u8 length_enabled;
    // Sweep
    u16 sweep_frequency;
    u8 sweep_timer;
    u8 sweep_enabled;
  } ch1;

  struct {
    u8 enabled;
    u16 frequency;
    u16 timer;
    u8 volume;
    u8 duty;
    u8 duty_step;
    u16 length_counter;
    u8 length_enabled;
  } ch2;
} apu_context;

void apu_init();
void apu_tick();

u8 apu_read(u16 address);
void apu_write(u16 address, u8 value);
