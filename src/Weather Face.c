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

static const uint32_t INBOUND_SIZE	=	128; // Inbound app message size
static const uint32_t OUTBOUND_SIZE	=	128; // Outbound app message size
static const uint16_t INTERVAL		=	15; // Minutes between weather checks
static uint16_t PADDING_LEFT		=	0; // Padding for left of screen
static uint16_t PADDING_RIGHT		=	0; // Padding for right of screen
static uint16_t PADDING_TOP			=	6; // Padding for top of screen
static uint16_t PADDING_BOTTOM		=	6; // Padding for bottom of screen
static const uint16_t SCR_W			=	144; // Screen width
static const uint16_t SCR_H			=	168; // Screen height

static Window *window;
static TextLayer *city_layer;
static TextLayer *temp_layer;
static TextLayer *desc_layer;
static BitmapLayer *icon_layer;
static GBitmap *icon = NULL;
static GFont font_tiny;
static GFont font_small;
static GFont font_medium;
static GFont font_large;

static char city_buff[50];
static char temp_buff[50];
static char desc_buff[50];
static char icon_buff[50];

enum dict_keys {
	STATUS,
	CITY,
	TEMP,
	DESC,
	ICON
};


static void set_layer_frames() {
	uint16_t city_w	=	SCR_W - (PADDING_LEFT + PADDING_RIGHT);
	uint16_t city_h	=	40;
	uint16_t city_x	=	PADDING_LEFT;
	uint16_t city_y	=	PADDING_TOP;

	uint16_t desc_w	=	SCR_W - (PADDING_LEFT + PADDING_RIGHT);
	uint16_t desc_h	=	20;
	uint16_t desc_x	=	PADDING_LEFT;
	uint16_t desc_y	=	SCR_H - (desc_h + PADDING_BOTTOM);

	uint16_t temp_w	=	SCR_W - (PADDING_LEFT + PADDING_RIGHT);
	uint16_t temp_h	=	20;
	uint16_t temp_x	=	PADDING_LEFT;
	uint16_t temp_y	=	SCR_H - (desc_h + temp_h + PADDING_BOTTOM);

	uint16_t icon_w	=	SCR_W - (PADDING_LEFT + PADDING_RIGHT);
	uint16_t icon_h	=	SCR_H - (city_h + temp_h + desc_h + PADDING_TOP);
	uint16_t icon_x	=	PADDING_LEFT;
	uint16_t icon_y	=	city_h;

	layer_set_frame(text_layer_get_layer(city_layer), GRect(city_x, city_y, city_w, city_h));
	layer_set_frame(text_layer_get_layer(desc_layer), GRect(desc_x, desc_y, desc_w, desc_h));
	layer_set_frame(text_layer_get_layer(temp_layer), GRect(temp_x, temp_y, temp_w, temp_h));
	layer_set_frame(bitmap_layer_get_layer(icon_layer), GRect(icon_x, icon_y, icon_w, icon_h));
}


/**
 * Outgoing message sent
 */
static void sent_handler(DictionaryIterator *sent, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Message sent");
}


/**
 * Outgoing message failed to send
 */
static void failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Failed to send message: %s", (char*)reason);
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
	char *status = (char*)dict_find(message, STATUS)->value;

	if(strcmp(status, "ready") == 0) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Recieved status \"ready\"");

		check_weather();
		tick_timer_service_subscribe(MINUTE_UNIT, &tick_handle);
	}

	else if(strcmp(status, "reporting") == 0) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Recieved status \"reporting\"");

		// Get values from dictionary
		strcpy(city_buff, (char*)dict_find(message, CITY)->value);
		strcpy(temp_buff, (char*)dict_find(message, TEMP)->value);
		strcpy(desc_buff, (char*)dict_find(message, DESC)->value);
		strcpy(icon_buff, (char*)dict_find(message, ICON)->value);

		if(strlen(city_buff) > 12 && strlen(city_buff) <= 26) {
			text_layer_set_font(city_layer, font_medium);
			PADDING_TOP = 0;
		}
		else if(strlen(city_buff) > 26) {
			text_layer_set_font(city_layer, font_tiny);
			PADDING_TOP = 0;
		}
		else {
			text_layer_set_font(city_layer, font_large);
			PADDING_TOP = 6;
		}

		set_layer_frames();

		// Set text layers
		text_layer_set_text(city_layer, city_buff);
		text_layer_set_text(temp_layer, temp_buff);
		text_layer_set_text(desc_layer, desc_buff);

		// If icon is already set, free it first
		if(icon != NULL)
			gbitmap_destroy(icon);

		// Set appropriate icon based on icon code
		if(strcmp(icon_buff, "01d") == 0)
			icon = gbitmap_create_with_resource(RESOURCE_ID_CLEAR_DAY);

		else if(strcmp(icon_buff, "01n") == 0)
			icon = gbitmap_create_with_resource(RESOURCE_ID_CLEAR_NIGHT);

		else if(strcmp(icon_buff, "02d") == 0)
			icon = gbitmap_create_with_resource(RESOURCE_ID_FEW_CLOUDS_DAY);

		else if(strcmp(icon_buff, "02n") == 0)
			icon = gbitmap_create_with_resource(RESOURCE_ID_FEW_CLOUDS_NIGHT);

		else if(strcmp(icon_buff, "03d") == 0 || strcmp(icon_buff, "04d") == 0)
			icon = gbitmap_create_with_resource(RESOURCE_ID_SCATTERED_CLOUDS_DAY);

		else if(strcmp(icon_buff, "03n") == 0 || strcmp(icon_buff, "04n") == 0)
			icon = gbitmap_create_with_resource(RESOURCE_ID_SCATTERED_CLOUDS_NIGHT);

		else if(strcmp(icon_buff, "09d") == 0 || strcmp(icon_buff, "09n") == 0 ||
				strcmp(icon_buff, "10d") == 0 || strcmp(icon_buff, "10n") == 0)
			icon = gbitmap_create_with_resource(RESOURCE_ID_RAIN);

		else if(strcmp(icon_buff, "11d") == 0 || strcmp(icon_buff, "11n") == 0)
			icon = gbitmap_create_with_resource(RESOURCE_ID_THUNDERSTORM);

		else if(strcmp(icon_buff, "13d") == 0 || strcmp(icon_buff, "13n") == 0)
			icon = gbitmap_create_with_resource(RESOURCE_ID_SNOW);

		else if(strcmp(icon_buff, "50d") == 0 || strcmp(icon_buff, "50n") == 0)
			icon = gbitmap_create_with_resource(RESOURCE_ID_MIST);

		else {
			APP_LOG(APP_LOG_LEVEL_DEBUG, "Can't find icon: %s", icon_buff);
			icon = NULL;
		}

		// Set icon (as long as it's not NULL)
		if(icon != NULL)
			bitmap_layer_set_bitmap(icon_layer, icon);
	}

	else if(strcmp(status, "configUpdated") == 0) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Recieved status \"configUpdate\"");

		check_weather();
	}
}


