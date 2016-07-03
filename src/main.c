#include <pebble.h>

#include "main.h"
#include "setpoint.h"
#include "commdata.h"
#include "alarm.h"
#include "timer.h"
#include "storage.h"

/* TODOs:
 * - check on floating point conversions (decimal is sometimes off of web page)
 * - icons: check mark for setpoints
 *   - upper/lower set point active on main screen, flash when alarming
 * - send battery charge percent as parameter on http get
 */

static Window *window;
static TextLayer *temp1_layer;
static TextLayer *temp2_layer;
static TextLayer *text_time_layer;
static char temp1_str[7];
static char temp2_str[7];
static char time_str[7];
Layer *line_layer;
static int master_temp1 = 0;
static int master_temp2 = 0;
GBitmap *upper_setpoint_image, *lower_setpoint_image;
BitmapLayer *setpoint_image_layers[4];

int get_temp(int t) {
	if (t == 1) return master_temp1;
	if (t == 2) return master_temp2;
	return 0;
}

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

void ditoa(char* str, size_t n, int di) {
	int min = di % 10;
	int maj = di / 10;
	snprintf(str, n, "%d.%d", maj, min);
}

void update_temp1(int t) {
	master_temp1 = t;
	ditoa(temp1_str, sizeof(temp1_str), t);
	text_layer_set_text(temp1_layer, temp1_str);
	alarm_check();
}

void update_temp2(int t) {
	master_temp2 = t;
	ditoa(temp2_str, sizeof(temp2_str), t);
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

void update_setpoint_icon(int t, bool is_upper, bool on) {
	int i = (t-1) * 2 + (1 - is_upper);
	layer_set_hidden(bitmap_layer_get_layer(setpoint_image_layers[i]), !on);
}

static AppTimer* flash_timer = NULL;
static int temp_flashing = 0;

// true if currently blank
static bool temp_flash_state = false;

void flash_temp(void* nothing) {
	// flash setpoint icons
	for (int i=0; i<4; i++) {
		int j = 1 << i;
		if (temp_flashing & j) {
			layer_set_hidden(bitmap_layer_get_layer(setpoint_image_layers[i]), !temp_flash_state);
		}
	}
	// flash numbers
	if (temp_flashing & 0x03) {
		if (temp_flash_state) {
			text_layer_set_text_color(temp1_layer, GColorWhite);
		} else {
			text_layer_set_text_color(temp1_layer, GColorBlack);
		}
	}
	if (temp_flashing & 0x0c) {
		if (temp_flash_state) {
			text_layer_set_text_color(temp2_layer, GColorWhite);
		} else {
			text_layer_set_text_color(temp2_layer, GColorBlack);
		}
	}
	temp_flash_state = !temp_flash_state;
	if (temp_flashing) {
		flash_timer = app_timer_register(500, flash_temp, NULL);
	}
}

void set_flash_temp(int t, bool alarm_upper, bool alarm_lower) {
	int old_flashing = temp_flashing;
	int flashing = alarm_lower << 1 | alarm_upper;
	if (t == 1) {
		temp_flashing = (temp_flashing & 0x0c) | flashing;
	} else {
		temp_flashing = (temp_flashing & 0x03) | (flashing << 2);
	}
	if (temp_flashing) {
		if (flash_timer == NULL) flash_temp(NULL);
	} else {
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "canceling flash");
		if (flash_timer != NULL) app_timer_cancel(flash_timer);
		flash_timer = NULL;
		text_layer_set_text_color(temp1_layer, GColorWhite);
		text_layer_set_text_color(temp2_layer, GColorWhite);
		// reset setpoint icons
		for (int i=0; i<4; i++) {
			int j = 1 << i;
			if (old_flashing & j) {
				layer_set_hidden(bitmap_layer_get_layer(setpoint_image_layers[i]), false);
			}
		}		
	}
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
	timer_toggle_clock();
}

static void up_long_handler(ClickRecognizerRef recognizer, void *context) {
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "get setpoint %d %d", alarm_1_upper_setpoint, alarm_1_lower_setpoint);
	get_setpoint(get_current_setpoint(1, true), get_current_setpoint(1, false), master_temp1, "Temp 1", (SetpointCallback) alarm_1_update);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
	alarm_silence();
}

static void select_long_handler(ClickRecognizerRef recognizer, void *context) {
	get_setpoint(get_current_setpoint(2, true), get_current_setpoint(2, false), master_temp2, "Temp 2", (SetpointCallback) alarm_2_update);
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
	
	// add setpoint icons
	upper_setpoint_image = gbitmap_create_with_resource(RESOURCE_ID_SETPOINT_UPPER);
	lower_setpoint_image = gbitmap_create_with_resource(RESOURCE_ID_SETPOINT_LOWER);
	
	for (int i=0; i<4; i++) {
		setpoint_image_layers[i] = bitmap_layer_create(GRect(8, 12 + (i*18) + ((i&2)*4), 16, 16));
		bitmap_layer_set_alignment(setpoint_image_layers[i], GAlignCenter);
		bitmap_layer_set_bitmap(setpoint_image_layers[i], (i & 1 ? lower_setpoint_image : upper_setpoint_image));
		layer_set_hidden(bitmap_layer_get_layer(setpoint_image_layers[i]), true);
		layer_add_child(window_layer, bitmap_layer_get_layer(setpoint_image_layers[i]));
	}
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
	storage_load();

  	app_event_loop();
	
	storage_save();
	app_message_deinit();
  	deinit();
}