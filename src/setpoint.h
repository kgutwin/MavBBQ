#pragma once
#define SETPOINT_OFF -9999
#define SETPOINT_NOTYET -10000
#define SETPOINT_MIN -400
#define SETPOINT_MAX 4000

typedef void(* SetpointCallback)(int upper_setpoint, int lower_setpoint);
void get_setpoint(int current_upper_setp, int current_lower_setp, int current_t, char* header, SetpointCallback cb);