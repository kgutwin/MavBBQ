#pragma once

void timer_init();
void timer_stop_start();
void timer_reset();
void timer_toggle_clock();
time_t get_timer_start_time();
time_t get_timer_pause_time();
void set_timer_start_time(time_t t);
void set_timer_pause_time(time_t t);