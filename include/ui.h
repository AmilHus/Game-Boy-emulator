#pragma once

#include <common.h>

static const int SCREEN_WIDTH = 650;
static const int SCREEN_HEIGHT = 600;

void ui_init();
void ui_update();
void ui_handle_events();
void delay(u32 ms);
