#include <pebble.h>
#include "storage.h"
#include "timer.h"
#include "alarm.h"

#define STORAGE_KEY_TIMER_START_TIME 1
#define STORAGE_KEY_TIMER_PAUSE_TIME 2
#define STORAGE_KEY_ALARM_1_UPPER_SETPOINT 3
#define STORAGE_KEY_ALARM_1_LOWER_SETPOINT 4
#define STORAGE_KEY_ALARM_2_UPPER_SETPOINT 5
#define STORAGE_KEY_ALARM_2_LOWER_SETPOINT 6

void storage_save() {
	persist_write_int(STORAGE_KEY_TIMER_START_TIME, get_timer_start_time());
	persist_write_int(STORAGE_KEY_TIMER_PAUSE_TIME, get_timer_pause_time());
	persist_write_int(STORAGE_KEY_ALARM_1_UPPER_SETPOINT, get_current_setpoint(1, true));
	persist_write_int(STORAGE_KEY_ALARM_1_LOWER_SETPOINT, get_current_setpoint(1, false));
	persist_write_int(STORAGE_KEY_ALARM_2_UPPER_SETPOINT, get_current_setpoint(2, true));
	persist_write_int(STORAGE_KEY_ALARM_2_LOWER_SETPOINT, get_current_setpoint(2, false));
}

void storage_load() {
	if ((time(NULL) - persist_read_int(STORAGE_KEY_TIMER_START_TIME)) < 24 * 60 * 60) {
		set_timer_start_time(persist_read_int(STORAGE_KEY_TIMER_START_TIME));
		set_timer_pause_time(persist_read_int(STORAGE_KEY_TIMER_PAUSE_TIME));
		if (persist_exists(STORAGE_KEY_ALARM_1_UPPER_SETPOINT))
			alarm_1_update(
				persist_read_int(STORAGE_KEY_ALARM_1_UPPER_SETPOINT),
				persist_read_int(STORAGE_KEY_ALARM_1_LOWER_SETPOINT));
		if (persist_exists(STORAGE_KEY_ALARM_2_UPPER_SETPOINT))
			alarm_2_update(
				persist_read_int(STORAGE_KEY_ALARM_2_UPPER_SETPOINT),
				persist_read_int(STORAGE_KEY_ALARM_2_LOWER_SETPOINT));
	}
}