#pragma once
#include "setpoint.h"

static int alarm_1_setpoint = SETPOINT_OFF;
static int alarm_2_setpoint = SETPOINT_OFF;

void alarm_1_update(float val);
void alarm_2_update(float val);
void alarm_silence();
void alarm_check();