#include <pebble.h>
#include "setpoint.h"

static int current_setpoint;
static float current_temp;
static char *setpoint_header;
static char setpoint_current_temp[21];
static ActionBarLayer *action_bar;
static TextLayer *header_layer, *setpoint_val_layer, *setpoint_current_t_layer;
static GBitmap *s_icon_plus, *s_icon_minus;
static SetpointCallback setpoint_callback;

static void setpoint_update_text() {
	static char setpoint_val_text[5];
	if (current_setpoint <= SETPOINT_MIN || current_setpoint >= SETPOINT_MAX) {
		snprintf(setpoint_val_text, sizeof(setpoint_val_text), "OFF");
	} else {
		snprintf(setpoint_val_text, sizeof(setpoint_val_text), "%d", current_setpoint);
	}
	text_layer_set_text(setpoint_val_layer, setpoint_val_text);
}

static void setpoint_down_handler(ClickRecognizerRef recognizer, void *context) {
	if (current_setpoint == SETPOINT_OFF) {
		current_setpoint = current_temp;
	}
	current_setpoint --;
	if (current_setpoint < SETPOINT_MIN) current_setpoint = SETPOINT_MIN;
	setpoint_update_text();
}

static void setpoint_up_handler(ClickRecognizerRef recognizer, void *context) {
	if (current_setpoint == SETPOINT_OFF) {
		current_setpoint = current_temp;
	} 
	current_setpoint ++;
	if (current_setpoint > SETPOINT_MAX) current_setpoint = SETPOINT_MAX;
	setpoint_update_text();
}

static void setpoint_select_handler(ClickRecognizerRef recognizer, void *context) {
	setpoint_callback(current_setpoint);
	window_stack_pop(true);
}

static void setpoint_select_long_handler(ClickRecognizerRef recognizer, void *context) {
	current_setpoint = SETPOINT_OFF;
	setpoint_update_text();
}

void setpoint_click_config_provider(void *context) {
	window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100,
											(ClickHandler) setpoint_down_handler);
	window_single_repeating_click_subscribe(BUTTON_ID_UP, 100,
											(ClickHandler) setpoint_up_handler);
  	window_single_click_subscribe(BUTTON_ID_SELECT, setpoint_select_handler);
	window_long_click_subscribe(BUTTON_ID_SELECT, 0, setpoint_select_long_handler, NULL);
}

static void setpoint_window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_frame(window_layer);
	
	action_bar = action_bar_layer_create();
	action_bar_layer_add_to_window(action_bar, window);
	action_bar_layer_set_click_config_provider(action_bar, setpoint_click_config_provider);

	s_icon_plus = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_PLUS);
  	s_icon_minus = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_MINUS);

  	action_bar_layer_set_icon(action_bar, BUTTON_ID_UP, s_icon_plus);
  	action_bar_layer_set_icon(action_bar, BUTTON_ID_DOWN, s_icon_minus);
	
	int width = bounds.size.w - ACTION_BAR_WIDTH - 3;
	
	header_layer = text_layer_create(GRect(4, 0, width, 60));
	text_layer_set_font(header_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
	text_layer_set_background_color(header_layer, GColorClear);
  	text_layer_set_text(header_layer, setpoint_header);
  	layer_add_child(window_layer, text_layer_get_layer(header_layer));

	setpoint_val_layer = text_layer_create(GRect(15, 44, width-15, 60));
  	text_layer_set_font(setpoint_val_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  	text_layer_set_background_color(setpoint_val_layer, GColorClear);
  	layer_add_child(window_layer, text_layer_get_layer(setpoint_val_layer));
	
	setpoint_current_t_layer = text_layer_create(GRect(8, 106, width, 60));
	text_layer_set_font(setpoint_current_t_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
	text_layer_set_background_color(setpoint_current_t_layer, GColorClear);
  	text_layer_set_text(setpoint_current_t_layer, setpoint_current_temp);
  	layer_add_child(window_layer, text_layer_get_layer(setpoint_current_t_layer));

	setpoint_update_text();
}

static void setpoint_window_unload(Window *window) {
	text_layer_destroy(header_layer);
	text_layer_destroy(setpoint_val_layer);
	text_layer_destroy(setpoint_current_t_layer);
	action_bar_layer_destroy(action_bar);
	gbitmap_destroy(s_icon_plus);
  	gbitmap_destroy(s_icon_minus);
}

void get_setpoint(int current_setp, float current_t, char* setpoint_head, SetpointCallback cb) {
	current_setpoint = current_setp;
	current_temp = current_t;
	setpoint_header = setpoint_head;
	setpoint_callback = cb;
	snprintf(setpoint_current_temp, sizeof(setpoint_current_temp),
			"Current: %d", (int) current_t);
	
	Window *setpoint_window = window_create();
	
	window_set_window_handlers(setpoint_window, (WindowHandlers) {
		.load = setpoint_window_load,
		.unload = setpoint_window_unload,
	});
	
	window_stack_push(setpoint_window, true);
}