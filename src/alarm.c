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
	update_setpoint_icon(1, true, upper != SETPOINT_OFF);
	update_setpoint_icon(1, false, lower != SETPOINT_OFF);
	alarm_check();
}

void alarm_2_update(int upper, int lower) {
	alarm_2_upper_setpoint = upper;
	alarm_2_lower_setpoint = lower;
	update_setpoint_icon(2, true, upper != SETPOINT_OFF);
	update_setpoint_icon(2, false, lower != SETPOINT_OFF);
	alarm_check();
}

void alarm_buzz(void* nothing) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "alarm buzz");
	vibes_double_pulse();
	alarm_timer = app_timer_register(2000, alarm_buzz, NULL);
}

void alarm_silence() {
	if (alarm_timer != NULL) {
		app_timer_cancel(alarm_timer);
		vibes_cancel();
		last_alarm_silence = time(NULL);
	}
	alarm_timer = NULL;
}

#define pass_upper(x) ((x) == SETPOINT_OFF ? -(x) : (x))

// Check whether we should be sounding the alarm
void alarm_check() {
	int master_temp1 = get_temp(1);
	int master_temp2 = get_temp(2);
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "check 1 %d < %d < %d", alarm_1_lower_setpoint, master_temp1, alarm_1_upper_setpoint);
	
	bool alarm_temp1_upper = master_temp1 > pass_upper(alarm_1_upper_setpoint);
	bool alarm_temp1_lower = master_temp1 < alarm_1_lower_setpoint;
	bool alarm_temp2_upper = master_temp2 > pass_upper(alarm_2_upper_setpoint);
	bool alarm_temp2_lower = master_temp2 < alarm_2_lower_setpoint;

	set_flash_temp(1, alarm_temp1_upper, alarm_temp1_lower);
	set_flash_temp(2, alarm_temp2_upper, alarm_temp2_lower);
	
	bool alarming = alarm_temp1_upper || alarm_temp1_lower || alarm_temp2_upper || alarm_temp2_lower;
	if (alarming) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "alarming %p %d %d", alarm_timer, (int) time(NULL), (int) last_alarm_silence);		
		if (!alarm_timer && time(NULL) > (last_alarm_silence + 60)) 
			alarm_buzz(NULL);
	} else {
		alarm_silence();
	}
}