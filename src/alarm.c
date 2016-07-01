#include <pebble.h>
#include "alarm.h"
#include "main.h"
#include "setpoint.h"

static int alarm_1_upper_setpoint = SETPOINT_OFF;
static int alarm_2_upper_setpoint = SETPOINT_OFF;
static int alarm_1_lower_setpoint = SETPOINT_OFF;
static int alarm_2_lower_setpoint = SETPOINT_OFF;

static AppTimer* alarm_timer = NULL;
time_t last_alarm_silence = 0;

int get_current_setpoint(int t, bool is_upper) {
	if (t == 1) {
		if (is_upper) return alarm_1_upper_setpoint;
		else return alarm_1_lower_setpoint;
	} else if (t == 2) {
		if (is_upper) return alarm_2_upper_setpoint;
		else return alarm_2_lower_setpoint;		
	}
	return 0;
}

void alarm_1_update(int upper, int lower) {
	alarm_1_upper_setpoint = upper;
	alarm_1_lower_setpoint = lower;
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "set setpoint %d %d", alarm_1_upper_setpoint, alarm_1_lower_setpoint);
	alarm_check();
}

void alarm_2_update(int upper, int lower) {
	alarm_2_upper_setpoint = upper;
	alarm_2_lower_setpoint = lower;
	alarm_check();
}

void alarm_buzz(void* nothing) {
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "alarm buzz");
	vibes_double_pulse();
	alarm_timer = app_timer_register(2000, alarm_buzz, NULL);
}

void alarm_silence() {
	if (alarm_timer != NULL) {
		app_timer_cancel(alarm_timer);
		vibes_cancel();
	}
	alarm_timer = NULL;
	last_alarm_silence = time(NULL);
}

#define pass_upper(x) ((x) == SETPOINT_OFF ? -(x) : (x))

// Check whether we should be sounding the alarm
void alarm_check() {
	int master_temp1 = get_temp(1);
	int master_temp2 = get_temp(2);
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "check 1 %d < %d < %d", alarm_1_lower_setpoint, master_temp1, alarm_1_upper_setpoint);
	if (master_temp1 > pass_upper(alarm_1_upper_setpoint) || master_temp1 < alarm_1_lower_setpoint) {
		set_flash_temp(1, true);
		if (!alarm_timer && time(NULL) > (last_alarm_silence + 60)) alarm_buzz(NULL);
	} else {
		set_flash_temp(1, false);
	}
	if (master_temp2 > pass_upper(alarm_2_upper_setpoint) || master_temp2 < alarm_2_lower_setpoint) {
		set_flash_temp(2, true);
		if (!alarm_timer && time(NULL) > (last_alarm_silence + 60)) alarm_buzz(NULL);
	} else {
		set_flash_temp(2, false);
	}
	if (master_temp1 <= alarm_1_upper_setpoint && master_temp1 >= alarm_1_lower_setpoint &&
	   master_temp2 <= alarm_2_upper_setpoint && master_temp2 >= alarm_2_lower_setpoint) {
		alarm_silence();
	}
}