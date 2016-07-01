#include <pebble.h>
#include "timer.h"
#include "main.h"

uint8_t clock_mode = 0;
time_t timer_start_time = 0;
time_t timer_pause_time = 0;
static int last_second = 59;

time_t get_timer_start_time() {
	return timer_start_time;
}

time_t get_timer_pause_time() {
	return timer_pause_time;
}

void set_timer_start_time(time_t t) {
	timer_start_time = t;
}

void set_timer_pause_time(time_t t) {
	timer_pause_time = t;
}

void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
	/*
	int current_sec = tick_time->tm_sec;
	if (! clock_mode) {
		current_sec -= timer_start_time % 60;
		current_sec %= 60;
	}
	if (current_sec > last_second) {
		last_second = current_sec;
		return;
	} else {
		last_second = current_sec;
	}
	*/
	if (clock_mode) {
		update_time(tick_time);
	} else {
		time_t elapsed = time(NULL) - timer_start_time;
		if (timer_pause_time > 0) elapsed = timer_pause_time - timer_start_time;
		
		struct tm timer_time;
		timer_time.tm_sec = elapsed % 60;
		timer_time.tm_min = (elapsed / 60) % 60;
		timer_time.tm_hour = elapsed / 3600;
		timer_time.tm_year = 0;

		update_time(&timer_time);
	}
}

void timer_toggle_clock() {
	clock_mode = (clock_mode == 0) ? 1 : 0;
	APP_LOG(APP_LOG_LEVEL_DEBUG, "clock mode: %d", clock_mode);
	time_t now = time(NULL);
	last_second = 59;
	handle_second_tick(localtime(&now), SECOND_UNIT);
}

void timer_reset() {
	timer_start_time = time(NULL);
	timer_pause_time = 0;
}

void timer_stop_start() {
	if (timer_pause_time > 0) {
		// resuming; advance timer_start_time as if we had never paused
		timer_start_time += time(NULL) - timer_pause_time;
		timer_pause_time = 0;
	} else {
		// pausing
		timer_pause_time = time(NULL);
	}
}

void timer_init() {
	timer_start_time = time(NULL);
	tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
}