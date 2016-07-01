#pragma once
#include "setpoint.h"

void alarm_1_update(int upper, int lower);
void alarm_2_update(int upper, int lower);
void alarm_silence();
void alarm_check();
int get_current_setpoint(int t, bool is_upper);