/**
 * Weather Face
 * A simple watch face for monitoring weather in your area
 *
 * Copyright 2013 Tory Gaurnier
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <pebble.h>

static const uint16_t INTERVAL		=	15; // Minutes between weather checks
static const uint32_t INBOUND_SIZE	=	64; // Inbound app message size
static const uint32_t OUTBOUND_SIZE	=	64; // Outbound app message size
static const uint16_t SCREEN_WIDTH	=	144;
static const uint16_t SCREEN_HEIGHT	=	164;

static Window *window;
static TextLayer *city_layer;
static TextLayer *temp_layer;
static TextLayer *desc_layer;

static char city_buff[50];
static char temp_buff[50];
static char desc_buff[50];

enum dict_keys {
	STATUS,
	CITY,
	TEMP,
	DESC
};


/**
 * Outgoing message sent
 */
static void sent_handler(DictionaryIterator *sent, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Message sent\n");
}


/**
 * Outgoing message failed to send
 */
static void failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Failed to send message: %s\n", (char*)reason);
}


/**
 * Send signal to Javascript to check the weather
 */
static void check_weather() {
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);

	Tuplet value = TupletCString(STATUS, "retrieve");
	dict_write_tuplet(iter, &value);

	app_message_outbox_send();
}


/**
 * Check weather every INTERVAL minutes
 */
static void tick_handle(struct tm *tick_time, TimeUnits units_changed) {
	static unsigned short count = 1;

	if(count == INTERVAL) {
		check_weather();

		count = 1;
	}

	else count++;
}


/**
 * Message received
 */
static void received_handler(DictionaryIterator *message, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Recieved message\n");
	char *status = (char*)dict_find(message, STATUS)->value;

	if(strcmp(status, "ready") == 0) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Recieved status \"ready\"\n");
		check_weather();
		tick_timer_service_subscribe(MINUTE_UNIT, &tick_handle);
	}

	else if(strcmp(status, "reporting") == 0) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Recieved status \"reporting\"\n");

		strcpy(city_buff, (char*)dict_find(message, CITY)->value);
		strcpy(temp_buff, (char*)dict_find(message, TEMP)->value);
		strcpy(desc_buff, (char*)dict_find(message, DESC)->value);

		text_layer_set_text(city_layer, city_buff);
		text_layer_set_text(temp_layer, temp_buff);
		text_layer_set_text(desc_layer, desc_buff);
	}

	else if(strcmp(status, "configUpdated") == 0)
		check_weather();
}


/**
 * Failed to receive message
 */
static void dropped_handler(AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Message dropped: %s\n", (char*)reason);
}


static void init(void) {
	GFont font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_MY_FONT_16));

	// Init window
	window = window_create();
	window_stack_push(window, true);
	window_set_background_color(window, GColorBlack);
	Layer *root_layer = window_get_root_layer(window);
	GRect frame = layer_get_frame(root_layer);

	// Init city_layer
	city_layer = text_layer_create(frame);
	layer_set_frame((Layer*)city_layer, GRect(0, 0, SCREEN_WIDTH, 20));
	text_layer_set_text_alignment(city_layer, GTextAlignmentCenter);
	text_layer_set_font(city_layer, font);
	layer_add_child(root_layer, text_layer_get_layer(city_layer));

	// Init temp_layer
	temp_layer = text_layer_create(frame);
	layer_set_frame((Layer*)temp_layer, GRect(0, SCREEN_HEIGHT - 60, SCREEN_WIDTH, 20));
	text_layer_set_text_alignment(temp_layer, GTextAlignmentCenter);
	text_layer_set_font(temp_layer, font);
	layer_add_child(root_layer, text_layer_get_layer(temp_layer));

	// Init desc_layer
	desc_layer = text_layer_create(frame);
	layer_set_frame((Layer*)desc_layer, GRect(0, SCREEN_HEIGHT - 30, SCREEN_WIDTH, 20));
	text_layer_set_text_alignment(desc_layer, GTextAlignmentCenter);
	text_layer_set_font(temp_layer, font);
	layer_add_child(root_layer, text_layer_get_layer(desc_layer));

	// Init app message and message handlers
	app_message_register_outbox_sent(sent_handler);
	app_message_register_outbox_failed(failed_handler);
	app_message_register_inbox_received(received_handler);
	app_message_register_inbox_dropped(dropped_handler);
	app_message_open(INBOUND_SIZE, OUTBOUND_SIZE);

	text_layer_set_text(city_layer, "loading...");
}


static void deinit(void) {
	text_layer_destroy(city_layer);
	window_destroy(window);
}


int main(void) {
	init();
	app_event_loop();
	deinit();
}