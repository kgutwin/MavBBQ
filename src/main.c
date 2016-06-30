#include <pebble.h>

#include "main.h"
#include "setpoint.h"
#include "commdata.h"
#include "alarm.h"
#include "timer.h"

/* TODOs:
 * - Make setpoints paired up
 * - Implement alarm code
 * - Make timer use persistent storage
 * - check on floating point conversions (decimal is sometimes off of web page)
 * - icons: check mark for setpoints
 *   - upper/lower set point active on main screen, flash when alarming
 */

static Window *window;
static TextLayer *temp1_layer;
static TextLayer *temp2_layer;
static TextLayer *text_time_layer;
static char temp1_str[7];
static char temp2_str[7];
static char time_str[7];
Layer *line_layer;

void line_layer_update_callback(Layer *layer, GContext* ctx) {
	graphics_context_set_fill_color(ctx, GColorWhite);
	GRect bounds = layer_get_bounds(layer);
	BatteryChargeState bat = battery_state_service_peek();
	// draw half height bar
	bounds.size.h = 1;
	graphics_fill_rect(ctx, bounds, 0, GCornerNone);
	// draw full height bar
	bounds.size.h = 2;
	bounds.size.w = (140 * bat.charge_percent) / 100;
	graphics_fill_rect(ctx, bounds, 0, GCornerNone);
}

void ftoa(char* str, size_t n, float f) {
	int maj = (int) f;
	int min = (int) (f * 10.0 - maj * 10.0);
	snprintf(str, n, "%d.%d", maj, min);
}

void update_temp1(float t) {
	master_temp1 = t;
	ftoa(temp1_str, sizeof(temp1_str), t);
	text_layer_set_text(temp1_layer, temp1_str);
	alarm_check();
}

void update_temp2(float t) {
	master_temp2 = t;
	ftoa(temp2_str, sizeof(temp2_str), t);
	text_layer_set_text(temp2_layer, temp2_str);
	alarm_check();
}

void update_time(struct tm *time) {
	char* time_format;
	if (time->tm_year == 0 || clock_is_24h_style()) {
		time_format = (time->tm_sec % 2) ? "%H:%M" : "%H.%M";
	} else {
		time_format = (time->tm_sec % 2) ? "%I:%M" : "%I.%M";
	}

	strftime(time_str, sizeof(time_str), time_format, time);

	// Kludge to handle lack of non-padded hour format string
	// for twelve hour clock.
	if (!clock_is_24h_style() && (time_str[0] == '0')) {
		memmove(time_str, &time_str[1], sizeof(time_str) - 1);
	}

	text_layer_set_text(text_time_layer, time_str);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
	timer_toggle_clock();
}

static void up_long_handler(ClickRecognizerRef recognizer, void *context) {
	get_setpoint(alarm_1_setpoint, master_temp1, "Temp 1", (SetpointCallback) alarm_1_update);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
	alarm_silence();
}

static void select_long_handler(ClickRecognizerRef recognizer, void *context) {
	get_setpoint(alarm_2_setpoint, master_temp2, "Temp 2", (SetpointCallback) alarm_2_update);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
	timer_stop_start();
}

static void down_long_handler(ClickRecognizerRef recognizer, void *context) {
	timer_reset();
}

static void click_config_provider(void *context) {
  	window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  	window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  	window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
	window_long_click_subscribe(BUTTON_ID_UP, 0, up_long_handler, NULL);
	window_long_click_subscribe(BUTTON_ID_SELECT, 0, select_long_handler, NULL);
	window_long_click_subscribe(BUTTON_ID_DOWN, 0, down_long_handler, NULL);
}

static void window_load(Window *window) {
  	Layer *window_layer = window_get_root_layer(window);
  	GRect bounds = layer_get_bounds(window_layer);

	window_set_background_color(window, GColorBlack);

	temp1_layer = text_layer_create(GRect(7, 2, bounds.size.w-14, 44));
	text_layer_set_text_color(temp1_layer, GColorWhite);
	text_layer_set_background_color(temp1_layer, GColorClear);
	text_layer_set_font(temp1_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
	text_layer_set_text_alignment(temp1_layer, GTextAlignmentRight);
	layer_add_child(window_layer, text_layer_get_layer(temp1_layer));

	temp2_layer = text_layer_create(GRect(7, 46, bounds.size.w-14, 48));
	text_layer_set_text_color(temp2_layer, GColorWhite);
	text_layer_set_background_color(temp2_layer, GColorClear);
	text_layer_set_font(temp2_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
	text_layer_set_text_alignment(temp2_layer, GTextAlignmentRight);
	layer_add_child(window_layer, text_layer_get_layer(temp2_layer));

	GRect line_frame = GRect(8, 97, 139, 2);
	line_layer = layer_create(line_frame);
	layer_set_update_proc(line_layer, line_layer_update_callback);
	layer_add_child(window_layer, line_layer);

	text_time_layer = text_layer_create(GRect(7, 96, bounds.size.w-14, bounds.size.h-96));
	text_layer_set_text_color(text_time_layer, GColorWhite);
	text_layer_set_background_color(text_time_layer, GColorClear);
	text_layer_set_font(text_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
	text_layer_set_text_alignment(text_time_layer, GTextAlignmentRight);
	layer_add_child(window_layer, text_layer_get_layer(text_time_layer));
}

static void window_unload(Window *window) {
  	text_layer_destroy(text_time_layer);
	text_layer_destroy(temp1_layer);
	text_layer_destroy(temp2_layer);
	layer_destroy(line_layer);
}

static void init(void) {
  	window = window_create();
  	window_set_click_config_provider(window, click_config_provider);
  	window_set_window_handlers(window, (WindowHandlers) {
    	.load = window_load,
    	.unload = window_unload,
  	});

	window_stack_push(window, true /* animated */);
}

static void deinit(void) {
  	window_destroy(window);
}

int main(void) {
  	init();
	app_message_init();
	timer_init();

  	app_event_loop();
	
	app_message_deinit();
  	deinit();
}