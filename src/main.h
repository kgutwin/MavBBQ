#pragma once
#include <pebble.h>

void ditoa(char* str, size_t n, int di);

void update_temp1(int t);
void update_temp2(int t);
void update_time(struct tm *time);
void set_flash_temp(int t, bool flashing);
int get_temp(int t);