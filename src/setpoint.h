#pragma once
#define SETPOINT_OFF -9999
#define SETPOINT_MIN -40
#define SETPOINT_MAX 400

typedef void(* SetpointCallback)(float returned_setpoint);
void get_setpoint(int current_setp, float current_t, char* header, SetpointCallback cb);