#pragma once
#include <pebble.h>

static float master_temp1 = 0.0;
static float master_temp2 = 0.0;

void update_temp1(float t);
void update_temp2(float t);
void update_time(struct tm *time);