/**
 * Failed to receive message
 */
static void dropped_handler(AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Message dropped: %s", (char*)reason);
}


static void init(void) {
	// Init fonts
	font_tiny	=	fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_12));
	font_small	=	fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_16));
	font_medium	=	fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_18));
	font_large	=	fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_20));


	// Init window
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Initializing window");
	window = window_create();
	window_set_background_color(window, GColorBlack);
	window_stack_push(window, true);
	Layer *root_layer = window_get_root_layer(window);
	GRect frame = layer_get_frame(root_layer);

	// Init city_layer
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Initializing city layer");
	city_layer = text_layer_create(frame);
	text_layer_set_font(city_layer, font_large);
	text_layer_set_text_alignment(city_layer, GTextAlignmentCenter);
	text_layer_set_text_color(city_layer, GColorWhite);
	text_layer_set_background_color(city_layer, GColorBlack);
	text_layer_set_overflow_mode(city_layer, GTextOverflowModeFill);
	layer_add_child(root_layer, text_layer_get_layer(city_layer));

	// Init desc_layer
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Initializing description layer");
	desc_layer = text_layer_create(frame);
	text_layer_set_font(desc_layer, font_small);
	text_layer_set_text_alignment(desc_layer, GTextAlignmentCenter);
	text_layer_set_text_color(desc_layer, GColorWhite);
	text_layer_set_background_color(desc_layer, GColorBlack);
	layer_add_child(root_layer, text_layer_get_layer(desc_layer));

	// Init temp_layer
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Initializing temperature layer");
	temp_layer = text_layer_create(frame);
	text_layer_set_font(temp_layer, font_small);
	text_layer_set_text_alignment(temp_layer, GTextAlignmentCenter);
	text_layer_set_text_color(temp_layer, GColorWhite);
	text_layer_set_background_color(temp_layer, GColorBlack);
	layer_add_child(root_layer, text_layer_get_layer(temp_layer));

	// Init icon_layer
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Initializing icon layer");
	icon_layer = bitmap_layer_create(frame);
	bitmap_layer_set_background_color(icon_layer, GColorClear);
	bitmap_layer_set_alignment(icon_layer, GAlignTop);
	layer_add_child(root_layer, bitmap_layer_get_layer(icon_layer));

	// Init app message and message handlers
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Initializing handlers");
	app_message_register_outbox_sent(sent_handler);
	app_message_register_outbox_failed(failed_handler);
	app_message_register_inbox_received(received_handler);
	app_message_register_inbox_dropped(dropped_handler);
	app_message_open(INBOUND_SIZE, OUTBOUND_SIZE);

	set_layer_frames();

	text_layer_set_text(city_layer, "loading...");
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Finished initializing");
}


static void deinit(void) {
	text_layer_destroy(city_layer);
	text_layer_destroy(temp_layer);
	text_layer_destroy(desc_layer);
	bitmap_layer_destroy(icon_layer);
	fonts_unload_custom_font(font_tiny);
	fonts_unload_custom_font(font_small);
	fonts_unload_custom_font(font_medium);
	fonts_unload_custom_font(font_large);
	if(icon != NULL)
		gbitmap_destroy(icon);
	window_destroy(window);
}


int main(void) {
	init();
	app_event_loop();
	deinit();
}