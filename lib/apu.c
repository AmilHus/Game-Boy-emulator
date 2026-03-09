#include <SDL2/SDL.h>
#include <apu.h>
#include <common.h>

static apu_context ctx;
static i16 audio_buffer[4096];
static u32 audio_ptr = 0;

static u8 duty_table[4][8] = {{0, 0, 0, 0, 0, 0, 0, 1},
                              {1, 0, 0, 0, 0, 0, 0, 1},
                              {1, 0, 0, 0, 0, 1, 1, 1},
                              {0, 1, 1, 1, 1, 1, 1, 0}};

void apu_init() {
  SDL_AudioSpec wanted;
  wanted.freq = 44100;
  wanted.format = AUDIO_S16SYS;
  wanted.channels = 2;
  wanted.samples = 1024;
  wanted.callback = NULL;

  if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
    printf("SDL_INIT_AUDIO failed: %s\n", SDL_GetError());
  }

  if (SDL_OpenAudio(&wanted, NULL) < 0) {
    printf("SDL_OpenAudio failed: %s\n", SDL_GetError());
  }

  SDL_PauseAudio(0);
}

static void ch1_trigger() {
  ctx.ch1.enabled = 1;
  ctx.ch1.timer = (2048 - ((ctx.nr14 & 0x7) << 8 | ctx.nr13)) * 4;
  ctx.ch1.volume = (ctx.nr12 & 0xF0) >> 4;
  ctx.ch1.duty = (ctx.nr11 & 0xC0) >> 6;
  if (ctx.ch1.length_counter == 0)
    ctx.ch1.length_counter = 64;
}

static void ch2_trigger() {
  ctx.ch2.enabled = 1;
  ctx.ch2.timer = (2048 - ((ctx.nr24 & 0x7) << 8 | ctx.nr23)) * 4;
  ctx.ch2.volume = (ctx.nr22 & 0xF0) >> 4;
  ctx.ch2.duty = (ctx.nr21 & 0xC0) >> 6;
  if (ctx.ch2.length_counter == 0)
    ctx.ch2.length_counter = 64;
}

void apu_tick() {
  ctx.frame_sequencer++;

  if (ctx.frame_sequencer % 8192 == 0) { // 512Hz
    u8 step = (ctx.frame_sequencer / 8192) % 8;

    if (step % 2 == 0) { // Length counter (256Hz)
      if (ctx.ch1.length_enabled && ctx.ch1.length_counter > 0) {
        if (--ctx.ch1.length_counter == 0)
          ctx.ch1.enabled = 0;
      }
      if (ctx.ch2.length_enabled && ctx.ch2.length_counter > 0) {
        if (--ctx.ch2.length_counter == 0)
          ctx.ch2.enabled = 0;
      }
    }
  }

  if (ctx.ch1.enabled) {
    if (ctx.ch1.timer > 0) {
      ctx.ch1.timer--;
    } else {
      ctx.ch1.timer = (2048 - ((ctx.nr14 & 0x7) << 8 | ctx.nr13)) * 4;
      ctx.ch1.duty_step = (ctx.ch1.duty_step + 1) % 8;
    }
  }

  if (ctx.ch2.enabled) {
    if (ctx.ch2.timer > 0) {
      ctx.ch2.timer--;
    } else {
      ctx.ch2.timer = (2048 - ((ctx.nr24 & 0x7) << 8 | ctx.nr23)) * 4;
      ctx.ch2.duty_step = (ctx.ch2.duty_step + 1) % 8;
    }
  }

  // Sample every 95 cycles (~44100Hz)
  static u32 sample_timer = 0;
  if (++sample_timer >= 95) {
    sample_timer = 0;

    i16 sample = 0;
    if (ctx.ch1.enabled && duty_table[ctx.ch1.duty][ctx.ch1.duty_step]) {
      sample += ctx.ch1.volume * 100;
    }
    if (ctx.ch2.enabled && duty_table[ctx.ch2.duty][ctx.ch2.duty_step]) {
      sample += ctx.ch2.volume * 100;
    }

    audio_buffer[audio_ptr++] = sample; // Left
    audio_buffer[audio_ptr++] = sample; // Right

    if (audio_ptr >= 4096) {
      SDL_QueueAudio(1, audio_buffer, sizeof(audio_buffer));
      audio_ptr = 0;
    }
  }
}

u8 apu_read(u16 address) {
  if (address >= 0xFF30 && address <= 0xFF3F) {
    return ctx.wave_ram[address - 0xFF30];
  }

  switch (address) {
  case 0xFF10:
    return ctx.nr10 | 0x80;
  case 0xFF11:
    return ctx.nr11 | 0x3F;
  case 0xFF12:
    return ctx.nr12;
  case 0xFF13:
    return 0xFF; // Write only
  case 0xFF14:
    return ctx.nr14 | 0xBF;
  case 0xFF16:
    return ctx.nr21 | 0x3F;
  case 0xFF17:
    return ctx.nr22;
  case 0xFF18:
    return 0xFF; // Write only
  case 0xFF19:
    return ctx.nr24 | 0xBF;
  case 0xFF24:
    return ctx.nr50;
  case 0xFF25:
    return ctx.nr51;
  case 0xFF26:
    return ctx.nr52 | 0x70;
  }

  return 0;
}

void apu_write(u16 address, u8 value) {
  if (address >= 0xFF30 && address <= 0xFF3F) {
    ctx.wave_ram[address - 0xFF30] = value;
    return;
  }

  switch (address) {
  case 0xFF10:
    ctx.nr10 = value;
    break;
  case 0xFF11:
    ctx.nr11 = value;
    ctx.ch1.length_counter = 64 - (value & 0x3F);
    break;
  case 0xFF12:
    ctx.nr12 = value;
    break;
  case 0xFF13:
    ctx.nr13 = value;
    break;
  case 0xFF14:
    ctx.nr14 = value;
    ctx.ch1.length_enabled = (value >> 6) & 1;
    if (value & 0x80)
      ch1_trigger();
    break;
  case 0xFF16:
    ctx.nr21 = value;
    ctx.ch2.length_counter = 64 - (value & 0x3F);
    break;
  case 0xFF17:
    ctx.nr22 = value;
    break;
  case 0xFF18:
    ctx.nr23 = value;
    break;
  case 0xFF19:
    ctx.nr24 = value;
    ctx.ch2.length_enabled = (value >> 6) & 1;
    if (value & 0x80)
      ch2_trigger();
    break;
  case 0xFF24:
    ctx.nr50 = value;
    break;
  case 0xFF25:
    ctx.nr51 = value;
    break;
  case 0xFF26:
    if (!(value & 0x80)) {
      memset(&ctx, 0, sizeof(ctx));
    }
    ctx.nr52 = value & 0x80;
    break;
  }
}
