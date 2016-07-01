#include <pebble.h>
#include "commdata.h"
#include "main.h"

enum messages_e {
	MSG_TEMP1 = 10,
	MSG_TEMP2 = 20,
	MSG_TEMPGET = 1,
};

static AppSync sync;
#define SYNC_BUFFER_SIZE 64
static uint8_t sync_buffer[SYNC_BUFFER_SIZE];
static AppTimer* update_timer;
	
// Compare two arbitrary tuples, returning zero if equal, and either positive
// or negative 1 depending on if the first is "greater" than the second.
#define THREE_COMP(x,y) if((x)>(y))return 1;if((x)<(y))return -1;
int8_t tuple_compare(const Tuple* a, const Tuple* b) {
	THREE_COMP(a->key, b->key)
	THREE_COMP(a->length, b->length)
	THREE_COMP(a->type, b->type)
	switch (a->type) {
		case TUPLE_CSTRING:
			return strcmp(a->value->cstring, b->value->cstring);
		case TUPLE_BYTE_ARRAY:
			return memcmp(a->value->data, b->value->data, a->length);
		case TUPLE_INT:
			if (a->length == 4) {
				THREE_COMP(a->value->int32, b->value->int32)
			} else if (a->length == 2) {
				THREE_COMP(a->value->int16, b->value->int16)
			} else if (a->length == 1) {
				THREE_COMP(a->value->int8, b->value->int8)
			}
			break;
		case TUPLE_UINT:
			if (a->length == 4) {
				THREE_COMP(a->value->uint32, b->value->uint32)
			} else if (a->length == 2) {
				THREE_COMP(a->value->uint16, b->value->uint16)
			} else if (a->length == 1) {
				THREE_COMP(a->value->uint8, b->value->uint8)
			}
			break;
	}
	return 0;
}

// Send a log message containing the content of the tuple.
void tuple_log(const Tuple* t) {
	switch (t->type) {
		case TUPLE_CSTRING:
			APP_LOG(APP_LOG_LEVEL_DEBUG, "key:%lu cstring:'%s'", t->key, t->value->cstring);
			break;
		case TUPLE_BYTE_ARRAY:
			APP_LOG(APP_LOG_LEVEL_DEBUG, "key:%lu bytearray len:%d", t->key, t->length);
			break;
		case TUPLE_INT:
			if (t->length == 4) { APP_LOG(APP_LOG_LEVEL_DEBUG, "key:%lu int32:%ld", t->key, t->value->int32); }
			if (t->length == 2) { APP_LOG(APP_LOG_LEVEL_DEBUG, "key:%lu int16:%d", t->key, t->value->int16); }
			if (t->length == 1) { APP_LOG(APP_LOG_LEVEL_DEBUG, "key:%lu int8:%d", t->key, t->value->int8); }
			break;
		case TUPLE_UINT:
			if (t->length == 4) { APP_LOG(APP_LOG_LEVEL_DEBUG, "key:%lu int32:%lu", t->key, t->value->uint32); }
			if (t->length == 2) { APP_LOG(APP_LOG_LEVEL_DEBUG, "key:%lu int16:%u", t->key, t->value->uint16); }
			if (t->length == 1) { APP_LOG(APP_LOG_LEVEL_DEBUG, "key:%lu int8:%u", t->key, t->value->uint8); }
			break;
	}
}

// This is run both on initial AppSync startup as well as any time a message
// is received with new tuple data.
static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
	// Frequently this function is called when the tuple value hasn't really
	// changed. We skip this situation (if this isn't initialization time)
	// so that we properly transition in and out of calendar mode.
	if (old_tuple != NULL && tuple_compare(new_tuple, old_tuple) == 0)
		return;
	
	tuple_log(new_tuple);
	
	switch (key) {
		case MSG_TEMP1:
			update_temp1(new_tuple->value->int16);
			break;
		case MSG_TEMP2:
			update_temp2(new_tuple->value->int16);
			break;
	}
}

// Called in case of problems with AppMessage.
static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

// Send a message to the javascript code requesting an update.
void request_update(void* nothing) {
	Tuplet get_tuple = TupletInteger(MSG_TEMPGET, 1);
        
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);

	if (iter == NULL) return;

	dict_write_tuplet(iter, &get_tuple);
	dict_write_end(iter);

	app_message_outbox_send();
	
	update_timer = app_timer_register(2000, request_update, NULL);
}

// Set up app sync, defining initial values for data parameters
void app_message_init() {
	Tuplet initial_values[] = {
		TupletInteger(MSG_TEMP1, 0),
		TupletInteger(MSG_TEMP2, 0),
	};
	
	app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values), 
		sync_tuple_changed_callback, sync_error_callback, NULL);
	// init buffers
	app_message_open(sizeof(sync_buffer), sizeof(sync_buffer));
	// send initial request
	update_timer = app_timer_register(1000, request_update, NULL);
}

// Tear down app sync
void app_message_deinit() {
	if (update_timer) app_timer_cancel(update_timer);	
	app_sync_deinit(&sync);
}